#ifndef ATHDVLIB_DARWIN_HPP
#define ATHDVLIB_DARWIN_HPP
#include "athdvlib.h"
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
#error "Darwin HID discovery requires 32-bit values and 64-bit registry entry identifiers."
#endif

static inline catalyst::STATUS_CODE athdv_mac_status(IOReturn status) {
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

static inline void athdv_mac_hex(CATALYST_UTF8W value, CATALYST_ULONG number, catalyst::BYTE digits) {
    const char* alphabet = "0123456789ABCDEF";
    for (catalyst::BYTE j = digits; j != 0;) { --j; value[j] = (catalyst::BYTE) alphabet[number & 15]; number >>= 4; }
    value[digits] = 0;
}
static inline catalyst::BOOL athdv_mac_direction(IOHIDElementRef e, catalyst::BOOL input) {
    IOHIDElementType type = IOHIDElementGetType(e);
    if (!input) return type == kIOHIDElementTypeOutput ? catalyst::TRUE : catalyst::FALSE;
    return type == kIOHIDElementTypeInput_Misc || type == kIOHIDElementTypeInput_Button ||
        type == kIOHIDElementTypeInput_Axis || type == kIOHIDElementTypeInput_ScanCodes ? catalyst::TRUE : catalyst::FALSE;
}
#endif
