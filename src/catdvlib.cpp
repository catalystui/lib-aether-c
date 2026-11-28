#include "catdvlib.h"
#include "catalyst.hpp"

/* Native backend not yet implemented on this target. No resources are created. */

CATALYST_RESULT catdv_devices_enumerate(CATDVLIB_DEVICES** devices) {
    *devices = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x01, 0xF0, 0);
}

CATALYST_RESULT catdv_devices_free(CATDVLIB_DEVICES* devices) {
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x02, 0xF0, 0);
}

CATALYST_NUINT catdv_devices_get_count(const CATDVLIB_DEVICES* devices) {
    return 0;
}

const CATDVLIB_DEVICE* catdv_devices_get(const CATDVLIB_DEVICES* devices, CATALYST_NUINT index) {
    return 0;
}

CATDVLIB_DEVICE_TYPE catdv_device_get_type(const CATDVLIB_DEVICE* device) {
    return CATDVLIB_DEVICE_UNKNOWN;
}

CATALYST_RESULT catdv_device_get_identifier(const CATDVLIB_DEVICE* device, CATDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = 0; identifier->value = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x06, 0xF0, 0);
}

CATALYST_RESULT catdv_device_get_hwid_count(const CATDVLIB_DEVICE* device, CATALYST_NUINT* count) {
    *count = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x07, 0xF0, 0);
}

CATALYST_RESULT catdv_device_get_hwid(const CATDVLIB_DEVICE* device, CATALYST_NUINT index, CATDVLIB_IDENTIFIER* hwid) {
    hwid->scheme = 0; hwid->value = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x08, 0xF0, 0);
}

CATALYST_RESULT catdv_device_get_serial(const CATDVLIB_DEVICE* device, CATALYST_UTF8* serial) {
    *serial = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x09, 0xF0, 0);
}

CATALYST_RESULT catdv_device_get_manufacturer(const CATDVLIB_DEVICE* device, CATALYST_UTF8* manufacturer) {
    *manufacturer = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0A, 0xF0, 0);
}

CATALYST_RESULT catdv_device_get_product(const CATDVLIB_DEVICE* device, CATALYST_UTF8* product) {
    *product = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0B, 0xF0, 0);
}

CATALYST_RESULT catdv_device_get_descriptor(const CATDVLIB_DEVICE* device, CATALYST_UTF8* descriptor) {
    *descriptor = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0C, 0xF0, 0);
}

CATALYST_RESULT catdv_device_get_revision(const CATDVLIB_DEVICE* device, CATALYST_UTF8* revision) {
    *revision = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0D, 0xF0, 0);
}

CATALYST_RESULT catdv_device_has_input(const CATDVLIB_DEVICE* device, CATALYST_BOOL* supported) {
    *supported = CATALYST_FALSE;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0E, 0xF0, 0);
}

CATALYST_RESULT catdv_device_has_output(const CATDVLIB_DEVICE* device, CATALYST_BOOL* supported) {
    *supported = CATALYST_FALSE;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0F, 0xF0, 0);
}

