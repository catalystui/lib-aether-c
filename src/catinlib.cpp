#include "catinlib.h"
#include "catalyst.hpp"
using namespace catalyst;

/* Native backend not yet implemented on this target. No resources are created. */

CATALYST_RESULT catin_device_open(const CATDVLIB_DEVICE* device, CATINLIB_INPUT** input) {
    *input = 0;
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x01, 0xF0, 0);
}

CATALYST_RESULT catin_device_close(CATINLIB_INPUT* input) {
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x02, 0xF0, 0);
}

CATALYST_RESULT catin_device_read(CATINLIB_INPUT* input, CATMMLIB_POINTERW data, CATALYST_NUINT capacity, CATALYST_NUINT* received) {
    *received = 0;
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x03, 0xF0, 0);
}

CATALYST_RESULT catin_device_get_channel_count(const CATINLIB_INPUT* input, CATALYST_NUINT* count) {
    *count = 0;
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x04, 0xF0, 0);
}

CATALYST_RESULT catin_channel_get_type(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATINLIB_CHANNEL_TYPE* type) {
    *type = CATINLIB_CHANNEL_UNKNOWN;
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x05, 0xF0, 0);
}

CATALYST_RESULT catin_channel_get_identifier(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = 0; identifier->value = 0;
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x06, 0xF0, 0);
}

CATALYST_RESULT catin_digital_get_state(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_BOOL* state) {
    *state = CATALYST_FALSE;
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x07, 0xF0, 0);
}

CATALYST_RESULT catin_digital_read(CATINLIB_INPUT* input, CATALYST_NUINT channel, CATINLIB_DIGITAL_ACTION* action) {
    *action = CATINLIB_DIGITAL_RELEASE;
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x08, 0xF0, 0);
}

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catin_absolute_get_range(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* minimum, CATALYST_INT* maximum) {
    *minimum = 0; *maximum = 0;
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x09, 0xF0, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catin_absolute_get_value(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* value) {
    *value = 0;
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0A, 0xF0, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catin_relative_read(CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* offset) {
    *offset = 0;
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0B, 0xF0, 0);
}
#endif

