#include "cmakecfg.h"
#if TARGET_PLATFORM_WIN32
#include "catsylib.internal.hpp"
#include "catalyst.hpp"
#include "catmmlib.h"
#include "cattelib.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>


/* -------------------------------------------------------------------------------------------------
 * Native text retrieval
 * ------------------------------------------------------------------------------------------------- */

static catalyst::STATUS_CODE catsy_win32_status(DWORD error) {
    switch (error) {
        case ERROR_ACCESS_DENIED: return CATALYST_STATUS_CODE_ERROR_ACCESS_DENIED;
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND: return CATALYST_STATUS_CODE_ERROR_NOT_FOUND;
        case ERROR_NOT_SUPPORTED:
        case ERROR_CALL_NOT_IMPLEMENTED:
        case ERROR_PROC_NOT_FOUND: return CATALYST_STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY: return CATALYST_STATUS_CODE_ERROR_ALLOCATION_FAILED;
        case ERROR_NO_UNICODE_TRANSLATION: return CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT;
        case ERROR_MORE_DATA: return CATALYST_STATUS_CODE_ERROR_INTERRUPTED;
        default: return CATALYST_STATUS_CODE_ERROR_IO_ERROR;
    }
}

/* Convert native UTF-16 code units explicitly to Catalyst UTF-16LE scalars.
 * With a null output, measure and validate only. Otherwise the caller has
 * already measured the same immutable native string and reserved the extent. */
static CATALYST_RESULT catsy_convert(CATSYLIB_FIELD field, const WCHAR* value,
    catalyst::UTF8W output, catalyst::NUINT* length) {
    catalyst::CONTEXT_CODE context = (catalyst::CONTEXT_CODE) field;
    catalyst::NUINT written = 0;
    while (*value != 0) {
        CATALYST_BYTE encoded[4];
        catalyst::NUINT produced = 1;
        catalyst::NUINT consumed = 1;
        if (*value < 0x80) {
            encoded[0] = (CATALYST_BYTE) *value;
        } else {
            CATALYST_UTF16LE_CODE_UNIT units[2];
            units[0].lsb = (CATALYST_BYTE) *value;
            units[0].msb = (CATALYST_BYTE) (*value >> 8);
            if (units[0].msb >= 0xD8 && units[0].msb <= 0xDB) {
                units[1].lsb = (CATALYST_BYTE) value[1];
                units[1].msb = (CATALYST_BYTE) (value[1] >> 8);
            }
            CATALYST_CODEPOINT codepoint;
            catalyst::RESULT decoded = catte_utf16le_decode(units, &codepoint, &consumed);
            if (!CATALYST_STATUS_CODE_IS_SUCCESS(decoded.status)) {
                return catalyst::RESULT(decoded.status, context, 0x48, decoded.detail);
            }
            catalyst::RESULT encodedResult = catte_utf8_encode(codepoint, encoded, &produced);
            if (!CATALYST_STATUS_CODE_IS_SUCCESS(encodedResult.status)) {
                return catalyst::RESULT(CATALYST_STATUS_CODE_FATAL_INVARIANT_VIOLATION, context, 0x4A, 0);
            }
        }
        if (produced >= CATALYST_NUINT_MAX - written) {
            return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x47, 0);
        }
        if (output != 0) {
            if (produced == 1) output[written] = encoded[0];
            else catmm_copy(output + written, encoded, produced);
        }
        written += produced;
        value += consumed;
    }
    if (output != 0) output[written] = 0;
    *length = written + (catalyst::NUINT) 1;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, context, 0x4C, 0);
}

