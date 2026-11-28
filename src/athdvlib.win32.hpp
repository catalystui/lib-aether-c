#ifndef ATHDVLIB_WIN32_HPP
#define ATHDVLIB_WIN32_HPP
#include "athdvlib.h"
#include "catalyst.hpp"
#include "catmmlib.h"
#include "cattelib.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
extern "C" {
#include <hidsdi.h>
#include <hidpi.h>
}

static inline catalyst::STATUS_CODE athdv_win_error(DWORD e) {
    switch (e) {
        case ERROR_ACCESS_DENIED: case ERROR_SHARING_VIOLATION: return CATALYST_STATUS_CODE_ERROR_ACCESS_DENIED;
        case ERROR_FILE_NOT_FOUND: case ERROR_PATH_NOT_FOUND: case ERROR_DEVICE_NOT_CONNECTED: return CATALYST_STATUS_CODE_ERROR_NOT_FOUND;
        case ERROR_OPERATION_ABORTED: return CATALYST_STATUS_CODE_ERROR_INTERRUPTED;
        case ERROR_NOT_ENOUGH_MEMORY: case ERROR_OUTOFMEMORY: return CATALYST_STATUS_CODE_ERROR_ALLOCATION_FAILED;
        case ERROR_INSUFFICIENT_BUFFER: case ERROR_MORE_DATA: return CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW;
        case ERROR_NOT_SUPPORTED: case ERROR_INVALID_FUNCTION: return CATALYST_STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
        default: return CATALYST_STATUS_CODE_ERROR_IO_ERROR;
    }
}

/* Windows WCHAR is a native UTF-16 unit; translate explicitly through Core. */
static inline catalyst::STATUS_CODE athdv_win_utf8(const WCHAR* value, catalyst::NUINT units, CATALYST_UTF8W output, catalyst::NUINT capacity) {
    catalyst::NUINT used = 0;
    for (catalyst::NUINT j = 0; j < units && value[j];) {
        CATALYST_UTF16LE_CODE_UNIT u[2];
        catmm_fill(u, 0, sizeof(u));
        u[0].lsb = (CATALYST_BYTE) value[j];
        u[0].msb = (CATALYST_BYTE) (value[j] >> 8);
        if (j + 1 < units) {
            u[1].lsb = (CATALYST_BYTE) value[j + 1];
            u[1].msb = (CATALYST_BYTE) (value[j + 1] >> 8);
        }
        CATALYST_CODEPOINT cp;
        catalyst::NUINT consumed, produced;
        catalyst::RESULT r = catte_utf16le_decode(u, &cp, &consumed);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status) || consumed > units - j) return CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT;
        CATALYST_UTF8_CODE_UNIT bytes[4];
        r = catte_utf8_encode(cp, bytes, &produced);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return r.status;
        if (produced >= capacity - used) return CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW;
        catmm_copy(output + used, bytes, produced);
        used += produced; j += consumed;
        if (j == units) return CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT;
    }
    output[used] = 0;
    return used ? CATALYST_STATUS_CODE_SUCCESS : CATALYST_STATUS_CODE_ERROR_NOT_FOUND;
}

static inline catalyst::STATUS_CODE athdv_win_caps(HANDLE handle, HIDP_CAPS* caps) {
    PHIDP_PREPARSED_DATA parsed = 0;
    if (!HidD_GetPreparsedData(handle, &parsed)) return athdv_win_error(GetLastError());
    NTSTATUS status = HidP_GetCaps(parsed, caps);
    if (!HidD_FreePreparsedData(parsed)) return CATALYST_STATUS_CODE_ERROR_IO_ERROR;
    return status == HIDP_STATUS_SUCCESS ? CATALYST_STATUS_CODE_SUCCESS : CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT;
}

#endif
