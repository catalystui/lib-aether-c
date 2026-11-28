#ifndef CATDVLIB_HPP
#define CATDVLIB_HPP
#include "catalyst.hpp"
#include "catdvlib.h"

namespace catalyst {
namespace device {

inline RESULT devices_enumerate(CATDVLIB_DEVICES** devices) {
    return ::catdv_devices_enumerate(devices);
}

inline RESULT devices_free(CATDVLIB_DEVICES* devices) {
    return ::catdv_devices_free(devices);
}

inline NUINT devices_get_count(const CATDVLIB_DEVICES* devices) {
    return ::catdv_devices_get_count(devices);
}

inline const CATDVLIB_DEVICE* devices_get(const CATDVLIB_DEVICES* devices, NUINT index) {
    return ::catdv_devices_get(devices, index);
}

inline CATDVLIB_DEVICE_TYPE device_get_type(const CATDVLIB_DEVICE* device) {
    return ::catdv_device_get_type(device);
}

inline RESULT device_get_identifier(const CATDVLIB_DEVICE* device, CATDVLIB_IDENTIFIER* identifier) {
    return ::catdv_device_get_identifier(device, identifier);
}

inline RESULT device_get_hwid_count(const CATDVLIB_DEVICE* device, NUINT* count) {
    return ::catdv_device_get_hwid_count(device, count);
}

inline RESULT device_get_hwid(const CATDVLIB_DEVICE* device, NUINT index, CATDVLIB_IDENTIFIER* hwid) {
    return ::catdv_device_get_hwid(device, index, hwid);
}

inline RESULT device_get_serial(const CATDVLIB_DEVICE* device, UTF8* serial) {
    return ::catdv_device_get_serial(device, serial);
}

inline RESULT device_get_manufacturer(const CATDVLIB_DEVICE* device, UTF8* manufacturer) {
    return ::catdv_device_get_manufacturer(device, manufacturer);
}

inline RESULT device_get_product(const CATDVLIB_DEVICE* device, UTF8* product) {
    return ::catdv_device_get_product(device, product);
}

inline RESULT device_get_descriptor(const CATDVLIB_DEVICE* device, UTF8* descriptor) {
    return ::catdv_device_get_descriptor(device, descriptor);
}

inline RESULT device_get_revision(const CATDVLIB_DEVICE* device, UTF8* revision) {
    return ::catdv_device_get_revision(device, revision);
}

inline RESULT device_has_input(const CATDVLIB_DEVICE* device, BOOL* supported) {
    return ::catdv_device_has_input(device, supported);
}

inline RESULT device_has_output(const CATDVLIB_DEVICE* device, BOOL* supported) {
    return ::catdv_device_has_output(device, supported);
}

inline BOOL device_is_display(const CATDVLIB_DEVICE* device) {
    return ::catdv_device_is_display(device);
}

inline RESULT display_get_edid(const CATDVLIB_DEVICE* device, const BYTE** bytes, NUINT* size) {
    return ::catdv_display_get_edid(device, bytes, size);
}

inline RESULT display_get_native_handle(const CATDVLIB_DEVICE* device, NUINT* handle) {
    return ::catdv_display_get_native_handle(device, handle);
}

inline RESULT devices_find_identifier(const CATDVLIB_DEVICES* devices, const CATDVLIB_IDENTIFIER* identifier, const CATDVLIB_DEVICE** device) {
    return ::catdv_devices_find_identifier(devices, identifier, device);
}

inline RESULT devices_find_native_display(const CATDVLIB_DEVICES* devices, NUINT handle, const CATDVLIB_DEVICE** device) {
    return ::catdv_devices_find_native_display(devices, handle, device);
}

} /* namespace */
} /* namespace catalyst */
#endif /* CATDVLIB_HPP */
