#ifndef CATIO_LINUX_HPP
#define CATIO_LINUX_HPP
#include "catalyst.hpp"
#include "catmmlib.h"
#include "catdvlib.h"
#include <errno.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <linux/input.h>

using namespace catalyst;

/* Native Linux bitmap and integer representation; not part of the public ABI. */
#define CATIO_BITS (sizeof(unsigned long) * 8)
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

/* Native path and identifier fields contain terminated byte sequences. These
 * operations are private to the Linux interface, not general text APIs. */
static inline NUINT catio_native_extent(const char* value) {
    const char* end = value;
    while (*end) ++end;
    return (NUINT) (end - value);
}

static inline BOOL catio_native_equal(const char* left, const char* right) {
    NUINT extent = catio_native_extent(left);
    return extent == catio_native_extent(right) &&
        catmm_compare(left, right, extent) == 0 ? TRUE : FALSE;
}

static inline const char* catio_basename(const char* path) {
    const char* name = path;
    while (*path) {
        if (*path == '/') name = path + 1;
        ++path;
    }
    return name;
}

static inline BOOL catio_event_name(const char* name) {
    /* Inspect only bytes before the terminator. */
    const char prefix[] = "event";
    for (BYTE j = 0; j < 5; ++j)
        if (name[j] != prefix[j]) return FALSE;
    name += 5;
    if (!*name) return FALSE;
    while (*name) {
        if (*name < '0' || *name > '9') return FALSE;
        ++name;
    }
    return TRUE;
}

/* Append an entire terminated native path component, including its terminator.
 * Caller supplies used < capacity and a valid source. */
static inline BOOL catio_append(char* path, NUINT capacity, NUINT* used, const char* component) {
    NUINT extent = catio_native_extent(component);
    if (extent >= capacity - *used) return FALSE;
    catmm_copy(path + *used, component, extent + 1);
    *used += extent;
    return TRUE;
}

static inline BOOL catio_join(char* path, NUINT capacity, const char* root, const char* name) {
    NUINT used = 0;
    return catio_append(path, capacity, &used, root) &&
        catio_append(path, capacity, &used, "/") &&
        catio_append(path, capacity, &used, name) ? TRUE : FALSE;
}

/* Linux's native device numbers, file descriptors and event codes fit a
 * 32-bit unsigned integer; format only those native decimal fields. */
#if !CATALYST_SUPPORTS_32BIT
#error "Linux device I/O requires 32-bit native device number support."
#endif
static inline BOOL catio_append_number(char* path, NUINT capacity, NUINT* used, UINT number) {
    char digits[11];
    BYTE first = 10;
    digits[first] = 0;
    do {
        --first;
        digits[first] = (char) ('0' + number % (UINT) 10);
        number /= (UINT) 10;
    } while (number);
    return catio_append(path, capacity, used, digits + first);
}

/* Resolve sysfs paths through Linux's kernel-provided descriptor links,
 * rather than the C library's realpath routine. Both sysfs and procfs must
 * be accessible. A truncated link is never returned as a valid identity. */
static inline STATUS_CODE catio_resolve(const char* path, char* resolved, NUINT capacity) {
    int fd = open(path, O_PATH | O_CLOEXEC);
    if (fd < 0) return catio_errno(errno);
    char link[32];
    NUINT used = 0;
    STATUS_CODE status = STATUS_CODE_SUCCESS;
    if (!catio_append(link, sizeof(link), &used, "/proc/self/fd/") ||
        !catio_append_number(link, sizeof(link), &used, (UINT) fd)) {
        status = STATUS_CODE_ERROR_BUFFER_OVERFLOW;
    } else {
        ssize_t got = readlink(link, resolved, capacity);
        if (got < 0) status = catio_errno(errno);
        else if ((NUINT) got >= capacity) status = STATUS_CODE_ERROR_BUFFER_OVERFLOW;
        else if (got == 0 || resolved[0] != '/') status = STATUS_CODE_ERROR_MALFORMED_INPUT;
        else resolved[got] = 0;
    }
    if (close(fd) != 0 && status == STATUS_CODE_SUCCESS) status = catio_errno(errno);
    return status;
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
    if (!catio_native_equal((const char*) id.scheme, "linux-sysfs")) return RESULT(STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED, 0x80, 2, 0);
    const char* name = catio_basename((const char*) id.value);
    if (!catio_event_name(name)) return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x80, 3, 0);
    char path[PATH_MAX];
    if (!catio_join(path, sizeof(path), "/dev/input", name)) return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x80, 4, 0);
    int handle = open(path, mode | O_NONBLOCK | O_CLOEXEC);
    if (handle < 0) return RESULT(catio_errno(errno), 0x80, 5, 0);
    struct stat info;
    CATALYST_STATUS_CODE status = STATUS_CODE_SUCCESS;
    char resolved[PATH_MAX];
    if (fstat(handle, &info) != 0) status = catio_errno(errno);
    else if (!S_ISCHR(info.st_mode)) status = STATUS_CODE_ERROR_MALFORMED_INPUT;
    else {
        NUINT used = 0;
        if (!catio_append(path, sizeof(path), &used, "/sys/dev/char/") ||
            !catio_append_number(path, sizeof(path), &used, (UINT) major(info.st_rdev)) ||
            !catio_append(path, sizeof(path), &used, ":") ||
            !catio_append_number(path, sizeof(path), &used, (UINT) minor(info.st_rdev))) {
            status = STATUS_CODE_ERROR_BUFFER_OVERFLOW;
        } else {
            status = catio_resolve(path, resolved, sizeof(resolved));
            if (status == STATUS_CODE_SUCCESS && !catio_native_equal(resolved, (const char*) id.value))
                status = STATUS_CODE_ERROR_NOT_FOUND;
        }
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
            NUINT used = 0;
            if (!catio_append_number(c->identifier, sizeof(c->identifier), &used, (UINT) c->type) ||
                !catio_append(c->identifier, sizeof(c->identifier), &used, ":") ||
                !catio_append_number(c->identifier, sizeof(c->identifier), &used, (UINT) j))
                return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x81, 4, 0);
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
