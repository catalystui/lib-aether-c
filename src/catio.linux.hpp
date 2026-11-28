#ifndef CATIO_LINUX_HPP
#define CATIO_LINUX_HPP
#include "catalyst.hpp"
#include "catmmlib.h"
#include "catdvlib.h"
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <linux/input.h>

using namespace catalyst;

/* Native Linux bitmap and integer representation; not part of the public ABI. */
#define CATIO_BITS (sizeof(unsigned long) * CHAR_BIT)
#define CATIO_WORDS(n) (((n) + CATIO_BITS - 1) / CATIO_BITS)

static inline CATALYST_STATUS_CODE catio_errno(int error) {
    switch (error) {
        case EACCES: case EPERM: return STATUS_CODE_ERROR_ACCESS_DENIED;
        case ENOENT: case ENODEV: case ENXIO: return STATUS_CODE_ERROR_NOT_FOUND;
        case EINTR: return STATUS_CODE_ERROR_INTERRUPTED;
        case ENOMEM: return STATUS_CODE_ERROR_ALLOCATION_FAILED;
        case ENOTTY: case EOPNOTSUPP: return STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
        default: return STATUS_CODE_ERROR_IO_ERROR;
    }
}

static inline CATALYST_BOOL catio_bit(const unsigned long* bits, unsigned int index) {
    return (bits[index / CATIO_BITS] & (1UL << (index % CATIO_BITS))) ? CATALYST_TRUE : CATALYST_FALSE;
}

struct CATIO_CHANNEL {
    unsigned short type;
    unsigned short code;
    char identifier[32];
};

struct CATIO_CHANNELS {
    CATALYST_NUINT count;
    CATIO_CHANNEL data[KEY_CNT + ABS_CNT + REL_CNT + LED_CNT];
};

static inline CATALYST_BOOL catio_contains(const CATIO_CHANNELS* channels, unsigned short type, unsigned short code) {
    for (CATALYST_NUINT j = 0; j < channels->count; ++j) {
        if (channels->data[j].type == type && channels->data[j].code == code) return CATALYST_TRUE;
    }
    return CATALYST_FALSE;
}

/* Resolve the enumerated sysfs event instance and verify the opened device node.
 * Identity is scoped to the current kernel device lifetime, not a permanent ID. */
static inline CATALYST_RESULT catio_open(const CATDVLIB_DEVICE* device, int mode, int* fd) {
    *fd = -1;
    CATDVLIB_IDENTIFIER id;
    CATALYST_RESULT r = catdv_device_get_identifier(device, &id);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return RESULT(r.status, 0x80, 1, r.detail);
    if (strcmp((const char*) id.scheme, "linux-sysfs") != 0) return RESULT(STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED, 0x80, 2, 0);
    const char* name = strrchr((const char*) id.value, '/');
    if (!name || strncmp(++name, "event", 5) != 0 || !name[5] || strspn(name + 5, "0123456789") != strlen(name + 5))
        return RESULT(STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED, 0x80, 3, 0);
    char path[PATH_MAX];
    int len = snprintf(path, sizeof(path), "/dev/input/%s", name);
    if (len < 0 || (size_t) len >= sizeof(path)) return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x80, 4, 0);
    int handle = open(path, mode | O_NONBLOCK | O_CLOEXEC);
    if (handle < 0) return RESULT(catio_errno(errno), 0x80, 5, 0);
    struct stat info;
    CATALYST_STATUS_CODE status = STATUS_CODE_SUCCESS;
    char resolved[PATH_MAX];
    if (fstat(handle, &info) != 0) status = catio_errno(errno);
    else if (!S_ISCHR(info.st_mode)) status = STATUS_CODE_ERROR_MALFORMED_INPUT;
    else {
        len = snprintf(path, sizeof(path), "/sys/dev/char/%u:%u", major(info.st_rdev), minor(info.st_rdev));
        if (len < 0 || (size_t) len >= sizeof(path)) status = STATUS_CODE_ERROR_BUFFER_OVERFLOW;
        else if (!realpath(path, resolved)) status = catio_errno(errno);
        else if (strcmp(resolved, (const char*) id.value) != 0) status = STATUS_CODE_ERROR_NOT_FOUND;
    }
    if (status != STATUS_CODE_SUCCESS) {
        if (close(handle) != 0) return RESULT(catio_errno(errno), 0x80, 6, 0);
        return RESULT(status, 0x80, 7, 0);
    }
    *fd = handle;
    return RESULT(STATUS_CODE_SUCCESS, 0x80, 8, 0);
}

/* Capture native channel order. The event type bitmap uses native unsigned longs. */
static inline CATALYST_RESULT catio_channels(int fd, CATALYST_BOOL input, CATIO_CHANNELS* channels) {
    channels->count = 0;
    unsigned long events[CATIO_WORDS(EV_CNT)] = { 0 };
    if (ioctl(fd, EVIOCGBIT(0, sizeof(events)), events) < 0) return RESULT(catio_errno(errno), 0x81, 1, 0);
    const unsigned short types[] = { EV_KEY, EV_ABS, EV_REL, EV_LED };
    const unsigned int limits[] = { KEY_CNT, ABS_CNT, REL_CNT, LED_CNT };
    for (CATALYST_BYTE k = input ? 0 : 3; k < (input ? 3 : 4); ++k) {
        if (!catio_bit(events, types[k])) continue;
        unsigned long bits[CATIO_WORDS(KEY_CNT)] = { 0 };
        if (ioctl(fd, EVIOCGBIT(types[k], sizeof(bits)), bits) < 0) return RESULT(catio_errno(errno), 0x81, 2, 0);
        for (unsigned int j = 0; j < limits[k]; ++j) {
            if (!catio_bit(bits, j)) continue;
            if (channels->count == (CATALYST_NUINT) -1) return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x81, 3, 0);
            CATIO_CHANNEL* c = &channels->data[channels->count];
            c->type = types[k]; c->code = (unsigned short) j;
            int len = snprintf(c->identifier, sizeof(c->identifier), "%u:%u", (unsigned int) c->type, j);
            if (len < 0 || (size_t) len >= sizeof(c->identifier)) return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x81, 4, 0);
            ++channels->count;
        }
    }
    if (!input && channels->count == 0) {
        if (catio_bit(events, EV_SND) || catio_bit(events, EV_FF)) return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x81, 5, 0);
        return RESULT(STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED, 0x81, 6, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x81, 7, 0);
}

static inline CATALYST_RESULT catio_write_led(int fd, unsigned short code, int value) {
    struct input_event event;
    catmm_fill(&event, 0, sizeof(event));
    event.type = EV_LED; event.code = code; event.value = value;
    ssize_t written = write(fd, &event, sizeof(event));
    if (written < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x82, 1, 0);
        return RESULT(catio_errno(errno), 0x82, 2, 0);
    }
    if (written == 0) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x82, 3, 0);
    if ((size_t) written != sizeof(event)) return RESULT(STATUS_CODE_ERROR_IO_ERROR, 0x82, 4, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x82, 5, 0);
}
#endif /* CATIO_LINUX_HPP */
