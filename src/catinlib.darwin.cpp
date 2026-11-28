#include "catinlib.h"
#include "catio.darwin.hpp"

struct CATINLIB_INPUT {
    CATIO_MAC_RESOURCE io;
    IOHIDQueueRef queue;
    IOHIDValueRef pending;
};

static STATUS_CODE catin_mac_peek(CATINLIB_INPUT* input) {
    if (!input->pending) input->pending = IOHIDQueueCopyNextValue(input->queue);
    return input->pending ? STATUS_CODE_SUCCESS : STATUS_CODE_SUCCESS_NOOP;
}
static void catin_mac_consume(CATINLIB_INPUT* input) {
    CFRelease(input->pending); input->pending = 0;
}
static STATUS_CODE catin_mac_close(CATINLIB_INPUT* input) {
    if (input->pending) catin_mac_consume(input);
    if (input->queue) { IOHIDQueueStop(input->queue); CFRelease(input->queue); input->queue = 0; }
    return catio_mac_close(&input->io);
}

CATALYST_RESULT catin_device_open(const CATDVLIB_DEVICE* device, CATINLIB_INPUT** input) {
    *input = 0;
    CATMMLIB_POINTERW memory = 0;
    RESULT r = catmm_alloc(&memory, sizeof(CATINLIB_INPUT));
    if (!status_code_is_success(r.status)) return RESULT(r.status, 0x01, 0x01, r.detail);
    CATINLIB_INPUT* resource = (CATINLIB_INPUT*) memory;
    STATUS_CODE status = catio_mac_open(device, &resource->io.device);
    if (status == STATUS_CODE_SUCCESS) status = catio_mac_channels(&resource->io, TRUE);
    if (status == STATUS_CODE_SUCCESS) {
        resource->queue = IOHIDQueueCreate(kCFAllocatorDefault, resource->io.device, 256, kIOHIDOptionsTypeNone);
        if (!resource->queue) status = STATUS_CODE_ERROR_ALLOCATION_FAILED;
        else {
            for (NUINT j = 0; j < resource->io.count; ++j) {
                IOHIDElementRef element = resource->io.channels[j].element;
                IOHIDQueueAddElement(resource->queue, element);
                if (!IOHIDQueueContainsElement(resource->queue, element)) {
                    status = STATUS_CODE_ERROR_IO_ERROR;
                    break;
                }
            }
            if (status == STATUS_CODE_SUCCESS) IOHIDQueueStart(resource->queue);
        }
    }
    if (status != STATUS_CODE_SUCCESS) {
        STATUS_CODE released = catin_mac_close(resource);
        if (released != STATUS_CODE_SUCCESS) return RESULT(released, 0x01, 0x04, 0);
        r = catmm_free(resource);
        if (!status_code_is_success(r.status)) return RESULT(r.status, 0x01, 0x05, r.detail);
        return RESULT(status, 0x01, 0x06, 0);
    }
    *input = resource; return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x07, 0);
}

CATALYST_RESULT catin_device_close(CATINLIB_INPUT* input) {
    STATUS_CODE status = catin_mac_close(input);
    if (status != STATUS_CODE_SUCCESS) return RESULT(status, 0x02, 0x01, 0);
    RESULT r = catmm_free(input); return RESULT(r.status, 0x02, 0x03, r.detail);
}

CATALYST_RESULT catin_device_read(CATINLIB_INPUT* input, CATMMLIB_POINTERW data, CATALYST_NUINT capacity, CATALYST_NUINT* received) {
    *received = 0;
    STATUS_CODE status = catin_mac_peek(input);
    if (status != STATUS_CODE_SUCCESS) return RESULT(status, 0x03, 0x02, 0);
    CFIndex length = IOHIDValueGetLength(input->pending);
    if (length < 0 || (CATALYST_ULONG) length > CATALYST_NUINT_MAX - sizeof(IOHIDElementCookie))
        return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x03, 0x04, 0);
    NUINT size = sizeof(IOHIDElementCookie) + (NUINT) length;
    if (capacity < size) return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x03, 0x01, 0);
    IOHIDElementCookie cookie = IOHIDElementGetCookie(IOHIDValueGetElement(input->pending));
    catmm_copy(data, &cookie, sizeof(cookie));
    if (length) catmm_copy((CATALYST_BYTE*) data + sizeof(cookie), IOHIDValueGetBytePtr(input->pending), (NUINT) length);
    catin_mac_consume(input);
    *received = size; return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x03, 0);
}

