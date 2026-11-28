#include "catotlib.h"
#include "catalyst.hpp"

/* Native backend not yet implemented on this target. No resources are created. */

CATALYST_RESULT catot_device_open(const CATDVLIB_DEVICE* device, CATOTLIB_OUTPUT** output) {
    *output = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x01, 0xF0, 0);
}

CATALYST_RESULT catot_device_close(CATOTLIB_OUTPUT* output) {
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x02, 0xF0, 0);
}

CATALYST_RESULT catot_device_write(CATOTLIB_OUTPUT* output, CATMMLIB_POINTER data, CATALYST_NUINT size, CATALYST_NUINT* written) {
    *written = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x03, 0xF0, 0);
}

CATALYST_RESULT catot_device_get_channel_count(const CATOTLIB_OUTPUT* output, CATALYST_NUINT* count) {
    *count = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x04, 0xF0, 0);
}

CATALYST_RESULT catot_channel_get_type(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATOTLIB_CHANNEL_TYPE* type) {
    *type = CATOTLIB_CHANNEL_UNKNOWN;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x05, 0xF0, 0);
}

CATALYST_RESULT catot_channel_get_identifier(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = 0; identifier->value = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x06, 0xF0, 0);
}

CATALYST_RESULT catot_digital_get_state(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_BOOL* state) {
    *state = CATALYST_FALSE;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x07, 0xF0, 0);
}

CATALYST_RESULT catot_digital_set_state(CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_BOOL state) {
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x08, 0xF0, 0);
}

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catot_absolute_get_range(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT* minimum, CATALYST_INT* maximum) {
    *minimum = 0; *maximum = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x09, 0xF0, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catot_absolute_get_value(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT* value) {
    *value = 0;
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0A, 0xF0, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catot_absolute_set_value(CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT value) {
    return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0B, 0xF0, 0);
}
#endif

