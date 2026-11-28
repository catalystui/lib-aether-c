#ifndef ATHDVLIB_LINUX_HPP
#define ATHDVLIB_LINUX_HPP
#include "catalyst.hpp"
#include "catmmlib.h"
#include "athdvlib.h"
#include <errno.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>


static inline CATALYST_STATUS_CODE athdv_errno(int error) {
    switch (error) {
        case EACCES: case EPERM: return catalyst::STATUS_CODE_ERROR_ACCESS_DENIED;
        case ENOENT: case ENODEV: case ENXIO: return catalyst::STATUS_CODE_ERROR_NOT_FOUND;
        case EINTR: return catalyst::STATUS_CODE_ERROR_INTERRUPTED;
        case ENOMEM: return catalyst::STATUS_CODE_ERROR_ALLOCATION_FAILED;
        case ENOTTY: case EOPNOTSUPP: return catalyst::STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
        default: return catalyst::STATUS_CODE_ERROR_IO_ERROR;
    }
}

/* Native path and identifier fields contain terminated byte sequences. These
 * operations are private to the Linux interface, not general text APIs. */
static inline catalyst::NUINT athdv_native_extent(const char* value) {
    const char* end = value;
    while (*end) ++end;
    return (catalyst::NUINT) (end - value);
}

static inline catalyst::BOOL athdv_native_equal(const char* left, const char* right) {
    catalyst::NUINT extent = athdv_native_extent(left);
    return extent == athdv_native_extent(right) &&
        catmm_compare(left, right, extent) == 0 ? catalyst::TRUE : catalyst::FALSE;
}

static inline const char* athdv_basename(const char* path) {
    const char* name = path;
    while (*path) {
        if (*path == '/') name = path + 1;
        ++path;
    }
    return name;
}

static inline catalyst::BOOL athdv_event_name(const char* name) {
    /* Inspect only bytes before the terminator. */
    const char prefix[] = "event";
    for (catalyst::BYTE j = 0; j < 5; ++j)
        if (name[j] != prefix[j]) return catalyst::FALSE;
    name += 5;
    if (!*name) return catalyst::FALSE;
    while (*name) {
        if (*name < '0' || *name > '9') return catalyst::FALSE;
        ++name;
    }
    return catalyst::TRUE;
}

/* Append an entire terminated native path component, including its terminator.
 * Caller supplies used < capacity and a valid source. */
static inline catalyst::BOOL athdv_append(char* path, catalyst::NUINT capacity, catalyst::NUINT* used, const char* component) {
    catalyst::NUINT extent = athdv_native_extent(component);
    if (extent >= capacity - *used) return catalyst::FALSE;
    catmm_copy(path + *used, component, extent + 1);
    *used += extent;
    return catalyst::TRUE;
}

static inline catalyst::BOOL athdv_join(char* path, catalyst::NUINT capacity, const char* root, const char* name) {
    catalyst::NUINT used = 0;
    return athdv_append(path, capacity, &used, root) &&
        athdv_append(path, capacity, &used, "/") &&
        athdv_append(path, capacity, &used, name) ? catalyst::TRUE : catalyst::FALSE;
}

/* Linux's native device numbers, file descriptors and event codes fit a
 * 32-bit unsigned integer; format only those native decimal fields. */
#if !CATALYST_SUPPORTS_32BIT
#error "Linux device discovery requires 32-bit native device number support."
#endif
static inline catalyst::BOOL athdv_append_number(char* path, catalyst::NUINT capacity, catalyst::NUINT* used, catalyst::UINT number) {
    char digits[11];
    catalyst::BYTE first = 10;
    digits[first] = 0;
    do {
        --first;
        digits[first] = (char) ('0' + number % (catalyst::UINT) 10);
        number /= (catalyst::UINT) 10;
    } while (number);
    return athdv_append(path, capacity, used, digits + first);
}

/* Resolve sysfs paths through Linux's kernel-provided descriptor links,
 * rather than the C library's realpath routine. Both sysfs and procfs must
 * be accessible. A truncated link is never returned as a valid identity. */
static inline catalyst::STATUS_CODE athdv_resolve(const char* path, char* resolved, catalyst::NUINT capacity) {
    int fd = open(path, O_PATH | O_CLOEXEC);
    if (fd < 0) return athdv_errno(errno);
    char link[32];
    catalyst::NUINT used = 0;
    catalyst::STATUS_CODE status = catalyst::STATUS_CODE_SUCCESS;
    if (!athdv_append(link, sizeof(link), &used, "/proc/self/fd/") ||
        !athdv_append_number(link, sizeof(link), &used, (catalyst::UINT) fd)) {
        status = catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW;
    } else {
        ssize_t got = readlink(link, resolved, capacity);
        if (got < 0) status = athdv_errno(errno);
        else if ((catalyst::NUINT) got >= capacity) status = catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW;
        else if (got == 0 || resolved[0] != '/') status = catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT;
        else resolved[got] = 0;
    }
    if (close(fd) != 0 && status == catalyst::STATUS_CODE_SUCCESS) status = athdv_errno(errno);
    return status;
}

#endif /* ATHDVLIB_LINUX_HPP */
