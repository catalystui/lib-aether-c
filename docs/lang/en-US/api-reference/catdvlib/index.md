# CATDVLIB

**Headers:** `catdvlib.h`, `catdvlib.hpp`  
**Namespace:** `catalyst::device`  
**Library:** `catdv`  
**Dependencies:** Catalyst Core (private memory/text implementation)

## Usage Notes

Production code uses Catalyst Core for memory operations and text validation. Linux native filesystem and evdev interfaces require the platform's POSIX/Linux ABI (normally supplied by libc); sysfs and `/proc/self/fd` must be accessible for device resolution. No general-purpose C/C++ standard-library string, formatting, allocation or conversion routines are called directly by these modules. This is not a freestanding or libc-free binary guarantee.

This initial implementation supports Linux USB/PCI sysfs and evdev device discovery; hardware IDs include native modalias strings plus supported bus/model identifiers. Windows and Darwin have native backends with the additional platform-specific contracts below. All pointers and indices must meet header preconditions; synchronize access externally. Numeric values are native integer measurements, not normalized; 32-bit measurement APIs require CATALYST_SUPPORTS_32BIT.

Device snapshots are finite and retain metadata until freed. A Linux sysfs instance path identifies a kernel resource, not a permanent identity; hot-plug may invalidate or reuse it. Only evdev logical devices have queried I/O capabilities. Unknown properties return NOT_FOUND or the native error. Metadata exceeding 1023 bytes is reported as BUFFER_OVERFLOW. Connected DRM connectors are also enumerated as logical display endpoints. EDID is copied from the kernel's binary connector attribute, not parsed. HWIDs are not device instance identities. The `usb` and `pci` schemes use uppercase four-hex-digit `VID:PID` pairs, while `linux-input` uses `BUS:VID:PID` with four hex digits per field. The native `linux-modalias` scheme preserves the original sysfs text. USB interface entries retain the modalias rather than claiming the physical USB model identity. When both a bus/model identifier and a modalias exist, the bus/model identifier is first. If a snapshot free fails, retry on the same snapshot; do not use any old borrowed references.

### Windows and Darwin backend coverage

Windows enumerates present SetupAPI PnP device instances and HID interfaces as separate records. The `windows-pnp` and `windows-hid` instance schemes distinguish device instance IDs and interface paths. Up to sixteen native PnP hardware identifiers are preserved; an overflow is reported, not truncated. HID capabilities and manufacturer, product and serial properties are queried where the OS permits; generic PnP records have no inferred physical/logical classification or I/O capability. Darwin enumerates IOHIDManager devices only, using hexadecimal I/O registry entry IDs (`darwin-hid`), vendor/product IDs, and native HID properties. Input/output capability queries reflect native element direction only; they neither open a communication resource for callers nor interpret HID elements or transfer reports. Online CoreGraphics displays are enumerated separately from HID endpoints, and legacy public IOKit display information supplies EDID only when the native vendor/product/serial association is unambiguous. General Darwin USB/PCI enumeration, persistent identity, and arbitrary native-handle import remain unsupported. Windows and Darwin snapshots use Core allocation and native SDK interfaces.

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


## Display identification and native interoperability

Display endpoints are included in the same device snapshot. They are logical system records, not an assertion that each record is a distinct physical panel. They expose output capability without opening an output communication resource. No geometry, modes, rendering, windowing, EDID interpretation, DDC/CI, or private SDK APIs are included.

- Windows: active monitor interfaces associated with Win32 monitor handles. `windows-monitor` identifies the exact monitor interface path; `windows-display-source` is used when no active monitor interface is exposed. SetupAPI opens the corresponding device registry key and reads its binary EDID value. Several monitor endpoints may share one HMONITOR (for example, cloned displays).
- Darwin: online CGDirectDisplayID values identify `darwin-display` entries (eight uppercase hexadecimal digits). The model identifier uses the `darwin-display-model` scheme, with eight-digit vendor and product fields separated by a colon. A nonzero numeric CoreGraphics serial is exposed in hexadecimal. EDID comes from public IOKit IODisplayConnect dictionaries only when a single native association can be established; matching uses vendor, product and available serial values. Duplicate/ambiguous associations are reported rather than guessed. On Apple Silicon, including built-in panels, the public legacy property may be unavailable. No synthetic EDID or private AVService access is used.
- Linux: connected DRM connectors are exposed as `linux-sysfs` canonical instance paths. The binary EDID attribute is read to EOF, with a 32768-byte bound; larger values report overflow. There is no single process-independent numeric X11/Wayland/DRM display handle. Use exact canonical connector identifiers for lookup. No libdrm, X11 or Wayland dependency is introduced.