static CATALYST_RESULT catsy_copy(CATSYLIB_FIELD field, const WCHAR* value,
    catalyst::UTF8W output, catalyst::NUINT capacity, catalyst::NUINT* required) {
    catalyst::CONTEXT_CODE context = (catalyst::CONTEXT_CODE) field;
    if (*value == 0) return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_NOT_FOUND, context, 0x46, 0);
    catalyst::NUINT length = 0;
    catalyst::RESULT measured = catsy_convert(field, value, 0, &length);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(measured.status)) return measured;
    catalyst::RESULT result = catsy_prepare_output(field, length, output, capacity, required);
    if (output == 0 || !CATALYST_STATUS_CODE_IS_SUCCESS(result.status)) return result;
    catalyst::RESULT converted = catsy_convert(field, value, output, &length);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(converted.status) || length != *required) {
        *required = 0;
        return catalyst::RESULT(CATALYST_STATUS_CODE_FATAL_INVARIANT_VIOLATION, context, 0x4B, 0);
    }
    return result;
}

/* -------------------------------------------------------------------------------------------------
 * Native system identification
 * ------------------------------------------------------------------------------------------------- */

/* RtlGetVersion reports the running NT version independently of application manifests. */
static catalyst::STATUS_CODE catsy_version(OSVERSIONINFOEXW* version) {
    typedef LONG (WINAPI *VersionFunction)(OSVERSIONINFOW*);
    HMODULE module = GetModuleHandleW(L"ntdll.dll");
    if (module == 0) return catsy_win32_status(GetLastError());
    VersionFunction function = reinterpret_cast<VersionFunction>(GetProcAddress(module, "RtlGetVersion"));
    if (function == 0) return CATALYST_STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
    catmm_fill(version, 0, (catalyst::NUINT) sizeof(*version));
    version->dwOSVersionInfoSize = sizeof(*version);
    return function((OSVERSIONINFOW*) version) == 0 ? CATALYST_STATUS_CODE_SUCCESS : CATALYST_STATUS_CODE_ERROR_IO_ERROR;
}

/* Append a DWORD in decimal; caller provides space for ten digits. */
static CATALYST_UTF8W catsy_decimal(CATALYST_UTF8W destination, DWORD value) {
    CATALYST_UTF8_CODE_UNIT reversed[10];
    CATALYST_BYTE count = 0;
    do {
        reversed[count++] = (CATALYST_UTF8_CODE_UNIT) ('0' + value % 10);
        value /= 10;
    } while (value != 0);
    while (count != 0) *destination++ = reversed[--count];
    return destination;
}

static CATALYST_RESULT catsy_kernel_version(CATSYLIB_FIELD field, catalyst::UTF8W output,
    catalyst::NUINT capacity, catalyst::NUINT* required) {
    OSVERSIONINFOEXW version;
    catalyst::STATUS_CODE status = catsy_version(&version);
    if (status != CATALYST_STATUS_CODE_SUCCESS) {
        return catalyst::RESULT(status, (catalyst::CONTEXT_CODE) field, 0x50, 0);
    }
    CATALYST_UTF8_CODE_UNIT value[33]; /* Three DWORDs, two separators, terminator. */
    CATALYST_UTF8W end = catsy_decimal(value, version.dwMajorVersion);
    *end++ = '.';
    end = catsy_decimal(end, version.dwMinorVersion);
    *end++ = '.';
    end = catsy_decimal(end, version.dwBuildNumber);
    *end = 0;
    return catsy_copy_utf8(field, value, output, capacity, required);
}

