#include "cmakecfg.h"
#if TARGET_PLATFORM_WIN32
#include "catsylib.internal.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wchar.h>

using catalyst::RESULT;
using catalyst::CONTEXT_CODE;
using catalyst::STATUS_CODE;
using catalyst::UTF8W;
using catalyst::NUINT;

/* -------------------------------------------------------------------------------------------------
 * Native text retrieval
 * ------------------------------------------------------------------------------------------------- */

static STATUS_CODE catsy_win32_status(DWORD error) {
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

static CATALYST_RESULT catsy_copy(CATSYLIB_FIELD field, const WCHAR* value,
    UTF8W output, NUINT capacity, NUINT* required) {
    CONTEXT_CODE context = (CONTEXT_CODE) field;
    if (*value == 0) return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_FOUND, context, 0x46, 0);
    int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, 0, 0, 0, 0);
    if (length <= 0) return RESULT(catsy_win32_status(GetLastError()), context, 0x48, 0);
    if ((SIZE_T) length > CATALYST_NUINT_MAX) {
        return RESULT(CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x47, 0);
    }
    *required = (NUINT) length;
    if (output == 0) return RESULT(CATALYST_STATUS_CODE_SUCCESS, context, 0x41, 0);
    if (capacity < *required) return RESULT(CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x43, 0);
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, -1,
        (char*) output, length, 0, 0) != length) {
        *required = 0;
        return RESULT(catsy_win32_status(GetLastError()), context, 0x4A, 0);
    }
    return RESULT(CATALYST_STATUS_CODE_SUCCESS, context, 0x42, 0);
}

/* -------------------------------------------------------------------------------------------------
 * Native system identification
 * ------------------------------------------------------------------------------------------------- */

/* RtlGetVersion reports the running NT version independently of application manifests. */
static STATUS_CODE catsy_version(OSVERSIONINFOEXW* version) {
    typedef LONG (WINAPI *VersionFunction)(OSVERSIONINFOW*);
    HMODULE module = GetModuleHandleW(L"ntdll.dll");
    if (module == 0) return catsy_win32_status(GetLastError());
    VersionFunction function = reinterpret_cast<VersionFunction>(GetProcAddress(module, "RtlGetVersion"));
    if (function == 0) return CATALYST_STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
    ZeroMemory(version, sizeof(*version));
    version->dwOSVersionInfoSize = sizeof(*version);
    return function((OSVERSIONINFOW*) version) == 0 ? CATALYST_STATUS_CODE_SUCCESS : CATALYST_STATUS_CODE_ERROR_IO_ERROR;
}

/* Append a DWORD in decimal; caller provides space for ten digits. */
static WCHAR* catsy_decimal(WCHAR* destination, DWORD value) {
    WCHAR reversed[10];
    CATALYST_BYTE count = 0;
    do {
        reversed[count++] = (WCHAR) (L'0' + value % 10);
        value /= 10;
    } while (value != 0);
    while (count != 0) *destination++ = reversed[--count];
    return destination;
}

static CATALYST_RESULT catsy_kernel_version(CATSYLIB_FIELD field, UTF8W output,
    NUINT capacity, NUINT* required) {
    OSVERSIONINFOEXW version;
    STATUS_CODE status = catsy_version(&version);
    if (status != CATALYST_STATUS_CODE_SUCCESS) {
        return RESULT(status, (CONTEXT_CODE) field, 0x50, 0);
    }
    WCHAR value[33]; /* Three DWORDs, two separators, terminator. */
    WCHAR* end = catsy_decimal(value, version.dwMajorVersion);
    *end++ = L'.';
    end = catsy_decimal(end, version.dwMinorVersion);
    *end++ = L'.';
    end = catsy_decimal(end, version.dwBuildNumber);
    *end = 0;
    return catsy_copy(field, value, output, capacity, required);
}

static CATALYST_RESULT catsy_registry(CATSYLIB_FIELD field, UTF8W output,
    NUINT capacity, NUINT* required) {
    CONTEXT_CODE context = (CONTEXT_CODE) field;
    HKEY key = 0;
    LONG error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0,
        KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key);
    if (error != ERROR_SUCCESS) return RESULT(catsy_win32_status((DWORD) error), context, 0x51, 0);

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
        return RESULT(catsy_win32_status((DWORD) error), context, 0x52, 0);
    }
    if (type != REG_SZ || bytes == 0 || bytes % sizeof(WCHAR) != 0) {
        RegCloseKey(key);
        return RESULT(CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x53, 0);
    }
    if ((SIZE_T) bytes > (SIZE_T) -1 - sizeof(WCHAR)) {
        RegCloseKey(key);
        return RESULT(CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x54, 0);
    }
    DWORD allocated = bytes;
    WCHAR* value = (WCHAR*) HeapAlloc(GetProcessHeap(), 0, (SIZE_T) bytes + sizeof(WCHAR));
    if (value == 0) {
        RegCloseKey(key);
        return RESULT(CATALYST_STATUS_CODE_ERROR_ALLOCATION_FAILED, context, 0x55, 0);
    }
    error = RegQueryValueExW(key, name, 0, &type, (BYTE*) value, &bytes);
    LONG closeError = RegCloseKey(key);
    if (error != ERROR_SUCCESS || closeError != ERROR_SUCCESS) {
        HeapFree(GetProcessHeap(), 0, value);
        return RESULT(catsy_win32_status((DWORD) (error != ERROR_SUCCESS ? error : closeError)), context, 0x56, 0);
    }
    if (type != REG_SZ || bytes > allocated || bytes % sizeof(WCHAR) != 0) {
        HeapFree(GetProcessHeap(), 0, value);
        return RESULT(CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x57, 0);
    }
    SIZE_T units = bytes / sizeof(WCHAR);
    value[units] = 0; /* Registry strings are not guaranteed terminated. */
    for (SIZE_T i = 0; i < units; ++i) {
        if (value[i] == 0 && i + 1 != units) {
            HeapFree(GetProcessHeap(), 0, value);
            return RESULT(CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT, context, 0x5F, 0);
        }
    }
    if (field == CATSYLIB_FIELD_DISTRIBUTION) {
        /* Windows 11 can retain a Windows 10 ProductName for compatibility.
         * Only correct that prefix for workstation builds known as Windows 11. */
        OSVERSIONINFOEXW version;
        if (wcsncmp(value, L"Windows 10", 10) == 0 &&
            catsy_version(&version) == CATALYST_STATUS_CODE_SUCCESS && version.wProductType == VER_NT_WORKSTATION &&
            version.dwMajorVersion == 10 && version.dwBuildNumber >= 22000) {
            value[9] = L'1';
        }
    }
    CATALYST_RESULT result = catsy_copy(field, value, output, capacity, required);
    if (!HeapFree(GetProcessHeap(), 0, value) && (CATALYST_STATUS_CODE_IS_SUCCESS(result.status) || (result.status == CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW && result.operation == 0x43))) {
        *required = 0;
        return RESULT(CATALYST_STATUS_CODE_ERROR_INVALID_STATE, context, 0x60, 0);
    }
    return result;
}

