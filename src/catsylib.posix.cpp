#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif
#include "cmakecfg.h"
#if TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN
#include "catsylib.internal.hpp"
#include "catalyst.hpp"
#include "catmmlib.h"
#include <sys/utsname.h>
#include <errno.h>
#if TARGET_PLATFORM_LINUX
#include <fcntl.h>
#include <unistd.h>
#endif
#if TARGET_PLATFORM_DARWIN
#include <sys/sysctl.h>
/* sysctl.h transitively includes Mach Boolean macros. */
#undef TRUE
#undef FALSE
#endif


/* -------------------------------------------------------------------------------------------------
 * Native text retrieval
 * ------------------------------------------------------------------------------------------------- */

static catalyst::STATUS_CODE catsy_errno_status(int error) {
    switch (error) {
        case EACCES:
        case EPERM: return catalyst::STATUS_CODE_ERROR_ACCESS_DENIED;
        case ENOENT: return catalyst::STATUS_CODE_ERROR_NOT_FOUND;
        case ENOSYS: return catalyst::STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
        case EINTR: return catalyst::STATUS_CODE_ERROR_INTERRUPTED;
        case ENOMEM: return catalyst::STATUS_CODE_ERROR_ALLOCATION_FAILED;
        default: return catalyst::STATUS_CODE_ERROR_IO_ERROR;
    }
}

/* Native byte strings use the UTF-8 code-unit representation. */
static CATALYST_RESULT catsy_copy(CATSYLIB_FIELD field, const char* value,
    catalyst::UTF8W output, catalyst::NUINT capacity, catalyst::NUINT* required) {
    return catsy_copy_utf8(field, (catalyst::UTF8) value, output, capacity, required);
}

/* Compare a terminated native value with a known literal without reading past it. */
static catalyst::BOOL catsy_equals(const char* value, const char* literal) {
    while (*literal != 0) {
        if (*value++ != *literal++) return catalyst::FALSE;
    }
    return *value == 0 ? catalyst::TRUE : catalyst::FALSE;
}

#if TARGET_PLATFORM_LINUX
/* -------------------------------------------------------------------------------------------------
 * Linux distribution identification
 * ------------------------------------------------------------------------------------------------- */

/* Decode one os-release assignment in place. Never execute the file or expand
 * variables. Quoted and escaped shell characters are treated as data. */
static catalyst::BOOL catsy_os_release_value(char* value) {
    char* source = value;
    char* destination = value;
    char quote = 0;
    catalyst::BOOL closed = catalyst::FALSE;
    if (*source == '\'' || *source == '"') quote = *source++;
    while (*source != 0) {
        char character = *source++;
        if (quote != 0 && character == quote) {
            closed = catalyst::TRUE;
            break;
        }
        if (quote == 0 && (character == ' ' || character == '\t')) {
            --source;
            break;
        }
        if (quote != '\'' && character == '\\') {
            if (*source == 0) return catalyst::FALSE;
            if (quote == '"' && *source != '"' && *source != '\\' &&
                *source != '$' && *source != '\x60') {
                *destination++ = character;
                continue;
            }
            character = *source++;
        } else if (quote != '\'' && (character == '$' || character == '\x60')) {
            return catalyst::FALSE;
        } else if (quote == 0 && (character == '\'' || character == '"')) {
            return catalyst::FALSE;
        }
        *destination++ = character;
    }
    if (quote != 0 && !closed) return catalyst::FALSE;
    if (*source != 0) {
        if (*source != ' ' && *source != '\t') return catalyst::FALSE;
        while (*source == ' ' || *source == '\t') ++source;
        if (*source != 0 && *source != '#') return catalyst::FALSE;
    }
    *destination = 0;
    return catalyst::TRUE;
}

