#include "athdvlib.native.hpp"
#include "athdvlib.win32.hpp"
#include <setupapi.h>

static void athdv_win_string(const WCHAR* value, catalyst::NUINT units, ATHDVLIB_NATIVE_TEXT* text) {
    text->status = athdv_win_utf8(value, units, text->value, sizeof(text->value));
}
static void athdv_win_property(HDEVINFO list, SP_DEVINFO_DATA* info, DWORD property, ATHDVLIB_NATIVE_TEXT* text) {
    WCHAR value[1024]; DWORD type = 0, bytes = 0;
    catmm_fill(value, 0, sizeof(value));
    if (!SetupDiGetDeviceRegistryPropertyW(list, info, property, &type, (PBYTE) value, sizeof(value), &bytes)) {
        DWORD e = GetLastError();
        text->status = e == ERROR_INVALID_DATA ? CATALYST_STATUS_CODE_ERROR_NOT_FOUND : athdv_win_error(e);
        return;
    }
    if (type != REG_SZ || bytes % sizeof(WCHAR) || bytes > sizeof(value)) { text->status = CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT; return; }
    athdv_win_string(value, bytes / sizeof(WCHAR), text);
}
static void athdv_win_metadata(HDEVINFO list, SP_DEVINFO_DATA* info, ATHDVLIB_DEVICE* d) {
    athdv_win_property(list, info, SPDRP_MFG, &d->manufacturer);
    athdv_win_property(list, info, SPDRP_DEVICEDESC, &d->product);
    athdv_win_property(list, info, SPDRP_FRIENDLYNAME, &d->descriptor);
    if (d->descriptor.status == CATALYST_STATUS_CODE_ERROR_NOT_FOUND) d->descriptor = d->product;
    WCHAR ids[4096]; DWORD type = 0, bytes = 0;
    d->hwid_scheme = (CATALYST_ASCII) "windows-pnp";
    if (!SetupDiGetDeviceRegistryPropertyW(list, info, SPDRP_HARDWAREID, &type, (PBYTE) ids, sizeof(ids), &bytes)) {
        DWORD e = GetLastError();
        if (e != ERROR_INVALID_DATA) d->hwid_status = athdv_win_error(e);
        return;
    }
    if (type != REG_MULTI_SZ || bytes % sizeof(WCHAR) || bytes > sizeof(ids)) { d->hwid_status = CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT; return; }
    catalyst::NUINT extent = bytes / sizeof(WCHAR), pos = 0;
    while (pos < extent && ids[pos]) {
        catalyst::NUINT end = pos;
        while (end < extent && ids[end]) ++end;
        if (end == extent) { d->hwid_status = CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT; return; }
        if (d->hwid_count == 16) { d->hwid_status = CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW; return; }
        ATHDVLIB_NATIVE_TEXT* text = &d->hwids[d->hwid_count];
        athdv_win_string(ids + pos, end - pos + 1, text);
        if (text->status != CATALYST_STATUS_CODE_SUCCESS) { d->hwid_status = text->status; return; }
        ++d->hwid_count; pos = end + 1;
    }
    if (pos == extent) d->hwid_status = CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT;
}
static void athdv_win_hid(HANDLE handle, ATHDVLIB_DEVICE* d) {
    HIDP_CAPS caps;
    d->capability_status = athdv_win_caps(handle, &caps);
    if (d->capability_status == CATALYST_STATUS_CODE_SUCCESS) {
        d->input = caps.InputReportByteLength ? CATALYST_TRUE : CATALYST_FALSE;
        d->output = caps.OutputReportByteLength ? CATALYST_TRUE : CATALYST_FALSE;
    }
    WCHAR value[256];
    catmm_fill(value, 0, sizeof(value));
    if (HidD_GetManufacturerString(handle, value, sizeof(value))) athdv_win_string(value, 256, &d->manufacturer);
    catmm_fill(value, 0, sizeof(value));
    if (HidD_GetProductString(handle, value, sizeof(value))) athdv_win_string(value, 256, &d->product);
    catmm_fill(value, 0, sizeof(value));
    if (HidD_GetSerialNumberString(handle, value, sizeof(value))) athdv_win_string(value, 256, &d->serial);
}
static catalyst::STATUS_CODE athdv_win_scan(ATHDVLIB_DEVICES* snapshot, CATALYST_BOOL hid) {
    GUID guid;
    HidD_GetHidGuid(&guid);
    HDEVINFO list = SetupDiGetClassDevsW(hid ? &guid : 0, 0, 0, DIGCF_PRESENT |
        (hid ? DIGCF_DEVICEINTERFACE : DIGCF_ALLCLASSES));
    if (list == INVALID_HANDLE_VALUE) return athdv_win_error(GetLastError());
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
                DWORD e = GetLastError(); if (e != ERROR_NO_MORE_ITEMS) status = athdv_win_error(e); break;
            }
            catmm_fill(&storage, 0, sizeof(storage)); storage.detail.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            if (!SetupDiGetDeviceInterfaceDetailW(list, &iface, &storage.detail, sizeof(storage), 0, &info)) {
                status = athdv_win_error(GetLastError()); break;
            }
            identity = storage.detail.DevicePath;
            units = (sizeof(storage) - ((const CATALYST_BYTE*) identity - storage.bytes)) / sizeof(WCHAR);
        } else {
            if (!SetupDiEnumDeviceInfo(list, index, &info)) {
                DWORD e = GetLastError(); if (e != ERROR_NO_MORE_ITEMS) status = athdv_win_error(e); break;
            }
            if (!SetupDiGetDeviceInstanceIdW(list, &info, instance, 1024, 0)) { status = athdv_win_error(GetLastError()); break; }
        }
        ATHDVLIB_DEVICE* d;
        catalyst::RESULT r = athdv_native_add(snapshot, &d);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) { status = r.status; break; }
        d->scheme = (CATALYST_ASCII) (hid ? "windows-hid" : "windows-pnp");
        d->type = hid ? ATHDVLIB_DEVICE_LOGICAL : ATHDVLIB_DEVICE_UNKNOWN;
        athdv_win_string(identity, units, &d->identifier);
        if (d->identifier.status != CATALYST_STATUS_CODE_SUCCESS) { status = d->identifier.status; break; }
        athdv_win_metadata(list, &info, d);
        if (hid) {
            HANDLE handle = CreateFileW(identity, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
            if (handle == INVALID_HANDLE_VALUE) d->capability_status = athdv_win_error(GetLastError());
            else { athdv_win_hid(handle, d); if (!CloseHandle(handle)) { status = athdv_win_error(GetLastError()); break; } }
        }
    }
    if (!SetupDiDestroyDeviceInfoList(list) && status == CATALYST_STATUS_CODE_SUCCESS) status = athdv_win_error(GetLastError());
    return status;
}
/* Resolve the exact monitor interface path, not a vendor/product substring. */
static catalyst::STATUS_CODE athdv_win_edid(HKEY key, ATHDVLIB_DISPLAY* display) {
    DWORD type = 0, size = 0;
    LSTATUS error = RegQueryValueExW(key, L"EDID", 0, &type, 0, &size);
    if (error != ERROR_SUCCESS) return athdv_win_error((DWORD) error);
    if (type != REG_BINARY) return CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT;
    if (!size) return CATALYST_STATUS_CODE_ERROR_NOT_FOUND;
    CATMMLIB_POINTERW memory = 0;
    catalyst::RESULT r = catmm_alloc(&memory, (catalyst::NUINT) size);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return r.status;
    display->bytes = (CATALYST_BYTE*) memory; /* Owned even if retrieval fails. */
    DWORD actual = size;
    error = RegQueryValueExW(key, L"EDID", 0, &type, (PBYTE) memory, &actual);
    if (error != ERROR_SUCCESS) return athdv_win_error((DWORD) error);
    if (type != REG_BINARY || actual > size) return CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT;
    if (!actual) return CATALYST_STATUS_CODE_ERROR_NOT_FOUND;
    display->size = (catalyst::NUINT) actual;
    return CATALYST_STATUS_CODE_SUCCESS;
}

