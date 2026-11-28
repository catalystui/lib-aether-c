#ifndef CATINLIB_H
#define CATINLIB_H
#include "catalyst.h"
#include "catmmlib.h"
#include "catdvlib.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Export API */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(CATINLIB_EXPORTS)
        #define CATINLIB_API __declspec(dllexport)
    #elif defined(CATINLIB_USE_DLL)
        #define CATINLIB_API __declspec(dllimport)
    #else
        #define CATINLIB_API
    #endif
#else
    #define CATINLIB_API
#endif

/* -------------------------------------------------------------------------------------------------
 * C API :: REPRESENTATIONS
 * ------------------------------------------------------------------------------------------------- */

typedef struct CATINLIB_INPUT CATINLIB_INPUT;
typedef CATALYST_BYTE CATINLIB_CHANNEL_TYPE;
#define CATINLIB_CHANNEL_UNKNOWN  0x00
#define CATINLIB_CHANNEL_DIGITAL  0x01
#define CATINLIB_CHANNEL_ABSOLUTE 0x02
#define CATINLIB_CHANNEL_RELATIVE 0x03

typedef CATALYST_BYTE CATINLIB_DIGITAL_ACTION;
#define CATINLIB_DIGITAL_RELEASE 0x00
#define CATINLIB_DIGITAL_PRESS   0x01
#define CATINLIB_DIGITAL_REPEAT  0x02

/* -------------------------------------------------------------------------------------------------
 * C API :: OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* All input pointers, output slots, live handles and channel indices must be valid.
 * Borrowed references are invalidated by release; synchronize access externally.
 * Unsupported or unimplemented capabilities are reported, not emulated. */

/** Acquire a live native input resource independently of the device snapshot; null on failure. */
CATINLIB_API CATALYST_RESULT catin_device_open(const CATDVLIB_DEVICE* device, CATINLIB_INPUT** input);

/** Release the resource according to the native backend's lifetime contract; see platform usage notes. */
CATINLIB_API CATALYST_RESULT catin_device_close(CATINLIB_INPUT* input);

/** Read at most one native event without blocking; capacity must accommodate a complete event, received is zero on failure or no data. */
CATINLIB_API CATALYST_RESULT catin_device_read(CATINLIB_INPUT* input, CATMMLIB_POINTERW data, CATALYST_NUINT capacity, CATALYST_NUINT* received);

/** Retrieve the number of individually accessible channels; zero on failure. */
CATINLIB_API CATALYST_RESULT catin_device_get_channel_count(const CATINLIB_INPUT* input, CATALYST_NUINT* count);

/** Retrieve the type of a valid channel index; unknown on failure. */
CATINLIB_API CATALYST_RESULT catin_channel_get_type(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATINLIB_CHANNEL_TYPE* type);

/** Retrieve a channel's borrowed native identifier; zero fields on failure. */
CATINLIB_API CATALYST_RESULT catin_channel_get_identifier(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATDVLIB_IDENTIFIER* identifier);

/** Retrieve the current digital state; false on failure. */
CATINLIB_API CATALYST_RESULT catin_digital_get_state(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_BOOL* state);

/** Consume the next occurrence only if it matches the requested channel; otherwise no-op without discarding it. */
CATINLIB_API CATALYST_RESULT catin_digital_read(CATINLIB_INPUT* input, CATALYST_NUINT channel, CATINLIB_DIGITAL_ACTION* action);

#if CATALYST_SUPPORTS_32BIT
/** Retrieve native integer bounds; zero both on failure. */
CATINLIB_API CATALYST_RESULT catin_absolute_get_range(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* minimum, CATALYST_INT* maximum);
#endif

#if CATALYST_SUPPORTS_32BIT
/** Retrieve native integer measurement; zero on failure. */
CATINLIB_API CATALYST_RESULT catin_absolute_get_value(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* value);
#endif

#if CATALYST_SUPPORTS_32BIT
/** Consume the next relative occurrence only if it matches the channel; zero on no-op or failure. */
CATINLIB_API CATALYST_RESULT catin_relative_read(CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* offset);
#endif

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATINLIB_H */
