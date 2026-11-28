#include "athdvlib.h"
#include "catalyst.hpp"

/* Native backend not yet implemented on this target. No resources are created. */

CATALYST_RESULT athdv_devices_enumerate(ATHDVLIB_DEVICES** devices) {
    *devices = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x01, 0xF0, 0);
}

CATALYST_RESULT athdv_devices_free(ATHDVLIB_DEVICES* devices) {
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x02, 0xF0, 0);
}

CATALYST_NUINT athdv_devices_get_count(const ATHDVLIB_DEVICES* devices) {
    return 0;
}

const ATHDVLIB_DEVICE* athdv_devices_get(const ATHDVLIB_DEVICES* devices, CATALYST_NUINT index) {
    return 0;
}

ATHDVLIB_DEVICE_TYPE athdv_device_get_type(const ATHDVLIB_DEVICE* device) {
    return ATHDVLIB_DEVICE_UNKNOWN;
}

CATALYST_RESULT athdv_device_get_identifier(const ATHDVLIB_DEVICE* device, ATHDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = 0; identifier->value = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x06, 0xF0, 0);
}

CATALYST_RESULT athdv_device_get_hwid_count(const ATHDVLIB_DEVICE* device, CATALYST_NUINT* count) {
    *count = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x07, 0xF0, 0);
}

CATALYST_RESULT athdv_device_get_hwid(const ATHDVLIB_DEVICE* device, CATALYST_NUINT index, ATHDVLIB_IDENTIFIER* hwid) {
    hwid->scheme = 0; hwid->value = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x08, 0xF0, 0);
}

CATALYST_RESULT athdv_device_get_serial(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* serial) {
    *serial = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x09, 0xF0, 0);
}

CATALYST_RESULT athdv_device_get_manufacturer(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* manufacturer) {
    *manufacturer = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0A, 0xF0, 0);
}

CATALYST_RESULT athdv_device_get_product(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* product) {
    *product = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0B, 0xF0, 0);
}

CATALYST_RESULT athdv_device_get_descriptor(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* descriptor) {
    *descriptor = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0C, 0xF0, 0);
}

CATALYST_RESULT athdv_device_get_revision(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* revision) {
    *revision = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0D, 0xF0, 0);
}

CATALYST_RESULT athdv_device_has_input(const ATHDVLIB_DEVICE* device, CATALYST_BOOL* supported) {
    *supported = CATALYST_FALSE;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0E, 0xF0, 0);
}

CATALYST_RESULT athdv_device_has_output(const ATHDVLIB_DEVICE* device, CATALYST_BOOL* supported) {
    *supported = CATALYST_FALSE;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0F, 0xF0, 0);
}


CATALYST_BOOL athdv_device_is_display(const ATHDVLIB_DEVICE* device) {
    return CATALYST_FALSE;
}
CATALYST_RESULT athdv_display_get_edid(const ATHDVLIB_DEVICE* device, const CATALYST_BYTE** bytes, CATALYST_NUINT* size) {
    *bytes = 0; *size = 0;
    return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x10, 0xF0, 0);
}
CATALYST_RESULT athdv_display_get_native_handle(const ATHDVLIB_DEVICE* device, CATALYST_NUINT* handle) {
    *handle = 0;
    return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x11, 0xF0, 0);
}
