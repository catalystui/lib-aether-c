#include "athdvlib.native.hpp"

CATALYST_RESULT athdv_devices_free(ATHDVLIB_DEVICES* devices) {
    if (devices->data) {
        for (catalyst::NUINT j = 0; j < devices->count; ++j) {
            catalyst::RESULT released = athdv_display_release(&devices->data[j].display);
            if (!CATALYST_STATUS_CODE_IS_SUCCESS(released.status)) return catalyst::RESULT(released.status, 0x02, 0x04, released.detail);
        }
        catalyst::RESULT r = catmm_free(devices->data);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x02, 0x01, r.detail);
        devices->data = 0; devices->count = 0; devices->capacity = 0;
    }
    catalyst::RESULT r = catmm_free(devices);
    return catalyst::RESULT(r.status, 0x02, 0x02, r.detail);
}

CATALYST_NUINT athdv_devices_get_count(const ATHDVLIB_DEVICES* devices) {
    return devices->count;
}

const ATHDVLIB_DEVICE* athdv_devices_get(const ATHDVLIB_DEVICES* devices, CATALYST_NUINT index) {
    return &devices->data[index];
}

ATHDVLIB_DEVICE_TYPE athdv_device_get_type(const ATHDVLIB_DEVICE* device) {
    return device->type;
}

CATALYST_RESULT athdv_device_get_identifier(const ATHDVLIB_DEVICE* device, ATHDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = 0; identifier->value = 0;
    if (device->identifier.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->identifier.status, 0x06, 0x01, 0);
    identifier->scheme = device->scheme; identifier->value = device->identifier.value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x06, 0x02, 0);
}

CATALYST_RESULT athdv_device_get_hwid_count(const ATHDVLIB_DEVICE* device, CATALYST_NUINT* count) {
    *count = 0;
    if (device->hwid_status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->hwid_status, 0x07, 0x01, 0);
    *count = device->hwid_count;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x07, 0x02, 0);
}

CATALYST_RESULT athdv_device_get_hwid(const ATHDVLIB_DEVICE* device, CATALYST_NUINT index, ATHDVLIB_IDENTIFIER* hwid) {
    hwid->scheme = device->hwid_scheme; hwid->value = device->hwids[index].value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x08, 0x01, 0);
}

CATALYST_RESULT athdv_device_get_serial(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* serial) {
    *serial = 0;
    if (device->serial.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->serial.status, 0x09, 0x01, 0);
    *serial = device->serial.value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x09, 0x02, 0);
}

CATALYST_RESULT athdv_device_get_manufacturer(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* manufacturer) {
    *manufacturer = 0;
    if (device->manufacturer.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->manufacturer.status, 0x0A, 0x01, 0);
    *manufacturer = device->manufacturer.value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x0A, 0x02, 0);
}

CATALYST_RESULT athdv_device_get_product(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* product) {
    *product = 0;
    if (device->product.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->product.status, 0x0B, 0x01, 0);
    *product = device->product.value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x0B, 0x02, 0);
}

CATALYST_RESULT athdv_device_get_descriptor(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* descriptor) {
    *descriptor = 0;
    if (device->descriptor.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->descriptor.status, 0x0C, 0x01, 0);
    *descriptor = device->descriptor.value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x0C, 0x02, 0);
}

CATALYST_RESULT athdv_device_get_revision(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* revision) {
    *revision = 0;
    if (device->revision.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->revision.status, 0x0D, 0x01, 0);
    *revision = device->revision.value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x0D, 0x02, 0);
}

CATALYST_RESULT athdv_device_has_input(const ATHDVLIB_DEVICE* device, CATALYST_BOOL* supported) {
    *supported = CATALYST_FALSE;
    if (device->capability_status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->capability_status, 0x0E, 0x02, 0);
    *supported = device->input;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x0E, 0x03, 0);
}

CATALYST_RESULT athdv_device_has_output(const ATHDVLIB_DEVICE* device, CATALYST_BOOL* supported) {
    *supported = CATALYST_FALSE;
    if (device->capability_status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->capability_status, 0x0F, 0x02, 0);
    *supported = device->output;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x0F, 0x03, 0);
}


CATALYST_BOOL athdv_device_is_display(const ATHDVLIB_DEVICE* device) {
    return device->display.present;
}

CATALYST_RESULT athdv_display_get_edid(const ATHDVLIB_DEVICE* device, const CATALYST_BYTE** bytes, CATALYST_NUINT* size) {
    *bytes = 0; *size = 0;
    if (!device->display.present) return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED, 0x10, 0x01, 0);
    if (device->display.edid_status != CATALYST_STATUS_CODE_SUCCESS)
        return catalyst::RESULT(device->display.edid_status, 0x10, 0x02, 0);
    *bytes = device->display.bytes; *size = device->display.size;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x10, 0x03, 0);
}

CATALYST_RESULT athdv_display_get_native_handle(const ATHDVLIB_DEVICE* device, CATALYST_NUINT* handle) {
    *handle = 0;
    if (!device->display.present) return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED, 0x11, 0x01, 0);
    if (device->display.handle_status != CATALYST_STATUS_CODE_SUCCESS)
        return catalyst::RESULT(device->display.handle_status, 0x11, 0x02, 0);
    *handle = device->display.handle;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x11, 0x03, 0);
}
