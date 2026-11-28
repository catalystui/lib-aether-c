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
using namespace catalyst;

#if !CATALYST_SUPPORTS_32BIT || !CATALYST_SUPPORTS_64BIT
#error "Darwin HID requires 32-bit HID values and 64-bit registry entry identifiers."
#endif

static inline STATUS_CODE catio_mac_status(IOReturn status) {
    switch (status) {
        case kIOReturnSuccess: return STATUS_CODE_SUCCESS;
        case kIOReturnNotPrivileged: case kIOReturnNotPermitted: case kIOReturnExclusiveAccess: return STATUS_CODE_ERROR_ACCESS_DENIED;
        case kIOReturnNoDevice: case kIOReturnNotFound: return STATUS_CODE_ERROR_NOT_FOUND;
        case kIOReturnNoMemory: return STATUS_CODE_ERROR_ALLOCATION_FAILED;
        case kIOReturnUnsupported: return STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
        case kIOReturnTimeout: return STATUS_CODE_ERROR_TIMEOUT;
        case kIOReturnAborted: return STATUS_CODE_ERROR_INTERRUPTED;
        default: return STATUS_CODE_ERROR_IO_ERROR;
    }
}

static inline void catio_mac_hex(CATALYST_UTF8W value, CATALYST_ULONG number, BYTE digits) {
    const char* alphabet = "0123456789ABCDEF";
    for (BYTE j = digits; j != 0;) { --j; value[j] = (BYTE) alphabet[number & 15]; number >>= 4; }
    value[digits] = 0;
}
static inline BOOL catio_mac_unhex(CATALYST_UTF8 value, CATALYST_ULONG* number) {
    *number = 0;
    for (BYTE j = 0; j < 16; ++j) {
        BYTE digit = value[j];
        if (digit >= '0' && digit <= '9') digit = (BYTE) (digit - '0');
        else if (digit >= 'A' && digit <= 'F') digit = (BYTE) (digit - 'A' + 10);
        else return FALSE;
        *number = (*number << 4) | digit;
    }
    return value[16] == 0 ? TRUE : FALSE;
}
static inline STATUS_CODE catio_mac_open(const CATDVLIB_DEVICE* device, IOHIDDeviceRef* result) {
    *result = 0;
    CATDVLIB_IDENTIFIER id;
    RESULT r = catdv_device_get_identifier(device, &id);
    if (!status_code_is_success(r.status)) return r.status;
    if (catte_ascii_length(id.scheme) != 10 || catmm_compare(id.scheme, "darwin-hid", 10) != 0)
        return STATUS_CODE_ERROR_NOT_IMPLEMENTED;
    CATALYST_ULONG number;
    if (!catio_mac_unhex(id.value, &number)) return STATUS_CODE_ERROR_MALFORMED_INPUT;
    CFMutableDictionaryRef matching = IORegistryEntryIDMatching((uint64_t) number);
    if (!matching) return STATUS_CODE_ERROR_ALLOCATION_FAILED;
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, matching);
    if (!service) return STATUS_CODE_ERROR_NOT_FOUND;
    IOHIDDeviceRef hid = IOHIDDeviceCreate(kCFAllocatorDefault, service);
    IOObjectRelease(service);
    if (!hid) return STATUS_CODE_ERROR_IO_ERROR;
    IOReturn opened = IOHIDDeviceOpen(hid, kIOHIDOptionsTypeNone);
    if (opened != kIOReturnSuccess) { CFRelease(hid); return catio_mac_status(opened); }
    *result = hid;
    return STATUS_CODE_SUCCESS;
}
struct CATIO_MAC_CHANNEL {
    IOHIDElementRef element;
    BYTE type; /* 1 digital, 2 absolute, 3 relative */
    CATALYST_UTF8_CODE_UNIT identifier[17];
};
struct CATIO_MAC_RESOURCE {
    IOHIDDeviceRef device;
    CFArrayRef elements;
    CATIO_MAC_CHANNEL* channels;
    NUINT count;
};
static inline BYTE catio_mac_type(IOHIDElementRef e, BOOL input) {
    IOHIDElementType type = IOHIDElementGetType(e);
    if (input) {
        if (type != kIOHIDElementTypeInput_Misc && type != kIOHIDElementTypeInput_Button &&
            type != kIOHIDElementTypeInput_Axis && type != kIOHIDElementTypeInput_ScanCodes) return 0;
    } else if (type != kIOHIDElementTypeOutput) return 0;
    if (IOHIDElementGetReportCount(e) != 1 || IOHIDElementGetReportSize(e) > 32) return 0;
    CFIndex low = IOHIDElementGetLogicalMin(e), high = IOHIDElementGetLogicalMax(e);
    if (low < CATALYST_INT_MIN || high > CATALYST_INT_MAX || low > high) return 0;
    if (input && IOHIDElementIsRelative(e)) return 3;
    return low == 0 && high == 1 ? 1 : 2;
}
static inline STATUS_CODE catio_mac_channels(CATIO_MAC_RESOURCE* io, BOOL input) {
    io->elements = IOHIDDeviceCopyMatchingElements(io->device, 0, kIOHIDOptionsTypeNone);
    if (!io->elements) return STATUS_CODE_ERROR_NOT_FOUND;
    CFIndex count = CFArrayGetCount(io->elements);
    if (count < 0 || (CATALYST_ULONG) count > CATALYST_NUINT_MAX / sizeof(CATIO_MAC_CHANNEL))
        return STATUS_CODE_ERROR_BUFFER_OVERFLOW;
    if (!count) return STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
    CATMMLIB_POINTERW memory = 0;
    RESULT r = catmm_alloc(&memory, (NUINT) count * sizeof(CATIO_MAC_CHANNEL));
    if (!status_code_is_success(r.status)) return r.status;
    io->channels = (CATIO_MAC_CHANNEL*) memory;
    for (CFIndex j = 0; j < count; ++j) {
        IOHIDElementRef element = (IOHIDElementRef) CFArrayGetValueAtIndex(io->elements, j);
        BYTE type = catio_mac_type(element, input);
        if (!type) continue;
        CATIO_MAC_CHANNEL* c = &io->channels[io->count++];
        c->element = element; c->type = type;
        catio_mac_hex(c->identifier, (CATALYST_ULONG) IOHIDElementGetCookie(element), 16);
    }
    return io->count ? STATUS_CODE_SUCCESS : STATUS_CODE_ERROR_NOT_IMPLEMENTED;
}
static inline STATUS_CODE catio_mac_close(CATIO_MAC_RESOURCE* io) {
    if (io->device) {
        IOReturn closed = IOHIDDeviceClose(io->device, kIOHIDOptionsTypeNone);
        if (closed != kIOReturnSuccess && closed != kIOReturnNoDevice) return catio_mac_status(closed);
        CFRelease(io->device); io->device = 0;
    }
    if (io->elements) { CFRelease(io->elements); io->elements = 0; }
    if (io->channels) {
        RESULT r = catmm_free(io->channels);
        if (!status_code_is_success(r.status)) return r.status;
        io->channels = 0;
    }
    return STATUS_CODE_SUCCESS;
}
static inline STATUS_CODE catio_mac_value(const CATIO_MAC_RESOURCE* io, NUINT channel, CATALYST_INT* value) {
    IOHIDValueRef v = 0;
    IOReturn r = IOHIDDeviceGetValue(io->device, io->channels[channel].element, &v);
    if (r != kIOReturnSuccess) return catio_mac_status(r);
    if (!v) return STATUS_CODE_ERROR_NOT_FOUND;
    CFIndex n = IOHIDValueGetIntegerValue(v);
    if (n < CATALYST_INT_MIN || n > CATALYST_INT_MAX) return STATUS_CODE_ERROR_MALFORMED_INPUT;
    *value = (CATALYST_INT) n;
    return STATUS_CODE_SUCCESS;
}
#endif
