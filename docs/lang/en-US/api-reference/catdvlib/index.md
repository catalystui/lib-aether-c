# CATDVLIB

**Headers:** `catdvlib.h`, `catdvlib.hpp`  
**Namespace:** `catalyst::device`  
**Library:** `catdv`  
**Dependencies:** Catalyst Core (private memory/text implementation)

## Usage Notes

Production code uses Catalyst Core for memory operations and text validation. Linux native filesystem and evdev interfaces require the platform's POSIX/Linux ABI (normally supplied by libc); sysfs and `/proc/self/fd` must be accessible for device resolution. No general-purpose C/C++ standard-library string, formatting, allocation or conversion routines are called directly by these modules. This is not a freestanding or libc-free binary guarantee.

This initial implementation supports Linux USB/PCI sysfs and evdev device discovery; hardware IDs include native modalias strings plus supported bus/model identifiers. Windows and Darwin have native backends with the additional platform-specific contracts below. All pointers and indices must meet header preconditions; synchronize access externally. Numeric values are native integer measurements, not normalized; 32-bit measurement APIs require CATALYST_SUPPORTS_32BIT.

Device snapshots are finite and retain metadata until freed. A Linux sysfs instance path identifies a kernel resource, not a permanent identity; hot-plug may invalidate or reuse it. Only evdev logical devices have queried I/O capabilities. Unknown properties return NOT_FOUND or the native error. Metadata exceeding 1023 bytes is reported as BUFFER_OVERFLOW. EDID acquisition and parsing are not yet implemented. HWIDs are not device instance identities. The `usb` and `pci` schemes use uppercase four-hex-digit `VID:PID` pairs, while `linux-input` uses `BUS:VID:PID` with four hex digits per field. The native `linux-modalias` scheme preserves the original sysfs text. USB interface entries retain the modalias rather than claiming the physical USB model identity. When both a bus/model identifier and a modalias exist, the bus/model identifier is first. If a snapshot free fails, retry on the same snapshot; do not use any old borrowed references.

### Windows and Darwin backend coverage

Windows enumerates present SetupAPI PnP device instances and HID interfaces as separate records. The `windows-pnp` and `windows-hid` instance schemes distinguish device instance IDs and interface paths. Up to sixteen native PnP hardware identifiers are preserved; an overflow is reported, not truncated. HID capabilities and manufacturer, product and serial properties are queried where the OS permits; generic PnP records have no inferred physical/logical classification or I/O capability. Darwin enumerates IOHIDManager devices only, using hexadecimal I/O registry entry IDs (`darwin-hid`), vendor/product IDs, and native HID properties. Input/output capability queries reflect native element direction only; they neither open a communication resource for callers nor interpret HID elements or transfer reports. No EDID, general Darwin USB/PCI enumeration, persistent identity, or arbitrary native-handle import is provided. Windows and Darwin snapshots use Core allocation and native SDK interfaces.

The existing Linux result tables describe Linux operation sites. Native Windows and Darwin outcomes use the same Result status definitions and fixed method contexts; their detailed operation-site tables require platform verification. No third-party runtime is introduced; Windows uses SetupAPI/HID and macOS uses IOKit/CoreFoundation. Native hardware execution is unverified.

## Types and constants

```c
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
```

## Functions

### catdv_devices_enumerate

Enumerate a snapshot into an empty output slot; null on failure.

```c
CATALYST_RESULT catdv_devices_enumerate(CATDVLIB_DEVICES** devices);
```

**Parameters:**  
`devices` — Valid resource or writable output slot as described above.

**Context:** `0x01`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0101DD` | Propagated native/dependency status | 0x01 | Dependency detail |
| `0xSS0102DD` | Propagated native/dependency status | 0x02 | Dependency detail |
| `0xSS010300` | Propagated native/dependency status | 0x03 | — |
| `0xSS0104DD` | Propagated native/dependency status | 0x04 | Dependency detail |
| `0xSS0105DD` | Propagated native/dependency status | 0x05 | Dependency detail |
| `0x00010600` | SUCCESS | 0x06 | — |
| `0x8501F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catdv_devices_free

Release a live snapshot; on partial cleanup failure retry using the same snapshot.

```c
CATALYST_RESULT catdv_devices_free(CATDVLIB_DEVICES* devices);
```

**Parameters:**  
`devices` — Valid resource or writable output slot as described above.

**Context:** `0x02`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0201DD` | Propagated native/dependency status | 0x01 | Dependency detail |
| `0xSS0202DD` | Propagated native/dependency status | 0x02 | Dependency detail |
| `0x00020300` | SUCCESS | 0x03 | — |
| `0x8502F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catdv_devices_get_count

Return the number of entries in a live snapshot.

```c
CATALYST_NUINT catdv_devices_get_count(const CATDVLIB_DEVICES* devices);
```

**Parameters:**  
`devices` — Borrowed live resource.

**Returns:** Return the number of entries in a live snapshot.

### catdv_devices_get

Borrow a device at a valid snapshot index until the snapshot is freed.

```c
const CATDVLIB_DEVICE* catdv_devices_get(const CATDVLIB_DEVICES* devices, CATALYST_NUINT index);
```

**Parameters:**  
`devices` — Borrowed live resource.  
`index` — Valid index in the reported collection.

**Returns:** Borrow a device at a valid snapshot index until the snapshot is freed.

### catdv_device_get_type

Return the device classification from the snapshot.

```c
CATDVLIB_DEVICE_TYPE catdv_device_get_type(const CATDVLIB_DEVICE* device);
```

**Parameters:**  
`device` — Borrowed live resource.

**Returns:** Return the device classification from the snapshot.