static CATALYST_RESULT catsy_registry(CATSYLIB_FIELD field, catalyst::UTF8W output,
    catalyst::NUINT capacity, catalyst::NUINT* required) {
    catalyst::CONTEXT_CODE context = (catalyst::CONTEXT_CODE) field;
    HKEY key = 0;
    LONG error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0,
        KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key);
    if (error != ERROR_SUCCESS) return catalyst::RESULT(catsy_win32_status((DWORD) error), context, 0x51, 0);

    const WCHAR* name = field == CATSYLIB_FIELD_DISTRIBUTION ? L"ProductName" : L"DisplayVersion";
    DWORD type = 0;
    DWORD bytes = 0;
    error = RegQueryValueExW(key, name, 0, &type, 0, &bytes);
    if (error == ERROR_FILE_NOT_FOUND && field == CATSYLIB_FIELD_DISTRIBUTION_VERSION) {
        name = L"ReleaseId";
        error = RegQueryValueExW(key, name, 0, &type, 0, &bytes);
    }
    if (error != ERROR_SUCCESS) {
        RegCloseKey(key);
        return catalyst::RESULT(catsy_win32_status((DWORD) error), context, 0x52, 0);
    }
    if (type != REG_SZ || bytes == 0 || bytes % sizeof(WCHAR) != 0) {
        RegCloseKey(key);
        return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x53, 0);
    }
    if ((SIZE_T) bytes > (SIZE_T) -1 - sizeof(WCHAR)) {
        RegCloseKey(key);
        return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x54, 0);
    }
    DWORD allocated = bytes;
    WCHAR local[256];
    WCHAR* value = local;
    CATMMLIB_POINTERW storage = 0;
    if ((SIZE_T) bytes + sizeof(WCHAR) > sizeof(local)) {
        catalyst::RESULT allocation = catmm_alloc(&storage, (catalyst::NUINT) ((SIZE_T) bytes + sizeof(WCHAR)));
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(allocation.status)) {
            RegCloseKey(key);
            return catalyst::RESULT(allocation.status, context, 0x55, allocation.detail);
        }
        value = (WCHAR*) storage;
    }
    error = RegQueryValueExW(key, name, 0, &type, (BYTE*) value, &bytes);
    LONG closeError = RegCloseKey(key);
    catalyst::RESULT result;
    if (error != ERROR_SUCCESS || closeError != ERROR_SUCCESS) {
        result = catalyst::RESULT(catsy_win32_status((DWORD) (error != ERROR_SUCCESS ? error : closeError)), context, 0x56, 0);
    } else if (type != REG_SZ || bytes > allocated || bytes % sizeof(WCHAR) != 0) {
        result = catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x57, 0);
    } else {
        SIZE_T units = bytes / sizeof(WCHAR);
        value[units] = 0; /* Registry strings are not guaranteed terminated. */
        SIZE_T i = 0;
        while (i < units && value[i] != 0) ++i;
        if (i < units && i + 1 != units) {
            result = catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x5F, 0);
        } else {
            if (field == CATSYLIB_FIELD_DISTRIBUTION) {
                /* Windows 11 can retain the Windows 10 ProductName. */
                OSVERSIONINFOEXW version;
                if (i >= 10 && catmm_compare(value, L"Windows 10", (catalyst::NUINT) (10 * sizeof(WCHAR))) == 0 &&
                    catsy_version(&version) == CATALYST_STATUS_CODE_SUCCESS && version.wProductType == VER_NT_WORKSTATION &&
                    version.dwMajorVersion == 10 && version.dwBuildNumber >= 22000) {
                    value[9] = L'1';
                }
            }
            result = catsy_copy(field, value, output, capacity, required);
        }
    }
    return catsy_release(storage, result, 0x60, required);
}

static CATALYST_RESULT catsy_hostname(CATSYLIB_FIELD field, catalyst::UTF8W output,
    catalyst::NUINT capacity, catalyst::NUINT* required) {
    catalyst::CONTEXT_CODE context = (catalyst::CONTEXT_CODE) field;
    WCHAR local[256];
    DWORD length = (DWORD) (sizeof(local) / sizeof(WCHAR));
    if (GetComputerNameExW(ComputerNameDnsHostname, local, &length)) {
        if (length >= sizeof(local) / sizeof(WCHAR) || local[length] != 0) {
            return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x62, 0);
        }
        for (DWORD i = 0; i < length; ++i) {
            if (local[i] == 0) return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x64, 0);
        }
        return catsy_copy(field, local, output, capacity, required);
    }
    DWORD error = GetLastError();
    if (error != ERROR_MORE_DATA) return catalyst::RESULT(catsy_win32_status(error), context, 0x58, 0);
    if (length == 0) return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_NOT_FOUND, context, 0x59, 0);
    if ((SIZE_T) length > (SIZE_T) -1 / sizeof(WCHAR)) {
        return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x5A, 0);
    }
    CATMMLIB_POINTERW storage = 0;
    catalyst::RESULT allocation = catmm_alloc(&storage, (catalyst::NUINT) ((SIZE_T) length * sizeof(WCHAR)));
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(allocation.status)) {
        return catalyst::RESULT(allocation.status, context, 0x5B, allocation.detail);
    }
    WCHAR* value = (WCHAR*) storage;
    DWORD allocated = length;
    catalyst::RESULT result;
    if (!GetComputerNameExW(ComputerNameDnsHostname, value, &length)) {
        result = catalyst::RESULT(catsy_win32_status(GetLastError()), context, 0x5C, 0);
    } else if (length >= allocated || value[length] != 0) {
        result = catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x63, 0);
    } else {
        DWORD i = 0;
        while (i < length && value[i] != 0) ++i;
        if (i != length) result = catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x65, 0);
        else result = catsy_copy(field, value, output, capacity, required);
    }
    return catsy_release(storage, result, 0x61, required);
}

