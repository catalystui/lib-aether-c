#include "catdvlib.native.hpp"
#include "catio.win32.hpp"
#include <setupapi.h>

static void catdv_win_string(const WCHAR* value, catalyst::NUINT units, CATDVLIB_NATIVE_TEXT* text) {
    text->status = catio_win_utf8(value, units, text->value, sizeof(text->value));
}
static void catdv_win_property(HDEVINFO list, SP_DEVINFO_DATA* info, DWORD property, CATDVLIB_NATIVE_TEXT* text) {
    WCHAR value[1024]; DWORD type = 0, bytes = 0;
    catmm_fill(value, 0, sizeof(value));
    if (!SetupDiGetDeviceRegistryPropertyW(list, info, property, &type, (PBYTE) value, sizeof(value), &bytes)) {
        DWORD e = GetLastError();
        text->status = e == ERROR_INVALID_DATA ? CATALYST_STATUS_CODE_ERROR_NOT_FOUND : catio_win_error(e);
        return;
    }
    if (type != REG_SZ || bytes % sizeof(WCHAR) || bytes > sizeof(value)) { text->status = CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT; return; }
    catdv_win_string(value, bytes / sizeof(WCHAR), text);
}
static void catdv_win_metadata(HDEVINFO list, SP_DEVINFO_DATA* info, CATDVLIB_DEVICE* d) {
    catdv_win_property(list, info, SPDRP_MFG, &d->manufacturer);
    catdv_win_property(list, info, SPDRP_DEVICEDESC, &d->product);
    catdv_win_property(list, info, SPDRP_FRIENDLYNAME, &d->descriptor);
    if (d->descriptor.status == CATALYST_STATUS_CODE_ERROR_NOT_FOUND) d->descriptor = d->product;
    WCHAR ids[4096]; DWORD type = 0, bytes = 0;
    d->hwid_scheme = (CATALYST_ASCII) "windows-pnp";
    if (!SetupDiGetDeviceRegistryPropertyW(list, info, SPDRP_HARDWAREID, &type, (PBYTE) ids, sizeof(ids), &bytes)) {
        DWORD e = GetLastError();
        if (e != ERROR_INVALID_DATA) d->hwid_status = catio_win_error(e);
        return;
    }
    if (type != REG_MULTI_SZ || bytes % sizeof(WCHAR) || bytes > sizeof(ids)) { d->hwid_status = CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT; return; }
    catalyst::NUINT extent = bytes / sizeof(WCHAR), pos = 0;
    while (pos < extent && ids[pos]) {
        catalyst::NUINT end = pos;
        while (end < extent && ids[end]) ++end;
        if (end == extent) { d->hwid_status = CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT; return; }
        if (d->hwid_count == 16) { d->hwid_status = CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW; return; }
        CATDVLIB_NATIVE_TEXT* text = &d->hwids[d->hwid_count];
        catdv_win_string(ids + pos, end - pos + 1, text);
        if (text->status != CATALYST_STATUS_CODE_SUCCESS) { d->hwid_status = text->status; return; }
        ++d->hwid_count; pos = end + 1;
    }
    if (pos == extent) d->hwid_status = CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT;
}
static void catdv_win_hid(HANDLE handle, CATDVLIB_DEVICE* d) {
    HIDP_CAPS caps;
    d->capability_status = catio_win_caps(handle, &caps);
    if (d->capability_status == CATALYST_STATUS_CODE_SUCCESS) {
        d->input = caps.InputReportByteLength ? CATALYST_TRUE : CATALYST_FALSE;
        d->output = caps.OutputReportByteLength ? CATALYST_TRUE : CATALYST_FALSE;
    }
    WCHAR value[256];
    catmm_fill(value, 0, sizeof(value));
    if (HidD_GetManufacturerString(handle, value, sizeof(value))) catdv_win_string(value, 256, &d->manufacturer);
    catmm_fill(value, 0, sizeof(value));
    if (HidD_GetProductString(handle, value, sizeof(value))) catdv_win_string(value, 256, &d->product);
    catmm_fill(value, 0, sizeof(value));
    if (HidD_GetSerialNumberString(handle, value, sizeof(value))) catdv_win_string(value, 256, &d->serial);
}
static catalyst::STATUS_CODE catdv_win_scan(CATDVLIB_DEVICES* snapshot, CATALYST_BOOL hid) {
    GUID guid;
    HidD_GetHidGuid(&guid);
    HDEVINFO list = SetupDiGetClassDevsW(hid ? &guid : 0, 0, 0, DIGCF_PRESENT |
        (hid ? DIGCF_DEVICEINTERFACE : DIGCF_ALLCLASSES));
    if (list == INVALID_HANDLE_VALUE) return catio_win_error(GetLastError());
    catalyst::STATUS_CODE status = CATALYST_STATUS_CODE_SUCCESS;
    for (DWORD index = 0; ; ++index) {
        SP_DEVINFO_DATA info; catmm_fill(&info, 0, sizeof(info)); info.cbSize = sizeof(info);
        WCHAR instance[1024];
        /* Aligned native variable-size detail buffer, no unbounded stack cast. */
        union { SP_DEVICE_INTERFACE_DETAIL_DATA_W detail; CATALYST_BYTE bytes[8192]; } storage;
        const WCHAR* identity = instance; catalyst::NUINT units = 1024;
        if (hid) {
            SP_DEVICE_INTERFACE_DATA iface; catmm_fill(&iface, 0, sizeof(iface)); iface.cbSize = sizeof(iface);
            if (!SetupDiEnumDeviceInterfaces(list, 0, &guid, index, &iface)) {
                DWORD e = GetLastError(); if (e != ERROR_NO_MORE_ITEMS) status = catio_win_error(e); break;
            }
            catmm_fill(&storage, 0, sizeof(storage)); storage.detail.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            if (!SetupDiGetDeviceInterfaceDetailW(list, &iface, &storage.detail, sizeof(storage), 0, &info)) {
                status = catio_win_error(GetLastError()); break;
            }
            identity = storage.detail.DevicePath;
            units = (sizeof(storage) - ((const CATALYST_BYTE*) identity - storage.bytes)) / sizeof(WCHAR);
        } else {
            if (!SetupDiEnumDeviceInfo(list, index, &info)) {
                DWORD e = GetLastError(); if (e != ERROR_NO_MORE_ITEMS) status = catio_win_error(e); break;
            }
            if (!SetupDiGetDeviceInstanceIdW(list, &info, instance, 1024, 0)) { status = catio_win_error(GetLastError()); break; }
        }
        CATDVLIB_DEVICE* d;
        catalyst::RESULT r = catdv_native_add(snapshot, &d);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) { status = r.status; break; }
        d->scheme = (CATALYST_ASCII) (hid ? "windows-hid" : "windows-pnp");
        d->type = hid ? CATDVLIB_DEVICE_LOGICAL : CATDVLIB_DEVICE_UNKNOWN;
        catdv_win_string(identity, units, &d->identifier);
        if (d->identifier.status != CATALYST_STATUS_CODE_SUCCESS) { status = d->identifier.status; break; }
        catdv_win_metadata(list, &info, d);
        if (hid) {
            HANDLE handle = CreateFileW(identity, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
            if (handle == INVALID_HANDLE_VALUE) d->capability_status = catio_win_error(GetLastError());
            else { catdv_win_hid(handle, d); if (!CloseHandle(handle)) { status = catio_win_error(GetLastError()); break; } }
        }
    }
    if (!SetupDiDestroyDeviceInfoList(list) && status == CATALYST_STATUS_CODE_SUCCESS) status = catio_win_error(GetLastError());
    return status;
}
CATALYST_RESULT catdv_devices_enumerate(CATDVLIB_DEVICES** devices) {
    *devices = 0;
    CATMMLIB_POINTERW memory = 0;
    catalyst::RESULT r = catmm_alloc(&memory, sizeof(CATDVLIB_DEVICES));
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x01, 0x01, r.detail);
    CATDVLIB_DEVICES* snapshot = (CATDVLIB_DEVICES*) memory;
    catalyst::STATUS_CODE status = catdv_win_scan(snapshot, CATALYST_FALSE);
    if (status == CATALYST_STATUS_CODE_SUCCESS) status = catdv_win_scan(snapshot, CATALYST_TRUE);
    if (status != CATALYST_STATUS_CODE_SUCCESS) {
        r = catdv_devices_free(snapshot);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x01, 0x02, r.detail);
        return catalyst::RESULT(status, 0x01, 0x03, 0);
    }
    *devices = snapshot;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x01, 0x06, 0);
}
