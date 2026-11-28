#include "catinlib.h"
#include "catio.linux.hpp"

struct CATINLIB_INPUT {
    int fd;
    CATIO_CHANNELS channels;
    CATALYST_BOOL pending;
    struct input_event event;
};

/* Keep exactly one lookahead event, so typed reads never discard other channels. */
static CATALYST_RESULT catin_peek(CATINLIB_INPUT* input) {
    if (input->pending) return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x83, 1, 0);
    ssize_t got = read(input->fd, &input->event, sizeof(input->event));
    if (got < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS_NOOP, 0x83, 2, 0);
        return catalyst::RESULT(catio_errno(errno), 0x83, 3, 0);
    }
    if (got == 0) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_FOUND, 0x83, 4, 0);
    if ((size_t) got != sizeof(input->event)) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_IO_ERROR, 0x83, 5, 0);
    input->pending = CATALYST_TRUE;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x83, 6, 0);
}

CATALYST_RESULT catin_device_open(const CATDVLIB_DEVICE* device, CATINLIB_INPUT** input) {
    *input = 0;
    int fd = -1;
    CATALYST_RESULT r = catio_open(device, O_RDONLY, &fd);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x01, 0x01, r.detail);
    CATMMLIB_POINTERW memory = 0;
    r = catmm_alloc(&memory, sizeof(CATINLIB_INPUT));
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) {
        if (close(fd) != 0) return catalyst::RESULT(catio_errno(errno), 0x01, 0x02, 0);
        return catalyst::RESULT(r.status, 0x01, 0x03, r.detail);
    }
    CATINLIB_INPUT* resource = (CATINLIB_INPUT*) memory;
    resource->fd = fd;
    r = catio_channels(fd, CATALYST_TRUE, &resource->channels);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) {
        const int closed = close(fd);
        const int close_error = errno;
        CATALYST_RESULT freed = catmm_free(resource);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(freed.status)) return catalyst::RESULT(freed.status, 0x01, 0x04, freed.detail);
        if (closed != 0) return catalyst::RESULT(catio_errno(close_error), 0x01, 0x05, 0);
        return catalyst::RESULT(r.status, 0x01, 0x06, r.detail);
    }
    *input = resource;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x01, 0x07, 0);
}

CATALYST_RESULT catin_device_close(CATINLIB_INPUT* input) {
    CATALYST_STATUS_CODE status = catalyst::STATUS_CODE_SUCCESS;
    if (input->fd >= 0) {
        if (close(input->fd) != 0) status = catio_errno(errno);
        input->fd = -1;
    }
    CATALYST_RESULT r = catmm_free(input);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x02, 0x01, r.detail);
    if (status != catalyst::STATUS_CODE_SUCCESS) return catalyst::RESULT(status, 0x02, 0x02, 0);
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x02, 0x03, 0);
}

CATALYST_RESULT catin_device_read(CATINLIB_INPUT* input, CATMMLIB_POINTERW data, CATALYST_NUINT capacity, CATALYST_NUINT* received) {
    *received = 0;
    if (capacity < sizeof(struct input_event)) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x03, 0x01, 0);
    CATALYST_RESULT r = catin_peek(input);
    if (r.status != catalyst::STATUS_CODE_SUCCESS) return catalyst::RESULT(r.status, 0x03, 0x02, r.detail);
    catmm_copy(data, &input->event, sizeof(input->event));
    input->pending = CATALYST_FALSE;
    *received = sizeof(input->event);
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x03, 0x03, 0);
}

CATALYST_RESULT catin_device_get_channel_count(const CATINLIB_INPUT* input, CATALYST_NUINT* count) {
    *count = input->channels.count;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x04, 0x01, 0);
}

CATALYST_RESULT catin_channel_get_type(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATINLIB_CHANNEL_TYPE* type) {
    *type = input->channels.data[channel].type == EV_KEY ? CATINLIB_CHANNEL_DIGITAL : (input->channels.data[channel].type == EV_ABS ? CATINLIB_CHANNEL_ABSOLUTE : CATINLIB_CHANNEL_RELATIVE);
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x05, 0x01, 0);
}

CATALYST_RESULT catin_channel_get_identifier(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = (CATALYST_ASCII) "linux-evdev";
    identifier->value = (CATALYST_UTF8) input->channels.data[channel].identifier;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x06, 0x01, 0);
}

CATALYST_RESULT catin_digital_get_state(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_BOOL* state) {
    *state = CATALYST_FALSE;
    const CATIO_CHANNEL* c = &input->channels.data[channel];
    if (c->type != EV_KEY) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x07, 0x01, 1);
    unsigned long bits[CATIO_WORDS(KEY_CNT)] = { 0 };
    if (ioctl(input->fd, EVIOCGKEY(sizeof(bits)), bits) < 0) return catalyst::RESULT(catio_errno(errno), 0x07, 0x02, 0);
    *state = catio_bit(bits, c->code);
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x07, 0x03, 0);
}

CATALYST_RESULT catin_digital_read(CATINLIB_INPUT* input, CATALYST_NUINT channel, CATINLIB_DIGITAL_ACTION* action) {
    *action = CATINLIB_DIGITAL_RELEASE;
    const CATIO_CHANNEL* c = &input->channels.data[channel];
    if (c->type != EV_KEY) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x08, 0x01, 1);
    CATALYST_RESULT r = catin_peek(input);
    if (r.status != catalyst::STATUS_CODE_SUCCESS) return catalyst::RESULT(r.status, 0x08, 0x02, r.detail);
    if (input->event.type != EV_KEY || input->event.code != c->code) return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS_NOOP, 0x08, 0x03, 0);
    if (input->event.value < 0 || input->event.value > 2) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x04, 0);
    *action = (CATINLIB_DIGITAL_ACTION) input->event.value;
    input->pending = CATALYST_FALSE;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x08, 0x05, 0);
}

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catin_absolute_get_range(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* minimum, CATALYST_INT* maximum) {
    *minimum = 0; *maximum = 0;
    const CATIO_CHANNEL* c = &input->channels.data[channel];
    if (c->type != EV_ABS) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x09, 0x01, 1);
    struct input_absinfo info;
    if (ioctl(input->fd, EVIOCGABS(c->code), &info) < 0) return catalyst::RESULT(catio_errno(errno), 0x09, 0x02, 0);
    *minimum = info.minimum; *maximum = info.maximum;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x09, 0x03, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catin_absolute_get_value(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* value) {
    *value = 0;
    const CATIO_CHANNEL* c = &input->channels.data[channel];
    if (c->type != EV_ABS) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x01, 1);
    struct input_absinfo info;
    if (ioctl(input->fd, EVIOCGABS(c->code), &info) < 0) return catalyst::RESULT(catio_errno(errno), 0x0A, 0x02, 0);
    *value = info.value;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x0A, 0x03, 0);
}
#endif

#if CATALYST_SUPPORTS_32BIT
CATALYST_RESULT catin_relative_read(CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* offset) {
    *offset = 0;
    const CATIO_CHANNEL* c = &input->channels.data[channel];
    if (c->type != EV_REL) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0B, 0x01, 1);
    CATALYST_RESULT r = catin_peek(input);
    if (r.status != catalyst::STATUS_CODE_SUCCESS) return catalyst::RESULT(r.status, 0x0B, 0x02, r.detail);
    if (input->event.type != EV_REL || input->event.code != c->code) return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS_NOOP, 0x0B, 0x03, 0);
    *offset = input->event.value;
    input->pending = CATALYST_FALSE;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x0B, 0x04, 0);
}
#endif