static const char* catsy_architecture(catalyst::STATUS_CODE* status) {
    *status = CATALYST_STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
    /* IsWow64Process2 distinguishes the native system from the emulated process. */
    typedef BOOL (WINAPI *Wow64Function)(HANDLE, USHORT*, USHORT*);
    HMODULE module = GetModuleHandleW(L"kernel32.dll");
    Wow64Function function = module == 0 ? 0 :
        reinterpret_cast<Wow64Function>(GetProcAddress(module, "IsWow64Process2"));
    USHORT process = 0;
    USHORT native = 0;
    if (function != 0) {
        if (!function(GetCurrentProcess(), &process, &native)) {
            *status = catsy_win32_status(GetLastError());
            return 0;
        }
        switch (native) {
            case 0x014C: return "x86";      /* IMAGE_FILE_MACHINE_I386 */
            case 0x8664: return "x86_64";   /* IMAGE_FILE_MACHINE_AMD64 */
            case 0x01C0:
            case 0x01C4: return "arm";      /* ARM / ARMNT */
            case 0xAA64: return "arm64";    /* IMAGE_FILE_MACHINE_ARM64 */
            case 0x0200: return "ia64";     /* IMAGE_FILE_MACHINE_IA64 */
            default: return 0;
        }
    }
    SYSTEM_INFO information;
    GetNativeSystemInfo(&information);
    switch (information.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
        case PROCESSOR_ARCHITECTURE_AMD64: return "x86_64";
        case PROCESSOR_ARCHITECTURE_ARM: return "arm";
        case 12: return "arm64"; /* PROCESSOR_ARCHITECTURE_ARM64 */
        case PROCESSOR_ARCHITECTURE_IA64: return "ia64";
        default: return 0;
    }
}

/* -------------------------------------------------------------------------------------------------
 * System identification
 * ------------------------------------------------------------------------------------------------- */

CATALYST_RESULT catsy_platform_get(CATSYLIB_FIELD field, catalyst::UTF8W output,
    catalyst::NUINT capacity, catalyst::NUINT* required) {
    switch (field) {
        case CATSYLIB_FIELD_NAME: return catsy_copy_utf8(field, (CATALYST_UTF8) "Windows NT", output, capacity, required);
        case CATSYLIB_FIELD_VERSION: return catsy_kernel_version(field, output, capacity, required);
        case CATSYLIB_FIELD_DISTRIBUTION:
        case CATSYLIB_FIELD_DISTRIBUTION_VERSION: return catsy_registry(field, output, capacity, required);
        case CATSYLIB_FIELD_HOSTNAME: return catsy_hostname(field, output, capacity, required);
        case CATSYLIB_FIELD_ARCHITECTURE: {
            catalyst::STATUS_CODE status;
            const char* architecture = catsy_architecture(&status);
            if (architecture != 0) return catsy_copy_utf8(field, (CATALYST_UTF8) architecture, output, capacity, required);
            return catalyst::RESULT(status, (catalyst::CONTEXT_CODE) field, 0x5D, 0);
        }
        default: return catalyst::RESULT(CATALYST_STATUS_CODE_FATAL_INVARIANT_VIOLATION, (catalyst::CONTEXT_CODE) field, 0x5E, 0);
    }
}

#endif /* TARGET_PLATFORM_WIN32 */
