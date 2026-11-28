#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif
#include "cmakecfg.h"
#if TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN
#include "catsylib.internal.hpp"
#include <sys/utsname.h>
#include <errno.h>
#if TARGET_PLATFORM_LINUX
#include <fcntl.h>
#include <unistd.h>
#endif
#if TARGET_PLATFORM_DARWIN
#include <sys/sysctl.h>
#endif

using namespace catalyst;

/* -------------------------------------------------------------------------------------------------
 * Native text retrieval
 * ------------------------------------------------------------------------------------------------- */

static STATUS_CODE catsy_errno_status(int error) {
    switch (error) {
        case EACCES:
        case EPERM: return STATUS_CODE_ERROR_ACCESS_DENIED;
        case ENOENT: return STATUS_CODE_ERROR_NOT_FOUND;
        case ENOSYS: return STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
        case EINTR: return STATUS_CODE_ERROR_INTERRUPTED;
        case ENOMEM: return STATUS_CODE_ERROR_ALLOCATION_FAILED;
        default: return STATUS_CODE_ERROR_IO_ERROR;
    }
}

/* Native byte strings use the UTF-8 code-unit representation. */
static CATALYST_RESULT catsy_copy(CATSYLIB_FIELD field, const char* value,
    UTF8W output, NUINT capacity, NUINT* required) {
    return catsy_copy_utf8(field, (UTF8) value, output, capacity, required);
}

/* Compare a terminated native value with a known literal without reading past it. */
static BOOL catsy_equals(const char* value, const char* literal) {
    while (*literal != 0) {
        if (*value++ != *literal++) return FALSE;
    }
    return *value == 0 ? TRUE : FALSE;
}

#if TARGET_PLATFORM_LINUX
/* -------------------------------------------------------------------------------------------------
 * Linux distribution identification
 * ------------------------------------------------------------------------------------------------- */

/* Decode one os-release assignment in place. Never execute the file or expand
 * variables. Quoted and escaped shell characters are treated as data. */
static BOOL catsy_os_release_value(char* value) {
    char* source = value;
    char* destination = value;
    char quote = 0;
    BOOL closed = FALSE;
    if (*source == '\'' || *source == '"') quote = *source++;
    while (*source != 0) {
        char character = *source++;
        if (quote != 0 && character == quote) {
            closed = TRUE;
            break;
        }
        if (quote == 0 && (character == ' ' || character == '\t')) {
            --source;
            break;
        }
        if (quote != '\'' && character == '\\') {
            if (*source == 0) return FALSE;
            if (quote == '"' && *source != '"' && *source != '\\' &&
                *source != '$' && *source != '\x60') {
                *destination++ = character;
                continue;
            }
            character = *source++;
        } else if (quote != '\'' && (character == '$' || character == '\x60')) {
            return FALSE;
        } else if (quote == 0 && (character == '\'' || character == '"')) {
            return FALSE;
        }
        *destination++ = character;
    }
    if (quote != 0 && !closed) return FALSE;
    if (*source != 0) {
        if (*source != ' ' && *source != '\t') return FALSE;
        while (*source == ' ' || *source == '\t') ++source;
        if (*source != 0 && *source != '#') return FALSE;
    }
    *destination = 0;
    return TRUE;
}