CATALYST_RESULT catin_device_get_channel_count(const CATINLIB_INPUT* input, CATALYST_NUINT* count) {
    *count = input->io.count; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x01, 0);
}

CATALYST_RESULT catin_channel_get_type(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATINLIB_CHANNEL_TYPE* type) {
    *type = input->io.channels[channel].type; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x01, 0);
}

CATALYST_RESULT catin_channel_get_identifier(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = (CATALYST_ASCII) "darwin-hid-cookie";
    identifier->value = input->io.channels[channel].identifier; return RESULT(STATUS_CODE_SUCCESS, 0x06, 0x01, 0);
}

CATALYST_RESULT catin_digital_get_state(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_BOOL* state) {
    *state = FALSE;
    if (input->io.channels[channel].type != CATINLIB_CHANNEL_DIGITAL) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x07, 0x01, 1);
    CATALYST_INT value;
    STATUS_CODE s = catio_mac_value(&input->io, channel, &value);
    if (s != STATUS_CODE_SUCCESS) return RESULT(s, 0x07, 0x02, 0);
    *state = value ? TRUE : FALSE; return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x03, 0);
}

CATALYST_RESULT catin_digital_read(CATINLIB_INPUT* input, CATALYST_NUINT channel, CATINLIB_DIGITAL_ACTION* action) {
    *action = CATINLIB_DIGITAL_RELEASE;
    const CATIO_MAC_CHANNEL* c = &input->io.channels[channel];
    if (c->type != CATINLIB_CHANNEL_DIGITAL) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x08, 0x01, 1);
    STATUS_CODE status = catin_mac_peek(input);
    if (status != STATUS_CODE_SUCCESS) return RESULT(status, 0x08, 0x02, 0);
    if (IOHIDElementGetCookie(IOHIDValueGetElement(input->pending)) != IOHIDElementGetCookie(c->element)) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x08, 0x03, 0);
    CFIndex value = IOHIDValueGetIntegerValue(input->pending);
    if (value != 0 && value != 1) return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x04, 0);
    *action = value ? CATINLIB_DIGITAL_PRESS : CATINLIB_DIGITAL_RELEASE;
    catin_mac_consume(input); return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x05, 0);
}

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catin_absolute_get_range(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* minimum, CATALYST_INT* maximum) {
    *minimum = 0; *maximum = 0;
    const CATIO_MAC_CHANNEL* c = &input->io.channels[channel];
    if (c->type != CATINLIB_CHANNEL_ABSOLUTE) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x09, 0x01, 1);
    *minimum = (CATALYST_INT) IOHIDElementGetLogicalMin(c->element);
    *maximum = (CATALYST_INT) IOHIDElementGetLogicalMax(c->element);
    return RESULT(STATUS_CODE_SUCCESS, 0x09, 0x03, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catin_absolute_get_value(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* value) {
    *value = 0;
    if (input->io.channels[channel].type != CATINLIB_CHANNEL_ABSOLUTE) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x01, 1);
    STATUS_CODE status = catio_mac_value(&input->io, channel, value);
    return RESULT(status, 0x0A, 0x02, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catin_relative_read(CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* offset) {
    *offset = 0;
    const CATIO_MAC_CHANNEL* c = &input->io.channels[channel];
    if (c->type != CATINLIB_CHANNEL_RELATIVE) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0B, 0x01, 1);
    STATUS_CODE status = catin_mac_peek(input);
    if (status != STATUS_CODE_SUCCESS) return RESULT(status, 0x0B, 0x02, 0);
    if (IOHIDElementGetCookie(IOHIDValueGetElement(input->pending)) != IOHIDElementGetCookie(c->element)) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x0B, 0x03, 0);
    CFIndex n = IOHIDValueGetIntegerValue(input->pending);
    if (n < CATALYST_INT_MIN || n > CATALYST_INT_MAX) return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x0B, 0x05, 0);
    *offset = (CATALYST_INT) n; catin_mac_consume(input); return RESULT(STATUS_CODE_SUCCESS, 0x0B, 0x04, 0);
}
#endif