static CATALYST_RESULT catsy_distribution(CATSYLIB_FIELD field, catalyst::UTF8W output,
    catalyst::NUINT capacity, catalyst::NUINT* required) {
    catalyst::CONTEXT_CODE context = (catalyst::CONTEXT_CODE) field;
    int file = open("/etc/os-release", O_RDONLY);
    if (file < 0 && errno == ENOENT) file = open("/usr/lib/os-release", O_RDONLY);
    if (file < 0) return catalyst::RESULT(catsy_errno_status(errno), context, 0x10, 0);

    /* Most os-release files fit on the stack; larger files grow through Core. */
    char local[4096];
    CATMMLIB_POINTERW storage = 0;
    char* data = local;
    catalyst::NUINT extent = (catalyst::NUINT) sizeof(local);
    catalyst::NUINT length = 0;
    catalyst::RESULT result(catalyst::STATUS_CODE_SUCCESS, context, 0x01, 0);
    for (;;) {
        if (length == extent - (catalyst::NUINT) 1) {
            if (extent == CATALYST_NUINT_MAX) {
                result = catalyst::RESULT(catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x1D, 0);
                break;
            }
            catalyst::NUINT next = extent > CATALYST_NUINT_MAX / (catalyst::NUINT) 2
                ? CATALYST_NUINT_MAX : extent * (catalyst::NUINT) 2;
            catalyst::BOOL first = storage == 0 ? catalyst::TRUE : catalyst::FALSE;
            catalyst::RESULT grown = catmm_realloc(&storage, next);
            if (!catalyst::status_code_is_success(grown.status)) {
                result = catalyst::RESULT(grown.status, context, 0x1E, grown.detail);
                break;
            }
            if (first) catmm_copy(storage, local, length);
            data = (char*) storage;
            extent = next;
        }
        catalyst::NUINT available = extent - length - (catalyst::NUINT) 1;
        /* Bound a native read to a small representable count. */
        if (available > (catalyst::NUINT) sizeof(local)) available = (catalyst::NUINT) sizeof(local);
        ssize_t count = read(file, data + length, (size_t) available);
        if (count < 0) {
            result = catalyst::RESULT(catsy_errno_status(errno), context, 0x11, 0);
            break;
        }
        if (count == 0) break;
        length += (catalyst::NUINT) count;
    }
    if (close(file) != 0 && catalyst::status_code_is_success(result.status)) {
        result = catalyst::RESULT(catsy_errno_status(errno), context, 0x12, 0);
    }
    if (catalyst::status_code_is_success(result.status)) {
        data[length] = 0;
        const char* key = field == CATSYLIB_FIELD_DISTRIBUTION ? "NAME" : "VERSION_ID";
        catalyst::NUINT keyLength = field == CATSYLIB_FIELD_DISTRIBUTION ? (catalyst::NUINT) 4 : (catalyst::NUINT) 10;
        char* found = 0;
        catalyst::NUINT begin = 0;
        while (begin < length) {
            catalyst::NUINT end = begin;
            while (end < length && data[end] != '\n') {
                if (data[end] == 0) {
                    result = catalyst::RESULT(catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x20, 0);
                    break;
                }
                ++end;
            }
            if (!catalyst::status_code_is_success(result.status)) break;
            catalyst::NUINT next = end < length ? end + (catalyst::NUINT) 1 : end;
            while (end > begin && data[end - (catalyst::NUINT) 1] == '\r') --end;
            data[end] = 0;
            if (end - begin > keyLength && data[begin + keyLength] == '=' &&
                catmm_compare(data + begin, key, keyLength) == 0) {
                char* value = data + begin + keyLength + (catalyst::NUINT) 1;
                if (!catsy_os_release_value(value)) {
                    result = catalyst::RESULT(catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x21, 0);
                    break;
                }
                found = value; /* Last assignment wins. */
            }
            begin = next;
        }
        if (catalyst::status_code_is_success(result.status)) {
            if (found == 0) result = catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_FOUND, context, 0x13, 0);
            else result = catsy_copy(field, found, output, capacity, required);
        }
    }
    return catsy_release(storage, result, 0x22, required);
}
#endif

#if TARGET_PLATFORM_DARWIN
/* -------------------------------------------------------------------------------------------------
 * Darwin distribution identification
 * ------------------------------------------------------------------------------------------------- */

