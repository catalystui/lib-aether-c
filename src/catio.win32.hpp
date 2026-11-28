#ifndef CATIO_WIN32_HPP
#define CATIO_WIN32_HPP
#include "catdvlib.h"
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
using catalyst::RESULT;
using catalyst::STATUS_CODE;
using catalyst::NUINT;

static inline STATUS_CODE catio_win_error(DWORD e) {
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
static inline STATUS_CODE catio_win_utf8(const WCHAR* value, NUINT units, CATALYST_UTF8W output, NUINT capacity) {
    NUINT used = 0;
    for (NUINT j = 0; j < units && value[j];) {
        CATALYST_UTF16LE_CODE_UNIT u[2];
        catmm_fill(u, 0, sizeof(u));
        u[0].lsb = (CATALYST_BYTE) value[j];
        u[0].msb = (CATALYST_BYTE) (value[j] >> 8);
        if (j + 1 < units) {
            u[1].lsb = (CATALYST_BYTE) value[j + 1];
            u[1].msb = (CATALYST_BYTE) (value[j + 1] >> 8);
        }
        CATALYST_CODEPOINT cp;
        NUINT consumed, produced;
        RESULT r = catte_utf16le_decode(u, &cp, &consumed);
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

static inline STATUS_CODE catio_win_path(const CATDVLIB_DEVICE* device, WCHAR* path, NUINT capacity) {
    CATDVLIB_IDENTIFIER id;
    RESULT r = catdv_device_get_identifier(device, &id);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return r.status;
    if (catte_ascii_length(id.scheme) != 11 || catmm_compare(id.scheme, "windows-hid", 11) != 0)
        return CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED;
    CATALYST_UTF16LE_CODE_UNIT units[1024];
    if (capacity > 1024) return CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW;
    r = catte_utf8_to_utf16le(id.value, units, capacity);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return r.status;
    for (NUINT j = 0; j < capacity; ++j) {
        path[j] = (WCHAR) ((unsigned int) units[j].lsb | ((unsigned int) units[j].msb << 8));
        if (!path[j]) return CATALYST_STATUS_CODE_SUCCESS;
    }
    return CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW;
}

static inline STATUS_CODE catio_win_caps(HANDLE handle, HIDP_CAPS* caps) {
    PHIDP_PREPARSED_DATA parsed = 0;
    if (!HidD_GetPreparsedData(handle, &parsed)) return catio_win_error(GetLastError());
    NTSTATUS status = HidP_GetCaps(parsed, caps);
    if (!HidD_FreePreparsedData(parsed)) return CATALYST_STATUS_CODE_ERROR_IO_ERROR;
    return status == HIDP_STATUS_SUCCESS ? CATALYST_STATUS_CODE_SUCCESS : CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT;
}

struct CATIO_WIN_RESOURCE {
    HANDLE handle;
    OVERLAPPED operation;
    CATALYST_BOOL pending;
    DWORD length;
    CATALYST_BYTE* buffer;
};

/* A single native operation owns its buffer until completion. */
static inline STATUS_CODE catio_win_open(const CATDVLIB_DEVICE* device, CATALYST_BOOL input, CATIO_WIN_RESOURCE* io) {
    WCHAR path[1024];
    STATUS_CODE s = catio_win_path(device, path, 1024);
    if (s != CATALYST_STATUS_CODE_SUCCESS) return s;
    HANDLE handle = CreateFileW(path, input ? GENERIC_READ : GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    if (handle == INVALID_HANDLE_VALUE) return catio_win_error(GetLastError());
    HIDP_CAPS caps;
    s = catio_win_caps(handle, &caps);
    if (s != CATALYST_STATUS_CODE_SUCCESS) { CloseHandle(handle); return s; }
    DWORD length = input ? caps.InputReportByteLength : caps.OutputReportByteLength;
    if (!length) { CloseHandle(handle); return CATALYST_STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED; }
    HANDLE event = CreateEventW(0, TRUE, FALSE, 0);
    if (!event) { s = catio_win_error(GetLastError()); CloseHandle(handle); return s; }
    CATMMLIB_POINTERW memory = 0;
    RESULT r = catmm_alloc(&memory, length);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) { CloseHandle(event); CloseHandle(handle); return r.status; }
    io->handle = handle; io->operation.hEvent = event;
    io->length = length; io->buffer = (CATALYST_BYTE*) memory; io->pending = CATALYST_FALSE;
    return CATALYST_STATUS_CODE_SUCCESS;
}

static inline STATUS_CODE catio_win_close(CATIO_WIN_RESOURCE* io) {
    if (io->handle && io->pending) {
        /* Cancellation is asynchronous: drain before releasing the OVERLAPPED storage. */
        if (!CancelIoEx(io->handle, &io->operation) && GetLastError() != ERROR_NOT_FOUND)
            return catio_win_error(GetLastError());
        DWORD count;
        if (!GetOverlappedResult(io->handle, &io->operation, &count, TRUE)) {
            DWORD error = GetLastError();
            if (error != ERROR_OPERATION_ABORTED && error != ERROR_DEVICE_NOT_CONNECTED &&
                error != ERROR_GEN_FAILURE) return catio_win_error(error);
        }
        io->pending = CATALYST_FALSE;
    }
    if (io->handle) {
        if (!CloseHandle(io->handle)) return catio_win_error(GetLastError());
        io->handle = 0;
    }
    if (io->operation.hEvent) {
        if (!CloseHandle(io->operation.hEvent)) return catio_win_error(GetLastError());
        io->operation.hEvent = 0;
    }
    if (io->buffer) {
        RESULT r = catmm_free(io->buffer);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return r.status;
        io->buffer = 0;
    }
    return CATALYST_STATUS_CODE_SUCCESS;
}

static inline STATUS_CODE catio_win_transfer(CATIO_WIN_RESOURCE* io, CATALYST_BOOL input, DWORD* count) {
    *count = 0;
    if (io->pending) {
        if (!GetOverlappedResult(io->handle, &io->operation, count, FALSE)) {
            DWORD error = GetLastError();
            if (error == ERROR_IO_INCOMPLETE) return CATALYST_STATUS_CODE_SUCCESS_NOOP;
            io->pending = CATALYST_FALSE;
            return catio_win_error(error);
        }
        io->pending = CATALYST_FALSE;
        return CATALYST_STATUS_CODE_SUCCESS;
    }
    HANDLE event = io->operation.hEvent;
    catmm_fill(&io->operation, 0, sizeof(io->operation));
    io->operation.hEvent = event;
    if (!ResetEvent(event)) return catio_win_error(GetLastError());
    BOOL ok = input ? ReadFile(io->handle, io->buffer, io->length, count, &io->operation) :
        WriteFile(io->handle, io->buffer, io->length, count, &io->operation);
    if (ok) return CATALYST_STATUS_CODE_SUCCESS;
    DWORD error = GetLastError();
    if (error != ERROR_IO_PENDING) return catio_win_error(error);
    io->pending = CATALYST_TRUE;
    return CATALYST_STATUS_CODE_SUCCESS_NOOP;
}
#endif