static CATALYST_RESULT catsy_distribution(CATSYLIB_FIELD field, UTF8W output,
    NUINT capacity, NUINT* required) {
    CONTEXT_CODE context = (CONTEXT_CODE) field;
    int file = open("/etc/os-release", O_RDONLY);
    if (file < 0 && errno == ENOENT) file = open("/usr/lib/os-release", O_RDONLY);
    if (file < 0) return RESULT(catsy_errno_status(errno), context, 0x10, 0);

    /* Most os-release files fit on the stack; larger files grow through Core. */
    char local[4096];
    CATMMLIB_POINTERW storage = 0;
    char* data = local;
    NUINT extent = (NUINT) sizeof(local);
    NUINT length = 0;
    RESULT result(STATUS_CODE_SUCCESS, context, 0x01, 0);
    for (;;) {
        if (length == extent - (NUINT) 1) {
            if (extent == CATALYST_NUINT_MAX) {
                result = RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x1D, 0);
                break;
            }
            NUINT next = extent > CATALYST_NUINT_MAX / (NUINT) 2
                ? CATALYST_NUINT_MAX : extent * (NUINT) 2;
            BOOL first = storage == 0 ? TRUE : FALSE;
            RESULT grown = catmm_realloc(&storage, next);
            if (!status_code_is_success(grown.status)) {
                result = RESULT(grown.status, context, 0x1E, grown.detail);
                break;
            }
            if (first) catmm_copy(storage, local, length);
            data = (char*) storage;
            extent = next;
        }
        NUINT available = extent - length - (NUINT) 1;
        /* Bound a native read to a small representable count. */
        if (available > (NUINT) sizeof(local)) available = (NUINT) sizeof(local);
        ssize_t count = read(file, data + length, (size_t) available);
        if (count < 0) {
            result = RESULT(catsy_errno_status(errno), context, 0x11, 0);
            break;
        }
        if (count == 0) break;
        length += (NUINT) count;
    }
    if (close(file) != 0 && status_code_is_success(result.status)) {
        result = RESULT(catsy_errno_status(errno), context, 0x12, 0);
    }
    if (status_code_is_success(result.status)) {
        data[length] = 0;
        const char* key = field == CATSYLIB_FIELD_DISTRIBUTION ? "NAME" : "VERSION_ID";
        NUINT keyLength = field == CATSYLIB_FIELD_DISTRIBUTION ? (NUINT) 4 : (NUINT) 10;
        char* found = 0;
        NUINT begin = 0;
        while (begin < length) {
            NUINT end = begin;
            while (end < length && data[end] != '\n') {
                if (data[end] == 0) {
                    result = RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x20, 0);
                    break;
                }
                ++end;
            }
            if (!status_code_is_success(result.status)) break;
            NUINT next = end < length ? end + (NUINT) 1 : end;
            while (end > begin && data[end - (NUINT) 1] == '\r') --end;
            data[end] = 0;
            if (end - begin > keyLength && data[begin + keyLength] == '=' &&
                catmm_compare(data + begin, key, keyLength) == 0) {
                char* value = data + begin + keyLength + (NUINT) 1;
                if (!catsy_os_release_value(value)) {
                    result = RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x21, 0);
                    break;
                }
                found = value; /* Last assignment wins. */
            }
            begin = next;
        }
        if (status_code_is_success(result.status)) {
            if (found == 0) result = RESULT(STATUS_CODE_ERROR_NOT_FOUND, context, 0x13, 0);
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

static CATALYST_RESULT catsy_distribution_version(CATSYLIB_FIELD field, UTF8W output,
    NUINT capacity, NUINT* required) {
    CONTEXT_CODE context = (CONTEXT_CODE) field;
    char local[128];
    char* value = local;
    CATMMLIB_POINTERW storage = 0;
    size_t length = sizeof(local);
    size_t allocated = length;
    if (sysctlbyname("kern.osproductversion", value, &length, 0, 0) != 0) {
        int error = errno;
        if (error != ENOMEM) {
            return RESULT(error == ENOENT ? STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED
                : catsy_errno_status(error), context, 0x14, 0);
        }
        length = 0;
        if (sysctlbyname("kern.osproductversion", 0, &length, 0, 0) != 0) {
            return RESULT(errno == ENOENT ? STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED
                : catsy_errno_status(errno), context, 0x23, 0);
        }
        if (length == 0) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, context, 0x15, 0);
        if (length > CATALYST_NUINT_MAX) return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x24, 0);
        allocated = length;
        RESULT allocation = catmm_alloc(&storage, (NUINT) length);
        if (!status_code_is_success(allocation.status)) {
            return RESULT(allocation.status, context, 0x16, allocation.detail);
        }
        value = (char*) storage;
        if (sysctlbyname("kern.osproductversion", value, &length, 0, 0) != 0) {
            return catsy_release(storage, RESULT(catsy_errno_status(errno), context, 0x17, 0), 0x25, required);
        }
    }
    RESULT result;
    if (length == 0 || length > allocated || value[length - 1] != 0) {
        result = RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x18, 0);
    } else {
        size_t i = 0;
        while (i < length - 1 && value[i] != 0) ++i;
        if (i != length - 1) result = RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x26, 0);
        else result = catsy_copy(field, value, output, capacity, required);
    }
    return catsy_release(storage, result, 0x27, required);
}

#endif

/* -------------------------------------------------------------------------------------------------
 * System identification
 * ------------------------------------------------------------------------------------------------- */

CATALYST_RESULT catsy_platform_get(CATSYLIB_FIELD field, UTF8W output,
    NUINT capacity, NUINT* required) {
    CONTEXT_CODE context = (CONTEXT_CODE) field;
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
                return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x1B, 0);
            }
            if (translated == 1) return catsy_copy(field, "arm64", output, capacity, required);
        } else if (errno != ENOENT) {
            return RESULT(catsy_errno_status(errno), context, 0x1C, 0);
        }
    }
#endif
    struct utsname system;
    if (uname(&system) != 0) return RESULT(catsy_errno_status(errno), context, 0x19, 0);
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
        default: return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, context, 0x1A, 0);
    }
}

#endif /* TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN */
