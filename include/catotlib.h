#ifndef CATOTLIB_H
#define CATOTLIB_H
#include "catalyst.h"
#include "catmmlib.h"
#include "catdvlib.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Export API */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(CATOTLIB_EXPORTS)
        #define CATOTLIB_API __declspec(dllexport)
    #elif defined(CATOTLIB_USE_DLL)
        #define CATOTLIB_API __declspec(dllimport)
    #else
        #define CATOTLIB_API
    #endif
#else
    #define CATOTLIB_API
#endif

/* -------------------------------------------------------------------------------------------------
 * C API :: REPRESENTATIONS
 * ------------------------------------------------------------------------------------------------- */

typedef struct CATOTLIB_OUTPUT CATOTLIB_OUTPUT;
typedef CATALYST_BYTE CATOTLIB_CHANNEL_TYPE;
#define CATOTLIB_CHANNEL_UNKNOWN  0x00
#define CATOTLIB_CHANNEL_DIGITAL  0x01
#define CATOTLIB_CHANNEL_ABSOLUTE 0x02

/* -------------------------------------------------------------------------------------------------
 * C API :: OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* All input pointers, output slots, live handles and channel indices must be valid.
 * Borrowed references are invalidated by release; synchronize access externally.
 * Unsupported or unimplemented capabilities are reported, not emulated. */

/** Acquire a live native output resource independently of the device snapshot; null on failure. */
CATOTLIB_API CATALYST_RESULT catot_device_open(const CATDVLIB_DEVICE* device, CATOTLIB_OUTPUT** output);

/** Release the resource; a failed memory release can be retried, but Linux close consumes the descriptor even on error. */
CATOTLIB_API CATALYST_RESULT catot_device_close(CATOTLIB_OUTPUT* output);

/** Submit exactly one native LED event without blocking; written is zero on failure or no progress. */
CATOTLIB_API CATALYST_RESULT catot_device_write(CATOTLIB_OUTPUT* output, CATMMLIB_POINTER data, CATALYST_NUINT size, CATALYST_NUINT* written);

/** Retrieve the number of individually accessible channels; zero on failure. */
CATOTLIB_API CATALYST_RESULT catot_device_get_channel_count(const CATOTLIB_OUTPUT* output, CATALYST_NUINT* count);

/** Retrieve the type of a valid channel index; unknown on failure. */
CATOTLIB_API CATALYST_RESULT catot_channel_get_type(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATOTLIB_CHANNEL_TYPE* type);

/** Retrieve a channel's borrowed native identifier; zero fields on failure. */
CATOTLIB_API CATALYST_RESULT catot_channel_get_identifier(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATDVLIB_IDENTIFIER* identifier);

/** Retrieve the current digital state; false on failure. */
CATOTLIB_API CATALYST_RESULT catot_digital_get_state(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_BOOL* state);

/** Assign a digital output state to a valid digital channel without blocking. */
CATOTLIB_API CATALYST_RESULT catot_digital_set_state(CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_BOOL state);

#if CATALYST_SUPPORTS_32BIT
/** Retrieve native integer bounds; zero both on failure. */
CATOTLIB_API CATALYST_RESULT catot_absolute_get_range(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT* minimum, CATALYST_INT* maximum);
#endif

#if CATALYST_SUPPORTS_32BIT
/** Retrieve native integer measurement; zero on failure. */
CATOTLIB_API CATALYST_RESULT catot_absolute_get_value(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT* value);
#endif

#if CATALYST_SUPPORTS_32BIT
/** Assign a native absolute output value where supported. */
CATOTLIB_API CATALYST_RESULT catot_absolute_set_value(CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT value);
#endif

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATOTLIB_H */