### catdv_device_get_identifier

Retrieve a borrowed system instance identifier; zero fields on failure.

```c
CATALYST_RESULT catdv_device_get_identifier(const CATDVLIB_DEVICE* device, CATDVLIB_IDENTIFIER* identifier);
```

**Parameters:**  
`device` — Borrowed live resource.  
`identifier` — Valid resource or writable output slot as described above.

**Context:** `0x06`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00060100` | SUCCESS | 0x01 | — |
| `0x8506F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catdv_device_get_hwid_count

Retrieve the count of available hardware identifiers; zero on failure.

```c
CATALYST_RESULT catdv_device_get_hwid_count(const CATDVLIB_DEVICE* device, CATALYST_NUINT* count);
```

**Parameters:**  
`device` — Borrowed live resource.  
`count` — Valid resource or writable output slot as described above.

**Context:** `0x07`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS070100` | Propagated native/dependency status | 0x01 | — |
| `0x00070200` | SUCCESS | 0x02 | — |
| `0xSS070300` | Bus identifier retrieval error | 0x03 | — |
| `0x8507F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catdv_device_get_hwid

Retrieve a borrowed hardware identifier at an index from the reported count; zero fields on failure.

```c
CATALYST_RESULT catdv_device_get_hwid(const CATDVLIB_DEVICE* device, CATALYST_NUINT index, CATDVLIB_IDENTIFIER* hwid);
```

**Parameters:**  
`device` — Borrowed live resource.  
`index` — Valid index in the reported collection.  
`hwid` — Valid resource or writable output slot as described above.

**Context:** `0x08`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00080100` | SUCCESS | 0x01 (native modalias) | — |
| `0x00080200` | SUCCESS | 0x02 (bus/model identifier) | — |
| `0x8508F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catdv_device_get_serial

Retrieve the borrowed manufacturer-assigned serial number; null on failure.

```c
CATALYST_RESULT catdv_device_get_serial(const CATDVLIB_DEVICE* device, CATALYST_UTF8* serial);
```

**Parameters:**  
`device` — Borrowed live resource.  
`serial` — Valid resource or writable output slot as described above.

**Context:** `0x09`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS090100` | Propagated native/dependency status | 0x01 | — |
| `0x00090200` | SUCCESS | 0x02 | — |
| `0x8509F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catdv_device_get_manufacturer

Retrieve the borrowed manufacturer name; null on failure.

```c
CATALYST_RESULT catdv_device_get_manufacturer(const CATDVLIB_DEVICE* device, CATALYST_UTF8* manufacturer);
```

**Parameters:**  
`device` — Borrowed live resource.  
`manufacturer` — Valid resource or writable output slot as described above.

**Context:** `0x0A`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0A0100` | Propagated native/dependency status | 0x01 | — |
| `0x000A0200` | SUCCESS | 0x02 | — |
| `0x850AF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catdv_device_get_product

Retrieve the borrowed product name; null on failure.

```c
CATALYST_RESULT catdv_device_get_product(const CATDVLIB_DEVICE* device, CATALYST_UTF8* product);
```

**Parameters:**  
`device` — Borrowed live resource.  
`product` — Valid resource or writable output slot as described above.

**Context:** `0x0B`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0B0100` | Propagated native/dependency status | 0x01 | — |
| `0x000B0200` | SUCCESS | 0x02 | — |
| `0x850BF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catdv_device_get_descriptor

Retrieve the borrowed human-readable descriptor; null on failure.

```c
CATALYST_RESULT catdv_device_get_descriptor(const CATDVLIB_DEVICE* device, CATALYST_UTF8* descriptor);
```

**Parameters:**  
`device` — Borrowed live resource.  
`descriptor` — Valid resource or writable output slot as described above.

**Context:** `0x0C`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0C0100` | Propagated native/dependency status | 0x01 | — |
| `0x000C0200` | SUCCESS | 0x02 | — |
| `0x850CF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catdv_device_get_revision

Retrieve the borrowed hardware revision; null on failure.

```c
CATALYST_RESULT catdv_device_get_revision(const CATDVLIB_DEVICE* device, CATALYST_UTF8* revision);
```

**Parameters:**  
`device` — Borrowed live resource.  
`revision` — Valid resource or writable output slot as described above.

**Context:** `0x0D`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0D0100` | Propagated native/dependency status | 0x01 | — |
| `0x000D0200` | SUCCESS | 0x02 | — |
| `0x850DF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catdv_device_has_input

Query whether the device exposes native input capabilities; false on failure.

```c
CATALYST_RESULT catdv_device_has_input(const CATDVLIB_DEVICE* device, CATALYST_BOOL* supported);
```

**Parameters:**  
`device` — Borrowed live resource.  
`supported` — Valid resource or writable output slot as described above.

**Context:** `0x0E`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x850E0100` | ERROR_NOT_IMPLEMENTED | 0x01 | — |
| `0xSS0E0200` | Propagated native/dependency status | 0x02 | — |
| `0x000E0300` | SUCCESS | 0x03 | — |
| `0x850EF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catdv_device_has_output

Query whether the device exposes native output capabilities; false on failure.

```c
CATALYST_RESULT catdv_device_has_output(const CATDVLIB_DEVICE* device, CATALYST_BOOL* supported);
```

**Parameters:**  
`device` — Borrowed live resource.  
`supported` — Valid resource or writable output slot as described above.

**Context:** `0x0F`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x850F0100` | ERROR_NOT_IMPLEMENTED | 0x01 | — |
| `0xSS0F0200` | Propagated native/dependency status | 0x02 | — |
| `0x000F0300` | SUCCESS | 0x03 | — |
| `0x850FF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