static void athdv_win_monitor_metadata(const WCHAR* path, ATHDVLIB_DEVICE* d) {
    HDEVINFO list = SetupDiCreateDeviceInfoList(0, 0);
    if (list == INVALID_HANDLE_VALUE) { d->display.edid_status = athdv_win_error(GetLastError()); return; }
    SP_DEVICE_INTERFACE_DATA iface;
    SP_DEVINFO_DATA info;
    catmm_fill(&iface, 0, sizeof(iface)); iface.cbSize = sizeof(iface);
    catmm_fill(&info, 0, sizeof(info)); info.cbSize = sizeof(info);
    union { SP_DEVICE_INTERFACE_DETAIL_DATA_W detail; CATALYST_BYTE bytes[8192]; } storage;
    catmm_fill(&storage, 0, sizeof(storage)); storage.detail.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
    if (!SetupDiOpenDeviceInterfaceW(list, path, 0, &iface) ||
        !SetupDiGetDeviceInterfaceDetailW(list, &iface, &storage.detail, sizeof(storage), 0, &info))
        d->display.edid_status = athdv_win_error(GetLastError());
    else {
        athdv_win_metadata(list, &info, d);
        HKEY key = SetupDiOpenDevRegKey(list, &info, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
        if (key == INVALID_HANDLE_VALUE) d->display.edid_status = athdv_win_error(GetLastError());
        else {
            d->display.edid_status = athdv_win_edid(key, &d->display);
            LSTATUS closed = RegCloseKey(key);
            if (closed != ERROR_SUCCESS && d->display.edid_status == CATALYST_STATUS_CODE_SUCCESS)
                d->display.edid_status = athdv_win_error((DWORD) closed);
        }
    }
    if (!SetupDiDestroyDeviceInfoList(list) && d->display.edid_status == CATALYST_STATUS_CODE_SUCCESS)
        d->display.edid_status = athdv_win_error(GetLastError());
}

struct ATHDVLIB_WIN_DISPLAYS {
    ATHDVLIB_DEVICES* snapshot;
    catalyst::STATUS_CODE status;
};

static catalyst::STATUS_CODE athdv_win_display_entry(ATHDVLIB_DEVICES* snapshot, HMONITOR handle, const WCHAR* identity,
    catalyst::NUINT units, const DISPLAY_DEVICEW* monitor) {
    ATHDVLIB_DEVICE* d;
    catalyst::RESULT r = athdv_native_add(snapshot, &d);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return r.status;
    athdv_display_init(&d->display);
    d->display.handle = (catalyst::NUINT) handle;
    d->display.handle_status = CATALYST_STATUS_CODE_SUCCESS;
    d->type = ATHDVLIB_DEVICE_LOGICAL;
    d->scheme = (CATALYST_ASCII) (monitor ? "windows-monitor" : "windows-display-source");
    athdv_win_string(identity, units, &d->identifier);
    if (d->identifier.status != CATALYST_STATUS_CODE_SUCCESS) return d->identifier.status;
    d->capability_status = CATALYST_STATUS_CODE_SUCCESS;
    d->output = CATALYST_TRUE;
    if (monitor) {
        athdv_win_monitor_metadata(identity, d);
        if (d->descriptor.status == CATALYST_STATUS_CODE_ERROR_NOT_FOUND)
            athdv_win_string(monitor->DeviceString, sizeof(monitor->DeviceString) / sizeof(WCHAR), &d->descriptor);
    }
    return CATALYST_STATUS_CODE_SUCCESS;
}

static BOOL CALLBACK athdv_win_monitor(HMONITOR handle, HDC, LPRECT, LPARAM context) {
    ATHDVLIB_WIN_DISPLAYS* scan = (ATHDVLIB_WIN_DISPLAYS*) context;
    MONITORINFOEXW info;
    catmm_fill(&info, 0, sizeof(info)); info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(handle, (LPMONITORINFO) &info)) {
        scan->status = athdv_win_error(GetLastError()); return FALSE;
    }
    bool found = false;
    for (DWORD index = 0; ; ++index) {
        DISPLAY_DEVICEW monitor;
        catmm_fill(&monitor, 0, sizeof(monitor)); monitor.cb = sizeof(monitor);
        if (!EnumDisplayDevicesW(info.szDevice, index, &monitor, EDD_GET_DEVICE_INTERFACE_NAME)) break;
        if (!(monitor.StateFlags & DISPLAY_DEVICE_ACTIVE) || !monitor.DeviceID[0]) continue;
        found = true;
        scan->status = athdv_win_display_entry(scan->snapshot, handle, monitor.DeviceID,
            sizeof(monitor.DeviceID) / sizeof(WCHAR), &monitor);
        if (scan->status != CATALYST_STATUS_CODE_SUCCESS) return FALSE;
    }
    /* Virtual displays may have no monitor interface or EDID. Preserve source identity. */
    if (!found) scan->status = athdv_win_display_entry(scan->snapshot, handle, info.szDevice,
        sizeof(info.szDevice) / sizeof(WCHAR), 0);
    return scan->status == CATALYST_STATUS_CODE_SUCCESS ? TRUE : FALSE;
}