EDID bytes are copied as supplied by the operating system, including extensions, and remain valid until snapshot release. No checksum or descriptor decoding is performed. Hardware identification derived from EDID contents belongs outside this acquisition interface. Native handles are borrowed numeric identities at the time of enumeration, not acquired resources. Refresh the snapshot after display reconfiguration or hotplug; a handle may be invalidated or reused. Never close a native handle obtained here.

### catdv_device_is_display

```c
CATALYST_BOOL catdv_device_is_display(const CATDVLIB_DEVICE* device);
```

Returns whether a valid borrowed snapshot entry is a display endpoint.

### catdv_display_get_edid

```c
CATALYST_RESULT catdv_display_get_edid(const CATDVLIB_DEVICE* device, const CATALYST_BYTE** bytes, CATALYST_NUINT* size);
```

**Parameters:**  
`device` — Valid borrowed device entry.  
`bytes` — Writable output for a borrowed immutable byte pointer.  
`size` — Writable byte count. Outputs do not overlap; both are zero on failure.

**Context:** `0x10`

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| 0x86100100 | System not supported | Not a display | — |
| 0xSS100200 | Native acquisition status | EDID unavailable, ambiguous, or acquisition failed | — |
| 0x00100300 | Success | Borrow EDID bytes and extent | — |
| 0x8510F000 | Not implemented | Target fallback | — |

### catdv_display_get_native_handle

```c
CATALYST_RESULT catdv_display_get_native_handle(const CATDVLIB_DEVICE* device, CATALYST_NUINT* handle);
```

**Parameters:**  
`device` — Valid borrowed device entry.  
`handle` — Writable pointer-width value; zero on failure. Cast to HMONITOR on Windows or CGDirectDisplayID on Darwin.

**Context:** `0x11`

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| 0x86110100 | System not supported | Not a display | — |
| 0x85110200 | Not implemented | No numeric native display identity | — |
| 0x00110300 | Success | Return borrowed native identity | — |
| 0x8511F000 | Not implemented | Target fallback | — |

### catdv_devices_find_identifier

```c
CATALYST_RESULT catdv_devices_find_identifier(const CATDVLIB_DEVICES* devices, const CATDVLIB_IDENTIFIER* identifier, const CATDVLIB_DEVICE** device);
```

**Parameters:**  
`devices` — Live snapshot.  
`identifier` — Valid nonnull, terminated scheme and instance value; exact byte comparison.  
`device` — Writable borrowed entry output; null on failure.

**Context:** `0x12`

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| 0xSS1201DD | Dependency status | Instance retrieval failed | Dependency detail |
| 0x82120200 | Invalid state | Multiple matching entries | — |
| 0x88120300 | Not found | No matching entry | — |
| 0x00120400 | Success | Return unique matching entry | — |

### catdv_devices_find_native_display

```c
CATALYST_RESULT catdv_devices_find_native_display(const CATDVLIB_DEVICES* devices, CATALYST_NUINT handle, const CATDVLIB_DEVICE** device);
```

**Parameters:**  
`devices` — Live snapshot.  
`handle` — Platform-native display identity as above.  
`device` — Writable borrowed entry output; null on failure.

**Context:** `0x13`

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| 0xSS1301DD | Dependency status | Native identity retrieval failed | Dependency detail |
| 0x82130200 | Invalid state | Multiple matching endpoints | — |
| 0x88130300 | Not found | No matching numeric handle | — |
| 0x00130400 | Success | Return unique matching display | — |
| 0x85130500 | Not implemented | No supported numeric display handle on this target | — |
