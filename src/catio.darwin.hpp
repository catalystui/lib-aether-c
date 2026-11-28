#ifndef CATIO_DARWIN_HPP
#define CATIO_DARWIN_HPP
#include "catdvlib.h"
#include "catalyst.hpp"
#include "catmmlib.h"
#include "cattelib.h"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>

/* Native Boolean macros cannot be namespace-qualified. */
#undef TRUE
#undef FALSE

#if !CATALYST_SUPPORTS_32BIT || !CATALYST_SUPPORTS_64BIT
#error "Darwin HID requires 32-bit HID values and 64-bit registry entry identifiers."
#endif

static inline catalyst::STATUS_CODE catio_mac_status(IOReturn status) {
    switch (status) {
        case kIOReturnSuccess: return catalyst::STATUS_CODE_SUCCESS;
        case kIOReturnNotPrivileged: case kIOReturnNotPermitted: case kIOReturnExclusiveAccess: return catalyst::STATUS_CODE_ERROR_ACCESS_DENIED;
        case kIOReturnNoDevice: case kIOReturnNotFound: return catalyst::STATUS_CODE_ERROR_NOT_FOUND;
        case kIOReturnNoMemory: return catalyst::STATUS_CODE_ERROR_ALLOCATION_FAILED;
        case kIOReturnUnsupported: return catalyst::STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
        case kIOReturnTimeout: return catalyst::STATUS_CODE_ERROR_TIMEOUT;
        case kIOReturnAborted: return catalyst::STATUS_CODE_ERROR_INTERRUPTED;
        default: return catalyst::STATUS_CODE_ERROR_IO_ERROR;
    }
}

static inline void catio_mac_hex(CATALYST_UTF8W value, CATALYST_ULONG number, catalyst::BYTE digits) {
    const char* alphabet = "0123456789ABCDEF";
    for (catalyst::BYTE j = digits; j != 0;) { --j; value[j] = (catalyst::BYTE) alphabet[number & 15]; number >>= 4; }
    value[digits] = 0;
}
static inline catalyst::BOOL catio_mac_unhex(CATALYST_UTF8 value, CATALYST_ULONG* number) {
    *number = 0;
    for (catalyst::BYTE j = 0; j < 16; ++j) {
        catalyst::BYTE digit = value[j];
        if (digit >= '0' && digit <= '9') digit = (catalyst::BYTE) (digit - '0');
        else if (digit >= 'A' && digit <= 'F') digit = (catalyst::BYTE) (digit - 'A' + 10);
        else return catalyst::FALSE;
        *number = (*number << 4) | digit;
    }
    return value[16] == 0 ? catalyst::TRUE : catalyst::FALSE;
}
static inline catalyst::STATUS_CODE catio_mac_open(const CATDVLIB_DEVICE* device, IOHIDDeviceRef* result) {
    *result = 0;
    CATDVLIB_IDENTIFIER id;
    catalyst::RESULT r = catdv_device_get_identifier(device, &id);
    if (!catalyst::status_code_is_success(r.status)) return r.status;
    if (catte_ascii_length(id.scheme) != 10 || catmm_compare(id.scheme, "darwin-hid", 10) != 0)
        return catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED;
    CATALYST_ULONG number;
    if (!catio_mac_unhex(id.value, &number)) return catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT;
    CFMutableDictionaryRef matching = IORegistryEntryIDMatching((uint64_t) number);
    if (!matching) return catalyst::STATUS_CODE_ERROR_ALLOCATION_FAILED;
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, matching);
    if (!service) return catalyst::STATUS_CODE_ERROR_NOT_FOUND;
    IOHIDDeviceRef hid = IOHIDDeviceCreate(kCFAllocatorDefault, service);
    IOObjectRelease(service);
    if (!hid) return catalyst::STATUS_CODE_ERROR_IO_ERROR;
    IOReturn opened = IOHIDDeviceOpen(hid, kIOHIDOptionsTypeNone);
    if (opened != kIOReturnSuccess) { CFRelease(hid); return catio_mac_status(opened); }
    *result = hid;
    return catalyst::STATUS_CODE_SUCCESS;
}
struct CATIO_MAC_CHANNEL {
    IOHIDElementRef element;
    catalyst::BYTE type; /* 1 digital, 2 absolute, 3 relative */
    CATALYST_UTF8_CODE_UNIT identifier[17];
};
struct CATIO_MAC_RESOURCE {
    IOHIDDeviceRef device;
    CFArrayRef elements;
    CATIO_MAC_CHANNEL* channels;
    catalyst::NUINT count;
};
/* Direction and scalar representability are separate properties. Unknown
 * channels are native elements whose values cannot be interpreted as scalars. */
