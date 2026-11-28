#ifndef CATINLIB_HPP
#define CATINLIB_HPP
#include "catalyst.hpp"
#include "catmmlib.hpp"
#include "catdvlib.hpp"
#include "catinlib.h"

namespace catalyst {
namespace input {

inline RESULT device_open(const CATDVLIB_DEVICE* device, CATINLIB_INPUT** input) {
    return ::catin_device_open(device, input);
}

inline RESULT device_close(CATINLIB_INPUT* input) {
    return ::catin_device_close(input);
}

inline RESULT device_read(CATINLIB_INPUT* input, CATMMLIB_POINTERW data, NUINT capacity, NUINT* received) {
    return ::catin_device_read(input, data, capacity, received);
}

inline RESULT device_get_channel_count(const CATINLIB_INPUT* input, NUINT* count) {
    return ::catin_device_get_channel_count(input, count);
}

inline RESULT channel_get_type(const CATINLIB_INPUT* input, NUINT channel, CATINLIB_CHANNEL_TYPE* type) {
    return ::catin_channel_get_type(input, channel, type);
}

inline RESULT channel_get_identifier(const CATINLIB_INPUT* input, NUINT channel, CATDVLIB_IDENTIFIER* identifier) {
    return ::catin_channel_get_identifier(input, channel, identifier);
}

inline RESULT digital_get_state(const CATINLIB_INPUT* input, NUINT channel, BOOL* state) {
    return ::catin_digital_get_state(input, channel, state);
}

inline RESULT digital_read(CATINLIB_INPUT* input, NUINT channel, CATINLIB_DIGITAL_ACTION* action) {
    return ::catin_digital_read(input, channel, action);
}

#if CATALYST_SUPPORTS_32BIT
inline RESULT absolute_get_range(const CATINLIB_INPUT* input, NUINT channel, INT* minimum, INT* maximum) {
    return ::catin_absolute_get_range(input, channel, minimum, maximum);
}
#endif

#if CATALYST_SUPPORTS_32BIT
inline RESULT absolute_get_value(const CATINLIB_INPUT* input, NUINT channel, INT* value) {
    return ::catin_absolute_get_value(input, channel, value);
}
#endif

#if CATALYST_SUPPORTS_32BIT
inline RESULT relative_read(CATINLIB_INPUT* input, NUINT channel, INT* offset) {
    return ::catin_relative_read(input, channel, offset);
}
#endif

} /* namespace */
} /* namespace catalyst */
#endif /* CATINLIB_HPP */
