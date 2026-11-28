#include "catdvlib.h"
#include "catalyst.hpp"
#include "cmakecfg.h"

static catalyst::BOOL catdv_identifier_equal(const CATALYST_BYTE* a, const CATALYST_BYTE* b) {
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b ? CATALYST_TRUE : CATALYST_FALSE;
}

CATALYST_RESULT catdv_devices_find_identifier(const CATDVLIB_DEVICES* devices, const CATDVLIB_IDENTIFIER* identifier, const CATDVLIB_DEVICE** device) {
    *device = 0;
    const CATDVLIB_DEVICE* match = 0;
    for (catalyst::NUINT j = 0; j < catdv_devices_get_count(devices); ++j) {
        const CATDVLIB_DEVICE* entry = catdv_devices_get(devices, j);
        CATDVLIB_IDENTIFIER id;
        catalyst::RESULT r = catdv_device_get_identifier(entry, &id);
        if (r.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(r.status, 0x12, 0x01, r.detail);
        if (!catdv_identifier_equal((const CATALYST_BYTE*) id.scheme, (const CATALYST_BYTE*) identifier->scheme) ||
            !catdv_identifier_equal((const CATALYST_BYTE*) id.value, (const CATALYST_BYTE*) identifier->value)) continue;
        if (match) return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_INVALID_STATE, 0x12, 0x02, 0);
        match = entry;
    }
    if (!match) return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_NOT_FOUND, 0x12, 0x03, 0);
    *device = match;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x12, 0x04, 0);
}

CATALYST_RESULT catdv_devices_find_native_display(const CATDVLIB_DEVICES* devices, CATALYST_NUINT handle, const CATDVLIB_DEVICE** device) {
    *device = 0;
#if TARGET_PLATFORM_WIN32 || TARGET_PLATFORM_DARWIN
    const CATDVLIB_DEVICE* match = 0;
    for (catalyst::NUINT j = 0; j < catdv_devices_get_count(devices); ++j) {
        const CATDVLIB_DEVICE* entry = catdv_devices_get(devices, j);
        if (!catdv_device_is_display(entry)) continue;
        catalyst::NUINT native = 0;
        catalyst::RESULT r = catdv_display_get_native_handle(entry, &native);
        if (r.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(r.status, 0x13, 0x01, r.detail);
        if (native != handle) continue;
        if (match) return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_INVALID_STATE, 0x13, 0x02, 0);
        match = entry;
    }
    if (!match) return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_NOT_FOUND, 0x13, 0x03, 0);
    *device = match;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x13, 0x04, 0);
#else
    return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x13, 0x05, 0);
#endif
}