static inline catalyst::BOOL catio_mac_direction(IOHIDElementRef e, catalyst::BOOL input) {
    IOHIDElementType type = IOHIDElementGetType(e);
    if (!input) return type == kIOHIDElementTypeOutput ? catalyst::TRUE : catalyst::FALSE;
    return type == kIOHIDElementTypeInput_Misc || type == kIOHIDElementTypeInput_Button ||
        type == kIOHIDElementTypeInput_Axis || type == kIOHIDElementTypeInput_ScanCodes ? catalyst::TRUE : catalyst::FALSE;
}
static inline catalyst::BYTE catio_mac_type(IOHIDElementRef e, catalyst::BOOL input) {
    if (!catio_mac_direction(e, input)) return 0;
    if (IOHIDElementIsArray(e) || IOHIDElementGetReportCount(e) != 1 ||
        IOHIDElementGetReportSize(e) == 0 || IOHIDElementGetReportSize(e) > 32) return 0;
    CFIndex low = IOHIDElementGetLogicalMin(e), high = IOHIDElementGetLogicalMax(e);
    if (low < CATALYST_INT_MIN || high > CATALYST_INT_MAX || low > high) return 0;
    if (IOHIDElementIsRelative(e)) return input ? 3 : 0;
    return low == 0 && high == 1 ? 1 : 2;
}
static inline catalyst::STATUS_CODE catio_mac_channels(CATIO_MAC_RESOURCE* io, catalyst::BOOL input) {
    io->elements = IOHIDDeviceCopyMatchingElements(io->device, 0, kIOHIDOptionsTypeNone);
    if (!io->elements) return catalyst::STATUS_CODE_ERROR_NOT_FOUND;
    CFIndex count = CFArrayGetCount(io->elements);
    if (count < 0 || (CATALYST_ULONG) count > CATALYST_NUINT_MAX / sizeof(CATIO_MAC_CHANNEL))
        return catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW;
    if (!count) return catalyst::STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
    CATMMLIB_POINTERW memory = 0;
    catalyst::RESULT r = catmm_alloc(&memory, (catalyst::NUINT) count * sizeof(CATIO_MAC_CHANNEL));
    if (!catalyst::status_code_is_success(r.status)) return r.status;
    io->channels = (CATIO_MAC_CHANNEL*) memory;
    for (CFIndex j = 0; j < count; ++j) {
        IOHIDElementRef element = (IOHIDElementRef) CFArrayGetValueAtIndex(io->elements, j);
        if (!catio_mac_direction(element, input)) continue;
        catalyst::BYTE type = catio_mac_type(element, input);
        CATIO_MAC_CHANNEL* c = &io->channels[io->count++];
        c->element = element; c->type = type;
        catio_mac_hex(c->identifier, (CATALYST_ULONG) IOHIDElementGetCookie(element), 16);
    }
    return io->count ? catalyst::STATUS_CODE_SUCCESS : catalyst::STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
}
static inline catalyst::STATUS_CODE catio_mac_close(CATIO_MAC_RESOURCE* io) {
    if (io->device) {
        IOReturn closed = IOHIDDeviceClose(io->device, kIOHIDOptionsTypeNone);
        if (closed != kIOReturnSuccess && closed != kIOReturnNoDevice) return catio_mac_status(closed);
        CFRelease(io->device); io->device = 0;
    }
    if (io->elements) { CFRelease(io->elements); io->elements = 0; }
    if (io->channels) {
        catalyst::RESULT r = catmm_free(io->channels);
        if (!catalyst::status_code_is_success(r.status)) return r.status;
        io->channels = 0;
    }
    return catalyst::STATUS_CODE_SUCCESS;
}
static inline catalyst::STATUS_CODE catio_mac_value(const CATIO_MAC_RESOURCE* io, catalyst::NUINT channel, CATALYST_INT* value) {
    IOHIDValueRef v = 0;
    IOReturn r = IOHIDDeviceGetValue(io->device, io->channels[channel].element, &v);
    if (r != kIOReturnSuccess) return catio_mac_status(r);
    if (!v) return catalyst::STATUS_CODE_ERROR_NOT_FOUND;
    CFIndex n = IOHIDValueGetIntegerValue(v);
    if (n < CATALYST_INT_MIN || n > CATALYST_INT_MAX) return catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT;
    *value = (CATALYST_INT) n;
    return catalyst::STATUS_CODE_SUCCESS;
}
#endif
