#ifndef ATHDVLIB_HPP
#define ATHDVLIB_HPP
#include "catalyst.hpp"
#include "athdvlib.h"

namespace catalyst {
namespace device {

inline RESULT devices_enumerate(ATHDVLIB_DEVICES** devices) {
    return ::athdv_devices_enumerate(devices);
}

inline RESULT devices_free(ATHDVLIB_DEVICES* devices) {
    return ::athdv_devices_free(devices);
}

inline NUINT devices_get_count(const ATHDVLIB_DEVICES* devices) {
    return ::athdv_devices_get_count(devices);
}

inline const ATHDVLIB_DEVICE* devices_get(const ATHDVLIB_DEVICES* devices, NUINT index) {
    return ::athdv_devices_get(devices, index);
}

inline ATHDVLIB_DEVICE_TYPE device_get_type(const ATHDVLIB_DEVICE* device) {
    return ::athdv_device_get_type(device);
}

inline RESULT device_get_identifier(const ATHDVLIB_DEVICE* device, ATHDVLIB_IDENTIFIER* identifier) {
    return ::athdv_device_get_identifier(device, identifier);
}

inline RESULT device_get_hwid_count(const ATHDVLIB_DEVICE* device, NUINT* count) {
    return ::athdv_device_get_hwid_count(device, count);
}

inline RESULT device_get_hwid(const ATHDVLIB_DEVICE* device, NUINT index, ATHDVLIB_IDENTIFIER* hwid) {
    return ::athdv_device_get_hwid(device, index, hwid);
}

inline RESULT device_get_serial(const ATHDVLIB_DEVICE* device, UTF8* serial) {
    return ::athdv_device_get_serial(device, serial);
}

inline RESULT device_get_manufacturer(const ATHDVLIB_DEVICE* device, UTF8* manufacturer) {
    return ::athdv_device_get_manufacturer(device, manufacturer);
}

inline RESULT device_get_product(const ATHDVLIB_DEVICE* device, UTF8* product) {
    return ::athdv_device_get_product(device, product);
}

inline RESULT device_get_descriptor(const ATHDVLIB_DEVICE* device, UTF8* descriptor) {
    return ::athdv_device_get_descriptor(device, descriptor);
}

inline RESULT device_get_revision(const ATHDVLIB_DEVICE* device, UTF8* revision) {
    return ::athdv_device_get_revision(device, revision);
}

inline RESULT device_has_input(const ATHDVLIB_DEVICE* device, BOOL* supported) {
    return ::athdv_device_has_input(device, supported);
}

inline RESULT device_has_output(const ATHDVLIB_DEVICE* device, BOOL* supported) {
    return ::athdv_device_has_output(device, supported);
}

inline BOOL device_is_display(const ATHDVLIB_DEVICE* device) {
    return ::athdv_device_is_display(device);
}

inline RESULT display_get_edid(const ATHDVLIB_DEVICE* device, const BYTE** bytes, NUINT* size) {
    return ::athdv_display_get_edid(device, bytes, size);
}

inline RESULT display_get_native_handle(const ATHDVLIB_DEVICE* device, NUINT* handle) {
    return ::athdv_display_get_native_handle(device, handle);
}

inline RESULT devices_find_identifier(const ATHDVLIB_DEVICES* devices, const ATHDVLIB_IDENTIFIER* identifier, const ATHDVLIB_DEVICE** device) {
    return ::athdv_devices_find_identifier(devices, identifier, device);
}

inline RESULT devices_find_native_display(const ATHDVLIB_DEVICES* devices, NUINT handle, const ATHDVLIB_DEVICE** device) {
    return ::athdv_devices_find_native_display(devices, handle, device);
}

} /* namespace */
} /* namespace catalyst */
#endif /* ATHDVLIB_HPP */