static CATALYST_RESULT catsy_distribution_version(CATSYLIB_FIELD field, catalyst::UTF8W output,
    catalyst::NUINT capacity, catalyst::NUINT* required) {
    catalyst::CONTEXT_CODE context = (catalyst::CONTEXT_CODE) field;
    char local[128];
    char* value = local;
    CATMMLIB_POINTERW storage = 0;
    size_t length = sizeof(local);
    size_t allocated = length;
    if (sysctlbyname("kern.osproductversion", value, &length, 0, 0) != 0) {
        int error = errno;
        if (error != ENOMEM) {
            return catalyst::RESULT(error == ENOENT ? catalyst::STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED
                : catsy_errno_status(error), context, 0x14, 0);
        }
        length = 0;
        if (sysctlbyname("kern.osproductversion", 0, &length, 0, 0) != 0) {
            return catalyst::RESULT(errno == ENOENT ? catalyst::STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED
                : catsy_errno_status(errno), context, 0x23, 0);
        }
        if (length == 0) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_FOUND, context, 0x15, 0);
        if (length > CATALYST_NUINT_MAX) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x24, 0);
        allocated = length;
        catalyst::RESULT allocation = catmm_alloc(&storage, (catalyst::NUINT) length);
        if (!catalyst::status_code_is_success(allocation.status)) {
            return catalyst::RESULT(allocation.status, context, 0x16, allocation.detail);
        }
        value = (char*) storage;
        if (sysctlbyname("kern.osproductversion", value, &length, 0, 0) != 0) {
            return catsy_release(storage, catalyst::RESULT(catsy_errno_status(errno), context, 0x17, 0), 0x25, required);
        }
    }
    catalyst::RESULT result;
    if (length == 0 || length > allocated || value[length - 1] != 0) {
        result = catalyst::RESULT(catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x18, 0);
    } else {
        size_t i = 0;
        while (i < length - 1 && value[i] != 0) ++i;
        if (i != length - 1) result = catalyst::RESULT(catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x26, 0);
        else result = catsy_copy(field, value, output, capacity, required);
    }
    return catsy_release(storage, result, 0x27, required);
}
#endif

/* -------------------------------------------------------------------------------------------------
 * System identification
 * ------------------------------------------------------------------------------------------------- */

CATALYST_RESULT catsy_platform_get(CATSYLIB_FIELD field, catalyst::UTF8W output,
    catalyst::NUINT capacity, catalyst::NUINT* required) {
    catalyst::CONTEXT_CODE context = (catalyst::CONTEXT_CODE) field;
#if TARGET_PLATFORM_LINUX
    if (field == CATSYLIB_FIELD_DISTRIBUTION || field == CATSYLIB_FIELD_DISTRIBUTION_VERSION) {
        return catsy_distribution(field, output, capacity, required);
    }
#elif TARGET_PLATFORM_DARWIN
    if (field == CATSYLIB_FIELD_DISTRIBUTION) return catsy_copy(field, "macOS", output, capacity, required);
    if (field == CATSYLIB_FIELD_DISTRIBUTION_VERSION) {
        return catsy_distribution_version(field, output, capacity, required);
    }
    if (field == CATSYLIB_FIELD_ARCHITECTURE) {
        int translated = 0;
        size_t size = sizeof(translated);
        if (sysctlbyname("sysctl.proc_translated", &translated, &size, 0, 0) == 0) {
            if (size != sizeof(translated) || (translated != 0 && translated != 1)) {
                return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x1B, 0);
            }
            if (translated == 1) return catsy_copy(field, "arm64", output, capacity, required);
        } else if (errno != ENOENT) {
            return catalyst::RESULT(catsy_errno_status(errno), context, 0x1C, 0);
        }
    }
#endif
    struct utsname system;
    if (uname(&system) != 0) return catalyst::RESULT(catsy_errno_status(errno), context, 0x19, 0);
    switch (field) {
        case CATSYLIB_FIELD_NAME: return catsy_copy(field, system.sysname, output, capacity, required);
        case CATSYLIB_FIELD_VERSION: return catsy_copy(field, system.release, output, capacity, required);
        case CATSYLIB_FIELD_HOSTNAME: return catsy_copy(field, system.nodename, output, capacity, required);
        case CATSYLIB_FIELD_ARCHITECTURE:
            if (catsy_equals(system.machine, "aarch64")) {
                return catsy_copy(field, "arm64", output, capacity, required);
            }
            if (catsy_equals(system.machine, "amd64")) {
                return catsy_copy(field, "x86_64", output, capacity, required);
            }
            if (catsy_equals(system.machine, "i386") || catsy_equals(system.machine, "i486") ||
                catsy_equals(system.machine, "i586") || catsy_equals(system.machine, "i686")) {
                return catsy_copy(field, "x86", output, capacity, required);
            }
            return catsy_copy(field, system.machine, output, capacity, required);
        default: return catalyst::RESULT(catalyst::STATUS_CODE_FATAL_INVARIANT_VIOLATION, context, 0x1A, 0);
    }
}

#endif /* TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN */
