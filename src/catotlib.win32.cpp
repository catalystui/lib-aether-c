#include "catotlib.h"
#include "catio.win32.hpp"

struct CATOTLIB_OUTPUT { CATIO_WIN_RESOURCE io; };

/* Raw HID reports are supported. Portable HID channel parsing is not yet implemented. */

CATALYST_RESULT catot_device_open(const CATDVLIB_DEVICE* device, CATOTLIB_OUTPUT** output) {
    *output = 0;
    CATMMLIB_POINTERW memory = 0;
    RESULT r = catmm_alloc(&memory, sizeof(CATOTLIB_OUTPUT));
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return RESULT(r.status, 0x01, 0x01, r.detail);
    CATOTLIB_OUTPUT* resource = (CATOTLIB_OUTPUT*) memory;
    STATUS_CODE status = catio_win_open(device, CATALYST_FALSE, &resource->io);
    if (status != CATALYST_STATUS_CODE_SUCCESS) {
        r = catmm_free(resource);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return RESULT(r.status, 0x01, 0x02, r.detail);
        return RESULT(status, 0x01, 0x03, 0);
    }
    *output = resource;
    return RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x01, 0x07, 0);
}

CATALYST_RESULT catot_device_close(CATOTLIB_OUTPUT* output) {
    STATUS_CODE status = catio_win_close(&output->io);
    if (status != CATALYST_STATUS_CODE_SUCCESS) return RESULT(status, 0x02, 0x01, 0);
    RESULT r = catmm_free(output);
    return RESULT(r.status, 0x02, 0x03, r.detail);
}

CATALYST_RESULT catot_device_write(CATOTLIB_OUTPUT* output, CATMMLIB_POINTER data, CATALYST_NUINT size, CATALYST_NUINT* written) {
    *written = 0;
    if (size != output->io.length) return RESULT(CATALYST_STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x01, 2);
    if (output->io.pending) {
        if (catmm_compare(data, output->io.buffer, size) != 0) return RESULT(CATALYST_STATUS_CODE_ERROR_INVALID_STATE, 0x03, 0x02, 0);
    } else catmm_copy(output->io.buffer, data, size);
    DWORD count = 0;
    STATUS_CODE status = catio_win_transfer(&output->io, CATALYST_FALSE, &count);
    if (status != CATALYST_STATUS_CODE_SUCCESS) return RESULT(status, 0x03, 0x04, 0);
    if (count != size) return RESULT(CATALYST_STATUS_CODE_ERROR_IO_ERROR, 0x03, 0x03, 0);
    *written = count;
    return RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x03, 0x05, 0);
}

CATALYST_RESULT catot_device_get_channel_count(const CATOTLIB_OUTPUT* output, CATALYST_NUINT* count) {
    *count = 0;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x04, 0xF0, 0);
}

CATALYST_RESULT catot_channel_get_type(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATOTLIB_CHANNEL_TYPE* type) {
    *type = CATOTLIB_CHANNEL_UNKNOWN;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x05, 0xF0, 0);
}

CATALYST_RESULT catot_channel_get_identifier(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = 0; identifier->value = 0;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x06, 0xF0, 0);
}

CATALYST_RESULT catot_digital_get_state(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_BOOL* state) {
    *state = CATALYST_FALSE;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x07, 0xF0, 0);
}

CATALYST_RESULT catot_digital_set_state(CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_BOOL state) {
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x08, 0xF0, 0);
}

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catot_absolute_get_range(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT* minimum, CATALYST_INT* maximum) {
    *minimum = 0; *maximum = 0;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x09, 0xF0, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catot_absolute_get_value(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT* value) {
    *value = 0;
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0A, 0xF0, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catot_absolute_set_value(CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT value) {
    return RESULT(CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0B, 0xF0, 0);
}
#endif

