#ifndef CATDVLIB_H
#define CATDVLIB_H
#include "catalyst.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Export API */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(CATDVLIB_EXPORTS)
        #define CATDVLIB_API __declspec(dllexport)
    #elif defined(CATDVLIB_USE_DLL)
        #define CATDVLIB_API __declspec(dllimport)
    #else
        #define CATDVLIB_API
    #endif
#else
    #define CATDVLIB_API
#endif

/* -------------------------------------------------------------------------------------------------
 * C API :: REPRESENTATIONS
 * ------------------------------------------------------------------------------------------------- */

typedef struct CATDVLIB_DEVICE CATDVLIB_DEVICE;
typedef struct CATDVLIB_DEVICES CATDVLIB_DEVICES;

/** An identifier in a named identification scheme; strings are borrowed. */
typedef struct CATDVLIB_IDENTIFIER {
    CATALYST_ASCII scheme;
    CATALYST_UTF8 value;
} CATDVLIB_IDENTIFIER;

typedef CATALYST_BYTE CATDVLIB_DEVICE_TYPE;
#define CATDVLIB_DEVICE_UNKNOWN  0x00
#define CATDVLIB_DEVICE_PHYSICAL 0x01
#define CATDVLIB_DEVICE_LOGICAL  0x02

/* -------------------------------------------------------------------------------------------------
 * C API :: OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* All input pointers, output slots, live handles and channel indices must be valid.
 * Borrowed references are invalidated by release; synchronize access externally.
 * Unsupported or unimplemented capabilities are reported, not emulated. */

/** Enumerate a snapshot into an empty output slot; null on failure. */
CATDVLIB_API CATALYST_RESULT catdv_devices_enumerate(CATDVLIB_DEVICES** devices);

/** Release a live snapshot; on partial cleanup failure retry using the same snapshot. */
CATDVLIB_API CATALYST_RESULT catdv_devices_free(CATDVLIB_DEVICES* devices);

/** Return the number of entries in a live snapshot. */
CATDVLIB_API CATALYST_NUINT catdv_devices_get_count(const CATDVLIB_DEVICES* devices);

/** Borrow a device at a valid snapshot index until the snapshot is freed. */
CATDVLIB_API const CATDVLIB_DEVICE* catdv_devices_get(const CATDVLIB_DEVICES* devices, CATALYST_NUINT index);

/** Return the device classification from the snapshot. */
CATDVLIB_API CATDVLIB_DEVICE_TYPE catdv_device_get_type(const CATDVLIB_DEVICE* device);

/** Retrieve a borrowed system instance identifier; zero fields on failure. */
CATDVLIB_API CATALYST_RESULT catdv_device_get_identifier(const CATDVLIB_DEVICE* device, CATDVLIB_IDENTIFIER* identifier);

/** Retrieve the count of available hardware identifiers; zero on failure. */
CATDVLIB_API CATALYST_RESULT catdv_device_get_hwid_count(const CATDVLIB_DEVICE* device, CATALYST_NUINT* count);

/** Retrieve a borrowed hardware identifier at an index from the reported count; zero fields on failure. */
CATDVLIB_API CATALYST_RESULT catdv_device_get_hwid(const CATDVLIB_DEVICE* device, CATALYST_NUINT index, CATDVLIB_IDENTIFIER* hwid);

/** Retrieve the borrowed manufacturer-assigned serial number; null on failure. */
CATDVLIB_API CATALYST_RESULT catdv_device_get_serial(const CATDVLIB_DEVICE* device, CATALYST_UTF8* serial);

/** Retrieve the borrowed manufacturer name; null on failure. */
CATDVLIB_API CATALYST_RESULT catdv_device_get_manufacturer(const CATDVLIB_DEVICE* device, CATALYST_UTF8* manufacturer);

/** Retrieve the borrowed product name; null on failure. */
CATDVLIB_API CATALYST_RESULT catdv_device_get_product(const CATDVLIB_DEVICE* device, CATALYST_UTF8* product);

/** Retrieve the borrowed human-readable descriptor; null on failure. */
CATDVLIB_API CATALYST_RESULT catdv_device_get_descriptor(const CATDVLIB_DEVICE* device, CATALYST_UTF8* descriptor);

/** Retrieve the borrowed hardware revision; null on failure. */
CATDVLIB_API CATALYST_RESULT catdv_device_get_revision(const CATDVLIB_DEVICE* device, CATALYST_UTF8* revision);

/** Query whether the device exposes native input capabilities; false on failure. */
CATDVLIB_API CATALYST_RESULT catdv_device_has_input(const CATDVLIB_DEVICE* device, CATALYST_BOOL* supported);

/** Query whether the device exposes native output capabilities; false on failure. */
CATDVLIB_API CATALYST_RESULT catdv_device_has_output(const CATDVLIB_DEVICE* device, CATALYST_BOOL* supported);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATDVLIB_H */
