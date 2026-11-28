#ifndef ATHDVLIB_H
#define ATHDVLIB_H
#include "catalyst.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Export API */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(ATHDVLIB_EXPORTS)
        #define ATHDVLIB_API __declspec(dllexport)
    #elif defined(ATHDVLIB_USE_DLL)
        #define ATHDVLIB_API __declspec(dllimport)
    #else
        #define ATHDVLIB_API
    #endif
#else
    #define ATHDVLIB_API
#endif

/* -------------------------------------------------------------------------------------------------
 * C API :: REPRESENTATIONS
 * ------------------------------------------------------------------------------------------------- */

typedef struct ATHDVLIB_DEVICE ATHDVLIB_DEVICE;
typedef struct ATHDVLIB_DEVICES ATHDVLIB_DEVICES;

/** An identifier in a named identification scheme; strings are borrowed. */
typedef struct ATHDVLIB_IDENTIFIER {
    CATALYST_ASCII scheme;
    CATALYST_UTF8 value;
} ATHDVLIB_IDENTIFIER;

typedef CATALYST_BYTE ATHDVLIB_DEVICE_TYPE;
#define ATHDVLIB_DEVICE_UNKNOWN  0x00
#define ATHDVLIB_DEVICE_PHYSICAL 0x01
#define ATHDVLIB_DEVICE_LOGICAL  0x02

/* -------------------------------------------------------------------------------------------------
 * C API :: OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* All input pointers, output slots, live snapshots and device indices must be valid.
 * Borrowed references are invalidated by release; synchronize access externally.
 * Discovery and capability metadata do not grant permission to open or operate a device. */

/** Enumerate a snapshot into an empty output slot; null on failure. */
ATHDVLIB_API CATALYST_RESULT athdv_devices_enumerate(ATHDVLIB_DEVICES** devices);

/** Release a live snapshot; on partial cleanup failure retry using the same snapshot. */
ATHDVLIB_API CATALYST_RESULT athdv_devices_free(ATHDVLIB_DEVICES* devices);

/** Return the number of entries in a live snapshot. */
ATHDVLIB_API CATALYST_NUINT athdv_devices_get_count(const ATHDVLIB_DEVICES* devices);

/** Borrow a device at a valid snapshot index until the snapshot is freed. */
ATHDVLIB_API const ATHDVLIB_DEVICE* athdv_devices_get(const ATHDVLIB_DEVICES* devices, CATALYST_NUINT index);

/** Return the device classification from the snapshot. */
ATHDVLIB_API ATHDVLIB_DEVICE_TYPE athdv_device_get_type(const ATHDVLIB_DEVICE* device);

/** Retrieve a borrowed system instance identifier; zero fields on failure. */
ATHDVLIB_API CATALYST_RESULT athdv_device_get_identifier(const ATHDVLIB_DEVICE* device, ATHDVLIB_IDENTIFIER* identifier);

/** Retrieve the count of available hardware identifiers; zero on failure. Bus/model IDs precede native aliases. */
ATHDVLIB_API CATALYST_RESULT athdv_device_get_hwid_count(const ATHDVLIB_DEVICE* device, CATALYST_NUINT* count);

/** Retrieve a borrowed hardware identifier at an index from the reported count; zero fields on failure. */
ATHDVLIB_API CATALYST_RESULT athdv_device_get_hwid(const ATHDVLIB_DEVICE* device, CATALYST_NUINT index, ATHDVLIB_IDENTIFIER* hwid);

/** Retrieve the borrowed manufacturer-assigned serial number; null on failure. */
ATHDVLIB_API CATALYST_RESULT athdv_device_get_serial(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* serial);

/** Retrieve the borrowed manufacturer name; null on failure. */
ATHDVLIB_API CATALYST_RESULT athdv_device_get_manufacturer(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* manufacturer);

/** Retrieve the borrowed product name; null on failure. */
ATHDVLIB_API CATALYST_RESULT athdv_device_get_product(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* product);

/** Retrieve the borrowed human-readable descriptor; null on failure. */
ATHDVLIB_API CATALYST_RESULT athdv_device_get_descriptor(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* descriptor);

/** Retrieve the borrowed hardware revision; null on failure. */
ATHDVLIB_API CATALYST_RESULT athdv_device_get_revision(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* revision);

/** Query whether the device exposes native input capabilities; false on failure. */
ATHDVLIB_API CATALYST_RESULT athdv_device_has_input(const ATHDVLIB_DEVICE* device, CATALYST_BOOL* supported);

/** Query whether the device exposes native output capabilities; false on failure. */
ATHDVLIB_API CATALYST_RESULT athdv_device_has_output(const ATHDVLIB_DEVICE* device, CATALYST_BOOL* supported);

/** Return whether this entry represents a native display endpoint. */
ATHDVLIB_API CATALYST_BOOL athdv_device_is_display(const ATHDVLIB_DEVICE* device);

/** Borrow the original system-provided EDID bytes until snapshot release; null/zero on failure. */
ATHDVLIB_API CATALYST_RESULT athdv_display_get_edid(const ATHDVLIB_DEVICE* device, const CATALYST_BYTE** bytes, CATALYST_NUINT* size);

/** Retrieve the native display handle (Win32 HMONITOR or Darwin CGDirectDisplayID); zero on failure. */
ATHDVLIB_API CATALYST_RESULT athdv_display_get_native_handle(const ATHDVLIB_DEVICE* device, CATALYST_NUINT* handle);

/** Find an exact scheme/value instance in a live snapshot; null on absence or ambiguity. */
ATHDVLIB_API CATALYST_RESULT athdv_devices_find_identifier(const ATHDVLIB_DEVICES* devices, const ATHDVLIB_IDENTIFIER* identifier, const ATHDVLIB_DEVICE** device);

/** Find a display by its native handle in a live snapshot; null on absence or ambiguity. */
ATHDVLIB_API CATALYST_RESULT athdv_devices_find_native_display(const ATHDVLIB_DEVICES* devices, CATALYST_NUINT handle, const ATHDVLIB_DEVICE** device);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* ATHDVLIB_H */
