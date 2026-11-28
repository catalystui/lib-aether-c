#include "catinlib.h"
#include "catio.win32.hpp"

struct CATINLIB_INPUT { CATIO_WIN_RESOURCE io; };

/* Raw HID reports are supported. Portable HID channel parsing is not yet implemented. */

CATALYST_RESULT catin_device_open(const CATDVLIB_DEVICE* device, CATINLIB_INPUT** input) {
    *input = 0;
    CATMMLIB_POINTERW memory = 0;
    RESULT r = catmm_alloc(&memory, sizeof(CATINLIB_INPUT));
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return RESULT(r.status, 0x01, 0x01, r.detail);
    CATINLIB_INPUT* resource = (CATINLIB_INPUT*) memory;
    STATUS_CODE status = catio_win_open(device, CATALYST_TRUE, &resource->io);
    if (status != CATALYST_STATUS_CODE_SUCCESS) {
        r = catmm_free(resource);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return RESULT(r.status, 0x01, 0x02, r.detail);
        return RESULT(status, 0x01, 0x03, 0);
    }
    *input = resource;
    return RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x01, 0x07, 0);
}

CATALYST_RESULT catin_device_close(CATINLIB_INPUT* input) {
    STATUS_CODE status = catio_win_close(&input->io);
    if (status != CATALYST_STATUS_CODE_SUCCESS) return RESULT(status, 0x02, 0x01, 0);
    RESULT r = catmm_free(input);
    return RESULT(r.status, 0x02, 0x03, r.detail);
}

CATALYST_RESULT catin_device_read(CATINLIB_INPUT* input, CATMMLIB_POINTERW data, CATALYST_NUINT capacity, CATALYST_NUINT* received) {
    *received = 0;
    if (capacity < input->io.length) return RESULT(CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x03, 0x01, 0);
    DWORD count = 0;
    STATUS_CODE status = catio_win_transfer(&input->io, CATALYST_TRUE, &count);
    if (status != CATALYST_STATUS_CODE_SUCCESS) return RESULT(status, 0x03, 0x02, 0);
    if (count != input->io.length) return RESULT(CATALYST_STATUS_CODE_ERROR_IO_ERROR, 0x03, 0x04, 0);
    catmm_copy(data, input->io.buffer, count); *received = count;
    return RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x03, 0x03, 0);
}

CATALYST_RESULT catin_device_get_channel_count(const CATINLIB_INPUT* input, CATALYST_NUINT* count) {
    *count = 0;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x04, 0xF0, 0);
}

CATALYST_RESULT catin_channel_get_type(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATINLIB_CHANNEL_TYPE* type) {
    *type = CATINLIB_CHANNEL_UNKNOWN;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x05, 0xF0, 0);
}

CATALYST_RESULT catin_channel_get_identifier(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = 0; identifier->value = 0;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x06, 0xF0, 0);
}

CATALYST_RESULT catin_digital_get_state(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_BOOL* state) {
    *state = CATALYST_FALSE;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x07, 0xF0, 0);
}

CATALYST_RESULT catin_digital_read(CATINLIB_INPUT* input, CATALYST_NUINT channel, CATINLIB_DIGITAL_ACTION* action) {
    *action = CATINLIB_DIGITAL_RELEASE;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x08, 0xF0, 0);
}

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catin_absolute_get_range(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* minimum, CATALYST_INT* maximum) {
    *minimum = 0; *maximum = 0;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x09, 0xF0, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catin_absolute_get_value(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* value) {
    *value = 0;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0A, 0xF0, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catin_relative_read(CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* offset) {
    *offset = 0;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0B, 0xF0, 0);
}
#endif

