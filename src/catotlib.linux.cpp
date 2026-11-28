#include "catotlib.h"
#include "catio.linux.hpp"

struct CATOTLIB_OUTPUT {
    int fd;
    CATIO_CHANNELS channels;
};

CATALYST_RESULT catot_device_open(const CATDVLIB_DEVICE* device, CATOTLIB_OUTPUT** output) {
    *output = 0;
    int fd = -1;
    CATALYST_RESULT r = catio_open(device, O_WRONLY, &fd);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return RESULT(r.status, 0x01, 0x01, r.detail);
    CATMMLIB_POINTERW memory = 0;
    r = catmm_alloc(&memory, sizeof(CATOTLIB_OUTPUT));
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) {
        if (close(fd) != 0) return RESULT(catio_errno(errno), 0x01, 0x02, 0);
        return RESULT(r.status, 0x01, 0x03, r.detail);
    }
    CATOTLIB_OUTPUT* resource = (CATOTLIB_OUTPUT*) memory;
    resource->fd = fd;
    r = catio_channels(fd, CATALYST_FALSE, &resource->channels);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) {
        const int closed = close(fd);
        const int close_error = errno;
        CATALYST_RESULT freed = catmm_free(resource);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(freed.status)) return RESULT(freed.status, 0x01, 0x04, freed.detail);
        if (closed != 0) return RESULT(catio_errno(close_error), 0x01, 0x05, 0);
        return RESULT(r.status, 0x01, 0x06, r.detail);
    }
    *output = resource;
    return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x07, 0);
}

CATALYST_RESULT catot_device_close(CATOTLIB_OUTPUT* output) {
    CATALYST_STATUS_CODE status = STATUS_CODE_SUCCESS;
    if (output->fd >= 0) {
        if (close(output->fd) != 0) status = catio_errno(errno);
        output->fd = -1;
    }
    CATALYST_RESULT r = catmm_free(output);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return RESULT(r.status, 0x02, 0x01, r.detail);
    if (status != STATUS_CODE_SUCCESS) return RESULT(status, 0x02, 0x02, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x02, 0x03, 0);
}

CATALYST_RESULT catot_device_write(CATOTLIB_OUTPUT* output, CATMMLIB_POINTER data, CATALYST_NUINT size, CATALYST_NUINT* written) {
    *written = 0;
    if (size != sizeof(struct input_event)) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x01, 2);
    struct input_event event;
    catmm_copy(&event, data, sizeof(event));
    if (event.type != EV_LED || event.code >= LED_CNT || (event.value != 0 && event.value != 1)) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x02, 1);
    if (!catio_contains(&output->channels, EV_LED, event.code)) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x03, 1);
    CATALYST_RESULT r = catio_write_led(output->fd, event.code, event.value);
    if (r.status != STATUS_CODE_SUCCESS) return RESULT(r.status, 0x03, 0x04, r.detail);
    *written = sizeof(event);
    return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x05, 0);
}

CATALYST_RESULT catot_device_get_channel_count(const CATOTLIB_OUTPUT* output, CATALYST_NUINT* count) {
    *count = output->channels.count;
    return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x01, 0);
}

CATALYST_RESULT catot_channel_get_type(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATOTLIB_CHANNEL_TYPE* type) {
    *type = output->channels.data[channel].type == EV_LED ? CATOTLIB_CHANNEL_DIGITAL : CATOTLIB_CHANNEL_UNKNOWN;
    return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x01, 0);
}

CATALYST_RESULT catot_channel_get_identifier(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = (CATALYST_ASCII) "linux-evdev";
    identifier->value = (CATALYST_UTF8) output->channels.data[channel].identifier;
    return RESULT(STATUS_CODE_SUCCESS, 0x06, 0x01, 0);
}

CATALYST_RESULT catot_digital_get_state(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_BOOL* state) {
    *state = CATALYST_FALSE;
    const CATIO_CHANNEL* c = &output->channels.data[channel];
    if (c->type != EV_LED) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x07, 0x01, 1);
    unsigned long bits[CATIO_WORDS(LED_CNT)] = { 0 };
    if (ioctl(output->fd, EVIOCGLED(sizeof(bits)), bits) < 0) return RESULT(catio_errno(errno), 0x07, 0x02, 0);
    *state = catio_bit(bits, c->code);
    return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x03, 0);
}

CATALYST_RESULT catot_digital_set_state(CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_BOOL state) {
    const CATIO_CHANNEL* c = &output->channels.data[channel];
    if (c->type != EV_LED) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x08, 0x01, 1);
    CATALYST_RESULT r = catio_write_led(output->fd, c->code, state ? 1 : 0);
    return RESULT(r.status, 0x08, 0x02, r.detail);
}

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catot_absolute_get_range(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT* minimum, CATALYST_INT* maximum) {
    *minimum = 0; *maximum = 0;
    return RESULT(STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED, 0x09, 0x01, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catot_absolute_get_value(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT* value) {
    *value = 0;
    return RESULT(STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED, 0x0A, 0x01, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catot_absolute_set_value(CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT value) {
    return RESULT(STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED, 0x0B, 0x01, 0);
}
#endif

