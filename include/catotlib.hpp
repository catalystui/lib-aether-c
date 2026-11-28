#ifndef CATOTLIB_HPP
#define CATOTLIB_HPP
#include "catalyst.hpp"
#include "catmmlib.hpp"
#include "catdvlib.hpp"
#include "catotlib.h"

namespace catalyst {
namespace output {

inline RESULT device_open(const CATDVLIB_DEVICE* device, CATOTLIB_OUTPUT** output) {
    return ::catot_device_open(device, output);
}

inline RESULT device_close(CATOTLIB_OUTPUT* output) {
    return ::catot_device_close(output);
}

inline RESULT device_write(CATOTLIB_OUTPUT* output, CATMMLIB_POINTER data, NUINT size, NUINT* written) {
    return ::catot_device_write(output, data, size, written);
}

inline RESULT device_get_channel_count(const CATOTLIB_OUTPUT* output, NUINT* count) {
    return ::catot_device_get_channel_count(output, count);
}

inline RESULT channel_get_type(const CATOTLIB_OUTPUT* output, NUINT channel, CATOTLIB_CHANNEL_TYPE* type) {
    return ::catot_channel_get_type(output, channel, type);
}

inline RESULT channel_get_identifier(const CATOTLIB_OUTPUT* output, NUINT channel, CATDVLIB_IDENTIFIER* identifier) {
    return ::catot_channel_get_identifier(output, channel, identifier);
}

inline RESULT digital_get_state(const CATOTLIB_OUTPUT* output, NUINT channel, BOOL* state) {
    return ::catot_digital_get_state(output, channel, state);
}

inline RESULT digital_set_state(CATOTLIB_OUTPUT* output, NUINT channel, BOOL state) {
    return ::catot_digital_set_state(output, channel, state);
}

#if CATALYST_SUPPORTS_32BIT
inline RESULT absolute_get_range(const CATOTLIB_OUTPUT* output, NUINT channel, INT* minimum, INT* maximum) {
    return ::catot_absolute_get_range(output, channel, minimum, maximum);
}
#endif

#if CATALYST_SUPPORTS_32BIT
inline RESULT absolute_get_value(const CATOTLIB_OUTPUT* output, NUINT channel, INT* value) {
    return ::catot_absolute_get_value(output, channel, value);
}
#endif

#if CATALYST_SUPPORTS_32BIT
inline RESULT absolute_set_value(CATOTLIB_OUTPUT* output, NUINT channel, INT value) {
    return ::catot_absolute_set_value(output, channel, value);
}
#endif

} /* namespace */
} /* namespace catalyst */
#endif /* CATOTLIB_HPP */
