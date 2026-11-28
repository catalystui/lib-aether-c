#include "catdvlib.native.hpp"

CATALYST_RESULT catdv_devices_free(CATDVLIB_DEVICES* devices) {
    if (devices->data) {
        for (catalyst::NUINT j = 0; j < devices->count; ++j) {
            catalyst::RESULT released = catdv_display_release(&devices->data[j].display);
            if (!CATALYST_STATUS_CODE_IS_SUCCESS(released.status)) return catalyst::RESULT(released.status, 0x02, 0x04, released.detail);
        }
        catalyst::RESULT r = catmm_free(devices->data);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x02, 0x01, r.detail);
        devices->data = 0; devices->count = 0; devices->capacity = 0;
    }
    catalyst::RESULT r = catmm_free(devices);
    return catalyst::RESULT(r.status, 0x02, 0x02, r.detail);
}

CATALYST_NUINT catdv_devices_get_count(const CATDVLIB_DEVICES* devices) {
    return devices->count;
}

const CATDVLIB_DEVICE* catdv_devices_get(const CATDVLIB_DEVICES* devices, CATALYST_NUINT index) {
    return &devices->data[index];
}

CATDVLIB_DEVICE_TYPE catdv_device_get_type(const CATDVLIB_DEVICE* device) {
    return device->type;
}

CATALYST_RESULT catdv_device_get_identifier(const CATDVLIB_DEVICE* device, CATDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = 0; identifier->value = 0;
    if (device->identifier.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->identifier.status, 0x06, 0x01, 0);
    identifier->scheme = device->scheme; identifier->value = device->identifier.value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x06, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_hwid_count(const CATDVLIB_DEVICE* device, CATALYST_NUINT* count) {
    *count = 0;
    if (device->hwid_status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->hwid_status, 0x07, 0x01, 0);
    *count = device->hwid_count;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x07, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_hwid(const CATDVLIB_DEVICE* device, CATALYST_NUINT index, CATDVLIB_IDENTIFIER* hwid) {
    hwid->scheme = device->hwid_scheme; hwid->value = device->hwids[index].value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x08, 0x01, 0);
}

CATALYST_RESULT catdv_device_get_serial(const CATDVLIB_DEVICE* device, CATALYST_UTF8* serial) {
    *serial = 0;
    if (device->serial.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->serial.status, 0x09, 0x01, 0);
    *serial = device->serial.value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x09, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_manufacturer(const CATDVLIB_DEVICE* device, CATALYST_UTF8* manufacturer) {
    *manufacturer = 0;
    if (device->manufacturer.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->manufacturer.status, 0x0A, 0x01, 0);
    *manufacturer = device->manufacturer.value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x0A, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_product(const CATDVLIB_DEVICE* device, CATALYST_UTF8* product) {
    *product = 0;
    if (device->product.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->product.status, 0x0B, 0x01, 0);
    *product = device->product.value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x0B, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_descriptor(const CATDVLIB_DEVICE* device, CATALYST_UTF8* descriptor) {
    *descriptor = 0;
    if (device->descriptor.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->descriptor.status, 0x0C, 0x01, 0);
    *descriptor = device->descriptor.value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x0C, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_revision(const CATDVLIB_DEVICE* device, CATALYST_UTF8* revision) {
    *revision = 0;
    if (device->revision.status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->revision.status, 0x0D, 0x01, 0);
    *revision = device->revision.value;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x0D, 0x02, 0);
}

CATALYST_RESULT catdv_device_has_input(const CATDVLIB_DEVICE* device, CATALYST_BOOL* supported) {
    *supported = CATALYST_FALSE;
    if (device->capability_status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->capability_status, 0x0E, 0x02, 0);
    *supported = device->input;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x0E, 0x03, 0);
}

CATALYST_RESULT catdv_device_has_output(const CATDVLIB_DEVICE* device, CATALYST_BOOL* supported) {
    *supported = CATALYST_FALSE;
    if (device->capability_status != CATALYST_STATUS_CODE_SUCCESS) return catalyst::RESULT(device->capability_status, 0x0F, 0x02, 0);
    *supported = device->output;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x0F, 0x03, 0);
}


CATALYST_BOOL catdv_device_is_display(const CATDVLIB_DEVICE* device) {
    return device->display.present;
}

CATALYST_RESULT catdv_display_get_edid(const CATDVLIB_DEVICE* device, const CATALYST_BYTE** bytes, CATALYST_NUINT* size) {
    *bytes = 0; *size = 0;
    if (!device->display.present) return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED, 0x10, 0x01, 0);
    if (device->display.edid_status != CATALYST_STATUS_CODE_SUCCESS)
        return catalyst::RESULT(device->display.edid_status, 0x10, 0x02, 0);
    *bytes = device->display.bytes; *size = device->display.size;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x10, 0x03, 0);
}

CATALYST_RESULT catdv_display_get_native_handle(const CATDVLIB_DEVICE* device, CATALYST_NUINT* handle) {
    *handle = 0;
    if (!device->display.present) return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED, 0x11, 0x01, 0);
    if (device->display.handle_status != CATALYST_STATUS_CODE_SUCCESS)
        return catalyst::RESULT(device->display.handle_status, 0x11, 0x02, 0);
    *handle = device->display.handle;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x11, 0x03, 0);
}
