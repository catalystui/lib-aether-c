#include "catotlib.h"
#include "catio.darwin.hpp"
#include <mach/mach_time.h>

struct CATOTLIB_OUTPUT {
    CATIO_MAC_RESOURCE io;
    CFRunLoopRef loop;
    IOHIDValueRef pending;
    CFDataRef report;
    CATALYST_BOOL completed;
    IOReturn completion;
    NUINT channel;
    CATALYST_INT value;
};

static CFStringRef catot_mac_mode() { return CFSTR("org.catalystui.aether.output"); }
static void catot_mac_callback(void* context, IOReturn result, void*, IOHIDValueRef) {
    CATOTLIB_OUTPUT* output = (CATOTLIB_OUTPUT*) context;
    output->completion = result; output->completed = TRUE;
}
static void catot_mac_report_callback(void* context, IOReturn result, void*, IOHIDReportType, uint32_t, uint8_t*, CFIndex) {
    CATOTLIB_OUTPUT* output = (CATOTLIB_OUTPUT*) context;
    output->completion = result; output->completed = TRUE;
}
static void catot_mac_release_pending(CATOTLIB_OUTPUT* output) {
    if (output->pending) { CFRelease(output->pending); output->pending = 0; }
    if (output->report) { CFRelease(output->report); output->report = 0; }
}
static STATUS_CODE catot_mac_progress(CATOTLIB_OUTPUT* output) {
    if (!output->completed) CFRunLoopRunInMode(catot_mac_mode(), 0, true);
    if (!output->completed) return STATUS_CODE_SUCCESS_NOOP;
    STATUS_CODE status = catio_mac_status(output->completion);
    catot_mac_release_pending(output);
    return status;
}
static STATUS_CODE catot_mac_submit(CATOTLIB_OUTPUT* output, NUINT channel, CATALYST_INT value) {
    if (CFRunLoopGetCurrent() != output->loop) return STATUS_CODE_ERROR_INVALID_STATE;
    if (output->report) return STATUS_CODE_ERROR_INVALID_STATE;
    if (output->pending) {
        if (output->channel != channel || output->value != value) return STATUS_CODE_ERROR_INVALID_STATE;
        return catot_mac_progress(output);
    }
    IOHIDElementRef e = output->io.channels[channel].element;
    if (value < IOHIDElementGetLogicalMin(e) || value > IOHIDElementGetLogicalMax(e)) return STATUS_CODE_ERROR_INVALID_ARGUMENT;
    output->pending = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, e, mach_absolute_time(), value);
    if (!output->pending) return STATUS_CODE_ERROR_ALLOCATION_FAILED;
    output->completed = FALSE; output->channel = channel; output->value = value;
    IOReturn submitted = IOHIDDeviceSetValueWithCallback(output->io.device, e, output->pending, 1.0, catot_mac_callback, output);
    if (submitted != kIOReturnSuccess) {
        CFRelease(output->pending); output->pending = 0;
        return catio_mac_status(submitted);
    }
    return STATUS_CODE_SUCCESS_NOOP;
}

CATALYST_RESULT catot_device_open(const CATDVLIB_DEVICE* device, CATOTLIB_OUTPUT** output) {
    *output = 0;
    CATMMLIB_POINTERW memory = 0;
    RESULT r = catmm_alloc(&memory, sizeof(CATOTLIB_OUTPUT));
    if (!status_code_is_success(r.status)) return RESULT(r.status, 0x01, 0x01, r.detail);
    CATOTLIB_OUTPUT* resource = (CATOTLIB_OUTPUT*) memory;
    STATUS_CODE status = catio_mac_open(device, &resource->io.device);
    if (status == STATUS_CODE_SUCCESS) status = catio_mac_channels(&resource->io, FALSE);
    if (status != STATUS_CODE_SUCCESS) {
        STATUS_CODE released = catio_mac_close(&resource->io);
        if (released != STATUS_CODE_SUCCESS) return RESULT(released, 0x01, 0x04, 0);
        r = catmm_free(resource);
        if (!status_code_is_success(r.status)) return RESULT(r.status, 0x01, 0x05, r.detail);
        return RESULT(status, 0x01, 0x06, 0);
    }
    resource->loop = CFRunLoopGetCurrent(); CFRetain(resource->loop);
    IOHIDDeviceScheduleWithRunLoop(resource->io.device, resource->loop, catot_mac_mode());
    *output = resource; return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x07, 0);
}

CATALYST_RESULT catot_device_close(CATOTLIB_OUTPUT* output) {
    if (output->loop && CFRunLoopGetCurrent() != output->loop) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x02, 0x02, 0);
    if (output->pending || output->report) {
        if (!output->completed) CFRunLoopRunInMode(catot_mac_mode(), 0, true);
        if (!output->completed) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x02, 0x04, 0);
        catot_mac_release_pending(output);
    }
    if (output->loop) {
        IOHIDDeviceUnscheduleFromRunLoop(output->io.device, output->loop, catot_mac_mode());
        CFRelease(output->loop); output->loop = 0;
    }
    STATUS_CODE status = catio_mac_close(&output->io);
    if (status != STATUS_CODE_SUCCESS) return RESULT(status, 0x02, 0x01, 0);
    RESULT r = catmm_free(output); return RESULT(r.status, 0x02, 0x03, r.detail);
}