static CATALYST_RESULT catsy_hostname(CATSYLIB_FIELD field, UTF8W output,
    NUINT capacity, NUINT* required) {
    CONTEXT_CODE context = (CONTEXT_CODE) field;
    DWORD length = 0;
    if (!GetComputerNameExW(ComputerNameDnsHostname, 0, &length)) {
        DWORD error = GetLastError();
        if (error != ERROR_MORE_DATA) return RESULT(catsy_win32_status(error), context, 0x58, 0);
    }
    if (length == 0) return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_FOUND, context, 0x59, 0);
    if ((SIZE_T) length > (SIZE_T) -1 / sizeof(WCHAR)) {
        return RESULT(CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x5A, 0);
    }
    WCHAR* value = (WCHAR*) HeapAlloc(GetProcessHeap(), 0, (SIZE_T) length * sizeof(WCHAR));
    if (value == 0) return RESULT(CATALYST_STATUS_CODE_ERROR_ALLOCATION_FAILED, context, 0x5B, 0);
    if (!GetComputerNameExW(ComputerNameDnsHostname, value, &length)) {
        DWORD error = GetLastError();
        HeapFree(GetProcessHeap(), 0, value);
        return RESULT(catsy_win32_status(error), context, 0x5C, 0);
    }
    CATALYST_RESULT result = catsy_copy(field, value, output, capacity, required);
    if (!HeapFree(GetProcessHeap(), 0, value) && (CATALYST_STATUS_CODE_IS_SUCCESS(result.status) || (result.status == CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW && result.operation == 0x43))) {
        *required = 0;
        return RESULT(CATALYST_STATUS_CODE_ERROR_INVALID_STATE, context, 0x61, 0);
    }
    return result;
}

static const WCHAR* catsy_architecture(STATUS_CODE* status) {
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
            case 0x014C: return L"x86";      /* IMAGE_FILE_MACHINE_I386 */
            case 0x8664: return L"x86_64";   /* IMAGE_FILE_MACHINE_AMD64 */
            case 0x01C0:
            case 0x01C4: return L"arm";      /* ARM / ARMNT */
            case 0xAA64: return L"arm64";    /* IMAGE_FILE_MACHINE_ARM64 */
            case 0x0200: return L"ia64";     /* IMAGE_FILE_MACHINE_IA64 */
            default: return 0;
        }
    }
    SYSTEM_INFO information;
    GetNativeSystemInfo(&information);
    switch (information.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_INTEL: return L"x86";
        case PROCESSOR_ARCHITECTURE_AMD64: return L"x86_64";
        case PROCESSOR_ARCHITECTURE_ARM: return L"arm";
        case 12: return L"arm64"; /* PROCESSOR_ARCHITECTURE_ARM64 */
        case PROCESSOR_ARCHITECTURE_IA64: return L"ia64";
        default: return 0;
    }
}

/* -------------------------------------------------------------------------------------------------
 * System identification
 * ------------------------------------------------------------------------------------------------- */

CATALYST_RESULT catsy_platform_get(CATSYLIB_FIELD field, UTF8W output,
    NUINT capacity, NUINT* required) {
    switch (field) {
        case CATSYLIB_FIELD_NAME: return catsy_copy(field, L"Windows NT", output, capacity, required);
        case CATSYLIB_FIELD_VERSION: return catsy_kernel_version(field, output, capacity, required);
        case CATSYLIB_FIELD_DISTRIBUTION:
        case CATSYLIB_FIELD_DISTRIBUTION_VERSION: return catsy_registry(field, output, capacity, required);
        case CATSYLIB_FIELD_HOSTNAME: return catsy_hostname(field, output, capacity, required);
        case CATSYLIB_FIELD_ARCHITECTURE: {
            STATUS_CODE status;
            const WCHAR* architecture = catsy_architecture(&status);
            if (architecture != 0) return catsy_copy(field, architecture, output, capacity, required);
            return RESULT(status, (CONTEXT_CODE) field, 0x5D, 0);
        }
        default: return RESULT(CATALYST_STATUS_CODE_FATAL_INVARIANT_VIOLATION, (CONTEXT_CODE) field, 0x5E, 0);
    }
}

#endif /* TARGET_PLATFORM_WIN32 */