static catalyst::STATUS_CODE athdv_win_displays(ATHDVLIB_DEVICES* snapshot) {
    ATHDVLIB_WIN_DISPLAYS scan = { snapshot, CATALYST_STATUS_CODE_SUCCESS };
    if (!EnumDisplayMonitors(0, 0, athdv_win_monitor, (LPARAM) &scan) && scan.status == CATALYST_STATUS_CODE_SUCCESS)
        return CATALYST_STATUS_CODE_ERROR_IO_ERROR;
    return scan.status;
}

CATALYST_RESULT athdv_devices_enumerate(ATHDVLIB_DEVICES** devices) {
    *devices = 0;
    CATMMLIB_POINTERW memory = 0;
    catalyst::RESULT r = catmm_alloc(&memory, sizeof(ATHDVLIB_DEVICES));
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x01, 0x01, r.detail);
    ATHDVLIB_DEVICES* snapshot = (ATHDVLIB_DEVICES*) memory;
    catalyst::STATUS_CODE status = athdv_win_scan(snapshot, CATALYST_FALSE);
    if (status == CATALYST_STATUS_CODE_SUCCESS) status = athdv_win_scan(snapshot, CATALYST_TRUE);
    if (status == CATALYST_STATUS_CODE_SUCCESS) status = athdv_win_displays(snapshot);
    if (status != CATALYST_STATUS_CODE_SUCCESS) {
        r = athdv_devices_free(snapshot);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x01, 0x02, r.detail);
        return catalyst::RESULT(status, 0x01, 0x03, 0);
    }
    *devices = snapshot;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x01, 0x06, 0);
}