CATALYST_RESULT catot_device_write(CATOTLIB_OUTPUT* output, CATMMLIB_POINTER data, CATALYST_NUINT size, CATALYST_NUINT* written) {
    *written = 0;
    if (CFRunLoopGetCurrent() != output->loop || output->pending)
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x03, 0x06, 0);
    /* Explicit ID prefix plus complete native report bytes. */
    if (size < 2 || size > (NUINT) CATALYST_LONG_MAX)
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x07, 2);
    const BYTE* bytes = (const BYTE*) data;
    if (output->report) {
        if ((NUINT) CFDataGetLength(output->report) != size ||
            catmm_compare(CFDataGetBytePtr(output->report), bytes, size) != 0)
            return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x03, 0x08, 0);
        STATUS_CODE status = catot_mac_progress(output);
        if (status != STATUS_CODE_SUCCESS) return RESULT(status, 0x03, 0x09, 0);
        *written = size; return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x0A, 0);
    }
    if (bytes[0] != 0 && bytes[1] != bytes[0])
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x0B, 1);
    BOOL known = FALSE;
    for (NUINT j = 0; j < output->io.count; ++j)
        if (IOHIDElementGetReportID(output->io.channels[j].element) == bytes[0]) { known = TRUE; break; }
    if (!known) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x0C, 1);
    CFTypeRef maxReport = IOHIDDeviceGetProperty(output->io.device, CFSTR(kIOHIDMaxOutputReportSizeKey));
    SInt64 maximum = 0;
    if (!maxReport) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0x03, 0x0D, 0);
    if (CFGetTypeID(maxReport) != CFNumberGetTypeID() ||
        !CFNumberGetValue((CFNumberRef) maxReport, kCFNumberSInt64Type, &maximum) || maximum <= 0)
        return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x03, 0x0E, 0);
    if ((CATALYST_ULONG) (size - 1) > (CATALYST_ULONG) maximum)
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x0F, 2);
    /* Retain native immutable report storage until the OS callback completes. */
    output->report = CFDataCreate(kCFAllocatorDefault, bytes, (CFIndex) size);
    if (!output->report) return RESULT(STATUS_CODE_ERROR_ALLOCATION_FAILED, 0x03, 0x10, 0);
    output->completed = FALSE;
    const UInt8* record = CFDataGetBytePtr(output->report);
    IOReturn submitted = IOHIDDeviceSetReportWithCallback(output->io.device, kIOHIDReportTypeOutput,
        record[0], record + 1, (CFIndex) (size - 1), 1.0, catot_mac_report_callback, output);
    if (submitted != kIOReturnSuccess) {
        catot_mac_release_pending(output);
        return RESULT(catio_mac_status(submitted), 0x03, 0x11, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x03, 0x12, 0);
}

CATALYST_RESULT catot_device_get_channel_count(const CATOTLIB_OUTPUT* output, CATALYST_NUINT* count) {
    *count = output->io.count; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x01, 0);
}

CATALYST_RESULT catot_channel_get_type(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATOTLIB_CHANNEL_TYPE* type) {
    *type = output->io.channels[channel].type; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x01, 0);
}

CATALYST_RESULT catot_channel_get_identifier(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = (CATALYST_ASCII) "darwin-hid-cookie";
    identifier->value = output->io.channels[channel].identifier; return RESULT(STATUS_CODE_SUCCESS, 0x06, 0x01, 0);
}

CATALYST_RESULT catot_digital_get_state(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_BOOL* state) {
    *state = FALSE;
    if (output->io.channels[channel].type != CATOTLIB_CHANNEL_DIGITAL) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x07, 0x01, 1);
    CATALYST_INT value;
    STATUS_CODE status = catio_mac_value(&output->io, channel, &value);
    if (status != STATUS_CODE_SUCCESS) return RESULT(status, 0x07, 0x02, 0);
    *state = value ? TRUE : FALSE; return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x03, 0);
}

CATALYST_RESULT catot_digital_set_state(CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_BOOL state) {
    if (output->io.channels[channel].type != CATOTLIB_CHANNEL_DIGITAL) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x08, 0x01, 1);
    STATUS_CODE status = catot_mac_submit(output, channel, state ? 1 : 0);
    return RESULT(status, 0x08, 0x02, status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? 2 : 0);
}

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catot_absolute_get_range(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT* minimum, CATALYST_INT* maximum) {
    *minimum = 0; *maximum = 0;
    const CATIO_MAC_CHANNEL* c = &output->io.channels[channel];
    if (c->type != CATOTLIB_CHANNEL_ABSOLUTE) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x09, 0x02, 1);
    *minimum = (CATALYST_INT) IOHIDElementGetLogicalMin(c->element);
    *maximum = (CATALYST_INT) IOHIDElementGetLogicalMax(c->element);
    return RESULT(STATUS_CODE_SUCCESS, 0x09, 0x03, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catot_absolute_get_value(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT* value) {
    *value = 0;
    if (output->io.channels[channel].type != CATOTLIB_CHANNEL_ABSOLUTE) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x02, 1);
    STATUS_CODE status = catio_mac_value(&output->io, channel, value);
    return RESULT(status, 0x0A, 0x03, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catot_absolute_set_value(CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT value) {
    if (output->io.channels[channel].type != CATOTLIB_CHANNEL_ABSOLUTE) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0B, 0x02, 1);
    STATUS_CODE status = catot_mac_submit(output, channel, value);
    return RESULT(status, 0x0B, 0x03, status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? 2 : 0);
}
#endif

