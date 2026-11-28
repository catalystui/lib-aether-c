#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif
#include "cmakecfg.h"
#if TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN
#include "catsylib.internal.hpp"
#include "catmmlib.h"
#include <sys/utsname.h>
#include <errno.h>
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/* Validate native byte strings as UTF-8 using the system's own converter.
 * No locale changes, replacement characters, or public allocation. */
static CATALYST_RESULT catsy_copy(CATSYLIB_FIELD field, const char* value,
    UTF8W output, NUINT capacity, NUINT* required) {
    CONTEXT_CODE context = (CONTEXT_CODE) field;
    size_t length = strlen(value);
    if (length == 0) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, context, 0x06, 0);
    if (length >= CATALYST_NUINT_MAX) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x07, 0);
    }

    iconv_t converter = iconv_open("UTF-8", "UTF-8");
    if (converter == (iconv_t) -1) {
        return RESULT(catsy_errno_status(errno), context, 0x08, 0);
    }
    char* input = const_cast<char*>(value);
    size_t remaining = length;
    int error = 0;
    while (remaining != 0) {
        char scratch[128];
        char* destination = scratch;
        size_t available = sizeof(scratch);
        if (iconv(converter, &input, &remaining, &destination, &available) == (size_t) -1) {
            if (errno == E2BIG) continue;
            error = errno;
            break;
        }
    }
    if (iconv_close(converter) != 0 && error == 0) {
        return RESULT(catsy_errno_status(errno), context, 0x09, 0);
    }
    if (error != 0) {
        return RESULT(error == EILSEQ || error == EINVAL
            ? STATUS_CODE_ERROR_MALFORMED_INPUT : catsy_errno_status(error), context, 0x0A, 0);
    }

    *required = (NUINT) (length + 1);
    if (output == 0) return RESULT(STATUS_CODE_SUCCESS, context, 0x01, 0);
    if (capacity < *required) return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x03, 0);
    catmm_copy(output, value, *required);
    return RESULT(STATUS_CODE_SUCCESS, context, 0x02, 0);
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
    FILE* file = fopen("/etc/os-release", "r");
    if (file == 0 && errno == ENOENT) file = fopen("/usr/lib/os-release", "r");
    if (file == 0) return RESULT(catsy_errno_status(errno), context, 0x10, 0);

    const char* key = field == CATSYLIB_FIELD_DISTRIBUTION ? "NAME" : "VERSION_ID";
    size_t keyLength = strlen(key);
    char* line = 0;
    size_t allocated = 0;
    ssize_t count;
    char* found = 0;
    STATUS_CODE status = STATUS_CODE_SUCCESS;
    for (;;) {
        errno = 0;
        count = getline(&line, &allocated, file);
        if (count < 0) {
            if (!feof(file) || ferror(file)) status = catsy_errno_status(errno);
            break;
        }
        if ((size_t) count != strlen(line)) {
            status = STATUS_CODE_ERROR_MALFORMED_INPUT;
            break;
        }
        while (count > 0 && (line[count - 1] == '\n' || line[count - 1] == '\r')) {
            line[--count] = 0;
        }
        if (strncmp(line, key, keyLength) != 0 || line[keyLength] != '=') continue;
        char* value = line + keyLength + 1;
        if (!catsy_os_release_value(value)) {
            status = STATUS_CODE_ERROR_MALFORMED_INPUT;
            break;
        }
        /* The last assignment wins, as specified by os-release. */
        char* replacement = strdup(value);
        if (replacement == 0) {
            status = STATUS_CODE_ERROR_ALLOCATION_FAILED;
            break;
        }
        free(found);
        found = replacement;
    }
    free(line);
    int closeError = fclose(file);
    int closeStatus = errno;
    if (status != STATUS_CODE_SUCCESS) {
        free(found);
        return RESULT(status, context, 0x11, 0);
    }
    if (closeError != 0) {
        free(found);
        return RESULT(catsy_errno_status(closeStatus), context, 0x12, 0);
    }
    if (found == 0) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, context, 0x13, 0);
    CATALYST_RESULT result = catsy_copy(field, found, output, capacity, required);
    free(found);
    return result;
}
#endif

#if TARGET_PLATFORM_DARWIN
/* -------------------------------------------------------------------------------------------------
 * Darwin distribution identification
 * ------------------------------------------------------------------------------------------------- */

static CATALYST_RESULT catsy_distribution_version(CATSYLIB_FIELD field, UTF8W output,
    NUINT capacity, NUINT* required) {
    CONTEXT_CODE context = (CONTEXT_CODE) field;
    size_t length = 0;
    if (sysctlbyname("kern.osproductversion", 0, &length, 0, 0) != 0) {
        return RESULT(errno == ENOENT ? STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED
            : catsy_errno_status(errno), context, 0x14, 0);
    }
    if (length == 0) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, context, 0x15, 0);
    size_t allocated = length;
    char* value = (char*) malloc(length);
    if (value == 0) return RESULT(STATUS_CODE_ERROR_ALLOCATION_FAILED, context, 0x16, 0);
    if (sysctlbyname("kern.osproductversion", value, &length, 0, 0) != 0) {
        int error = errno;
        free(value);
        return RESULT(catsy_errno_status(error), context, 0x17, 0);
    }
    if (length == 0 || length > allocated || value[length - 1] != 0 || strlen(value) != length - 1) {
        free(value);
        return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x18, 0);
    }
    CATALYST_RESULT result = catsy_copy(field, value, output, capacity, required);
    free(value);
    return result;
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
            if (strcmp(system.machine, "aarch64") == 0) {
                return catsy_copy(field, "arm64", output, capacity, required);
            }
            if (strcmp(system.machine, "amd64") == 0) {
                return catsy_copy(field, "x86_64", output, capacity, required);
            }
            if (strcmp(system.machine, "i386") == 0 || strcmp(system.machine, "i486") == 0 ||
                strcmp(system.machine, "i586") == 0 || strcmp(system.machine, "i686") == 0) {
                return catsy_copy(field, "x86", output, capacity, required);
            }
            return catsy_copy(field, system.machine, output, capacity, required);
        default: return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, context, 0x1A, 0);
    }
}

#endif /* TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN */
