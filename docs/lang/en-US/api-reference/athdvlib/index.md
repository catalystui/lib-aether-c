# ATHDVLIB

Provides native device and display discovery, identification, and interoperability.

**C Header:** `athdvlib.h`  
**C++ Header:** `athdvlib.hpp`  
**Namespace:** `catalyst::device`  
**Library:** `athdv`  
**Dependencies:** CATALYST, CATMMLIB, CATTELIB; platform-native system APIs.  
**Specification:** [FDEFSPEC](https://catalystui.org/specifications/languages/fdefspec/)

## Usage Notes

Production code uses Catalyst Core for memory operations and text validation. Linux native filesystem interfaces require the platform's POSIX/Linux ABI (normally supplied by libc); sysfs and `/proc/self/fd` must be accessible for device resolution. No general-purpose C/C++ standard-library string, formatting, allocation or conversion routines are called directly by these modules. This is not a freestanding or libc-free binary guarantee.

All pointers and indices must meet header preconditions; synchronize access externally. Device records, identifier strings, metadata, and EDID bytes are borrowed from a live snapshot. For Aether-wide requirements, see [Assumptions](../../assumptions/index.md).

Device snapshots are finite and retain metadata until freed. A Linux sysfs instance path identifies a kernel resource, not a permanent identity; hot-plug may invalidate or reuse it. Linux enumerates USB/PCI sysfs devices and evdev endpoints; capability discovery is available for evdev endpoints and connected DRM display connectors. Unknown properties return NOT_FOUND or the native error. Metadata exceeding 1023 bytes is reported as BUFFER_OVERFLOW. Connected DRM connectors are also enumerated as logical display endpoints. EDID is copied from the kernel's binary connector attribute, not parsed. HWIDs are not device instance identities. The `usb` and `pci` schemes use uppercase four-hex-digit `VID:PID` pairs, while `linux-input` uses `BUS:VID:PID` with four hex digits per field. The native `linux-modalias` scheme preserves the original sysfs text. USB interface entries retain the modalias rather than claiming the physical USB model identity. When both a bus/model identifier and a modalias exist, the bus/model identifier is first. If a snapshot free fails, retry on the same snapshot; do not use any old borrowed references.

### Windows and Darwin backend coverage

Windows enumerates present SetupAPI PnP device instances and HID interfaces as separate records. The `windows-pnp` and `windows-hid` instance schemes distinguish device instance IDs and interface paths. Up to sixteen native PnP hardware identifiers are preserved; an overflow is reported, not truncated. HID capabilities and manufacturer, product and serial properties are queried where the OS permits; generic PnP records have no inferred physical/logical classification or I/O capability. Darwin enumerates IOHIDManager devices and online CoreGraphics displays; HID records are identified using hexadecimal I/O registry entry IDs (`darwin-hid`), vendor/product IDs, and native HID properties. Input/output capability queries reflect native element direction only; they neither open a communication resource for callers nor interpret HID elements or transfer reports. Online CoreGraphics displays are enumerated separately from HID endpoints, and legacy public IOKit display information supplies EDID only when the native vendor/product/serial association is unambiguous. General Darwin USB/PCI enumeration, persistent identity, and arbitrary native-handle import remain unsupported. Windows and Darwin snapshots use Core allocation and native SDK interfaces.

No third-party runtime is introduced. Windows uses SetupAPI, HID, User32, and registry APIs; macOS uses IOKit, CoreFoundation, and CoreGraphics. macOS ARM64 manual tests verified HID discovery, display identity and independent primary-display handle lookup. EDID was unavailable for the tested built-in display; successful EDID retrieval and Windows/Linux display hardware execution remain unverified.

### Display identification and native interoperability

Display endpoints are included in the same device snapshot. They are logical system records, not an assertion that each record is a distinct physical panel. They expose output capability without opening an output communication resource. No geometry, modes, rendering, windowing, EDID interpretation, DDC/CI, or private SDK APIs are included.

- Windows: active monitor interfaces associated with Win32 monitor handles. `windows-monitor` identifies the exact monitor interface path; `windows-display-source` is used when no active monitor interface is exposed. SetupAPI opens the corresponding device registry key and reads its binary EDID value. Several monitor endpoints may share one HMONITOR (for example, cloned displays).
- Darwin: online CGDirectDisplayID values identify `darwin-display` entries (eight uppercase hexadecimal digits). The model identifier uses the `darwin-display-model` scheme, with eight-digit vendor and product fields separated by a colon. A nonzero numeric CoreGraphics serial is exposed in hexadecimal. EDID comes from public IOKit IODisplayConnect dictionaries only when a single native association can be established; matching uses vendor, product and available serial values. Duplicate/ambiguous associations are reported rather than guessed. On Apple Silicon, including built-in panels, the public legacy property may be unavailable. No synthetic EDID or private AVService access is used.
- Linux: connected DRM connectors are exposed as `linux-sysfs` canonical instance paths. The binary EDID attribute is read to EOF, with a 32768-byte bound; larger values report overflow. There is no single process-independent numeric X11/Wayland/DRM display handle. Use exact canonical connector identifiers for lookup. No libdrm, X11 or Wayland dependency is introduced.

EDID bytes are copied as supplied by the operating system, including extensions, and remain valid until snapshot release. No checksum or descriptor decoding is performed. Hardware identification derived from EDID contents belongs outside this acquisition interface. Native handles are borrowed numeric identities at the time of enumeration, not acquired resources. Refresh the snapshot after display reconfiguration or hotplug; a handle may be invalidated or reused. Never close a native handle obtained here.

## Summary

| Type or macro | Meaning |
| --- | --- |
| [ATHDVLIB_API](#athdvlib_api) | Native export/import annotation. |
| [ATHDVLIB_DEVICE](#athdvlib_device-and-athdvlib_devices) | Borrowed device record. |
| [ATHDVLIB_DEVICES](#athdvlib_device-and-athdvlib_devices) | Owned snapshot. |
| [ATHDVLIB_IDENTIFIER](#athdvlib_identifier) | Scheme-qualified identifier. |
| [ATHDVLIB_DEVICE_TYPE](#athdvlib_device_type) | Physical/logical classification. |

| Function | Behavior |
| --- | --- |
| [athdv_devices_enumerate](#athdv_devices_enumerate) | Acquire a device snapshot. |
| [athdv_devices_free](#athdv_devices_free) | Release a snapshot. |
| [athdv_devices_get_count](#athdv_devices_get_count) | Count entries. |
| [athdv_devices_get](#athdv_devices_get) | Borrow an indexed entry. |
| [athdv_device_get_type](#athdv_device_get_type) | Retrieve device classification. |
| [athdv_device_get_identifier](#athdv_device_get_identifier) | Borrow an instance identifier. |
| [athdv_device_get_hwid_count](#athdv_device_get_hwid_count) | Count available HWIDs. |
| [athdv_device_get_hwid](#athdv_device_get_hwid) | Borrow an indexed HWID. |
| [athdv_device_get_serial](#athdv_device_get_serial) | Borrow the serial number. |
| [athdv_device_get_manufacturer](#athdv_device_get_manufacturer) | Borrow the manufacturer name. |
| [athdv_device_get_product](#athdv_device_get_product) | Borrow the product name. |
| [athdv_device_get_descriptor](#athdv_device_get_descriptor) | Borrow the descriptor. |
| [athdv_device_get_revision](#athdv_device_get_revision) | Borrow the revision. |
| [athdv_device_has_input](#athdv_device_has_input) | Query native input capability. |
| [athdv_device_has_output](#athdv_device_has_output) | Query native output capability. |
| [athdv_device_is_display](#athdv_device_is_display) | Identify a display endpoint. |
| [athdv_display_get_edid](#athdv_display_get_edid) | Borrow original EDID bytes. |
| [athdv_display_get_native_handle](#athdv_display_get_native_handle) | Retrieve a native display identity. |
| [athdv_devices_find_identifier](#athdv_devices_find_identifier) | Find an exact instance identifier. |
| [athdv_devices_find_native_display](#athdv_devices_find_native_display) | Resolve a native display identity. |

## Macros

### ATHDVLIB_API

Export/import annotation for the C ABI. On Windows, `ATHDVLIB_EXPORTS` selects `__declspec(dllexport)` and `ATHDVLIB_USE_DLL` selects `__declspec(dllimport)`; otherwise the annotation is empty.

## Types and constants

### ATHDVLIB_DEVICE and ATHDVLIB_DEVICES

Opaque device and snapshot representations. A device is borrowed from its owning snapshot; release the snapshot with `athdv_devices_free`.

```c
typedef struct ATHDVLIB_DEVICE ATHDVLIB_DEVICE;
typedef struct ATHDVLIB_DEVICES ATHDVLIB_DEVICES;
```

### ATHDVLIB_IDENTIFIER

A scheme-qualified native identity; neither field alone is a globally unique identifier.

```c
typedef struct ATHDVLIB_IDENTIFIER {
    CATALYST_ASCII scheme;
    CATALYST_UTF8 value;
} ATHDVLIB_IDENTIFIER;
```

**Fields:**  
`scheme` — Null-terminated ASCII scheme name.  
`value` — Null-terminated UTF-8 identifier value.

Returned strings are borrowed. For lookup, both strings must be valid and terminated.

### ATHDVLIB_DEVICE_TYPE

Byte-valued classification of the native device record.

```c
typedef CATALYST_BYTE ATHDVLIB_DEVICE_TYPE;
#define ATHDVLIB_DEVICE_UNKNOWN  0x00
#define ATHDVLIB_DEVICE_PHYSICAL 0x01
#define ATHDVLIB_DEVICE_LOGICAL  0x02
```

| Constant | Meaning |
| --- | --- |
| `ATHDVLIB_DEVICE_UNKNOWN` | No physical/logical classification established. |
| `ATHDVLIB_DEVICE_PHYSICAL` | Physical device record. |
| `ATHDVLIB_DEVICE_LOGICAL` | Logical device or interface record. |

The C++ wrappers use the same C ABI representations and forward to the C functions.

## Functions

### athdv_devices_enumerate

Enumerate a snapshot into an empty output slot; null on failure.

```c
CATALYST_RESULT athdv_devices_enumerate(ATHDVLIB_DEVICES** devices);
```

**Parameters:**  
`devices` — Writable output slot; receives the owned snapshot on success, null on failure.

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

### athdv_devices_free

Release a live snapshot; on partial cleanup failure retry using the same snapshot.

```c
CATALYST_RESULT athdv_devices_free(ATHDVLIB_DEVICES* devices);
```

**Parameters:**  
`devices` — Live owned snapshot. After a partial cleanup failure retry release; no borrowed entry or data may be used.

**Context:** `0x02`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0201DD` | Propagated native/dependency status | 0x01 | Dependency detail |
| `0xSS0202DD` | Snapshot allocation release status (including SUCCESS on Windows/Darwin) | Release snapshot | Dependency detail |
| `0x00020300` | SUCCESS | Linux snapshot released | — |
| `0xSS0204DD` | Dependency failure | Release owned EDID bytes | Dependency detail |
| `0x8502F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### athdv_devices_get_count

Return the number of entries in a live snapshot.

```c
CATALYST_NUINT athdv_devices_get_count(const ATHDVLIB_DEVICES* devices);
```

**Parameters:**  
`devices` — Borrowed live resource.

**Returns:** Return the number of entries in a live snapshot.

### athdv_devices_get

Borrow a device at a valid snapshot index until the snapshot is freed.

```c
const ATHDVLIB_DEVICE* athdv_devices_get(const ATHDVLIB_DEVICES* devices, CATALYST_NUINT index);
```

**Parameters:**  
`devices` — Borrowed live resource.  
`index` — Valid index in the reported collection.

**Returns:** Borrow a device at a valid snapshot index until the snapshot is freed.

### athdv_device_get_type

Return the device classification from the snapshot.

```c
ATHDVLIB_DEVICE_TYPE athdv_device_get_type(const ATHDVLIB_DEVICE* device);
```

**Parameters:**  
`device` — Borrowed live resource.

**Returns:** Return the device classification from the snapshot.

### athdv_device_get_identifier

Retrieve a borrowed system instance identifier; zero fields on failure.

```c
CATALYST_RESULT athdv_device_get_identifier(const ATHDVLIB_DEVICE* device, ATHDVLIB_IDENTIFIER* identifier);
```

**Parameters:**  
`device` — Borrowed live resource.  
`identifier` — Writable identifier output; both fields zero on failure.

**Context:** `0x06`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00060100` | SUCCESS | Linux instance path | — |
| `0xSS060100` | Stored native failure status | Windows/Darwin identifier unavailable | — |
| `0x00060200` | SUCCESS | Windows/Darwin instance identity | — |
| `0x8506F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### athdv_device_get_hwid_count

Retrieve the count of available hardware identifiers; zero on failure.

```c
CATALYST_RESULT athdv_device_get_hwid_count(const ATHDVLIB_DEVICE* device, CATALYST_NUINT* count);
```

**Parameters:**  
`device` — Borrowed live resource.  
`count` — Writable count output; zero on failure.

**Context:** `0x07`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS070100` | Propagated native/dependency status | 0x01 | — |
| `0x00070200` | SUCCESS | 0x02 | — |
| `0xSS070300` | Bus identifier retrieval error | 0x03 | — |
| `0x8507F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### athdv_device_get_hwid

Retrieve a borrowed hardware identifier at an index from the reported count; zero fields on failure.

```c
CATALYST_RESULT athdv_device_get_hwid(const ATHDVLIB_DEVICE* device, CATALYST_NUINT index, ATHDVLIB_IDENTIFIER* hwid);
```

**Parameters:**  
`device` — Borrowed live resource.  
`index` — Valid index in the reported collection.  
`hwid` — Writable borrowed identifier output; zero fields on failure.

**Context:** `0x08`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00080100` | SUCCESS | 0x01 (native modalias) | — |
| `0x00080200` | SUCCESS | 0x02 (bus/model identifier) | — |
| `0x8508F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### athdv_device_get_serial

Retrieve the borrowed manufacturer-assigned serial number; null on failure.

```c
CATALYST_RESULT athdv_device_get_serial(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* serial);
```

**Parameters:**  
`device` — Borrowed live resource.  
`serial` — Writable borrowed UTF-8 pointer output; null on failure.

**Context:** `0x09`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS090100` | Propagated native/dependency status | 0x01 | — |
| `0x00090200` | SUCCESS | 0x02 | — |
| `0x8509F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### athdv_device_get_manufacturer

Retrieve the borrowed manufacturer name; null on failure.

```c
CATALYST_RESULT athdv_device_get_manufacturer(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* manufacturer);
```

**Parameters:**  
`device` — Borrowed live resource.  
`manufacturer` — Writable borrowed UTF-8 pointer output; null on failure.

**Context:** `0x0A`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0A0100` | Propagated native/dependency status | 0x01 | — |
| `0x000A0200` | SUCCESS | 0x02 | — |
| `0x850AF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### athdv_device_get_product

Retrieve the borrowed product name; null on failure.

```c
CATALYST_RESULT athdv_device_get_product(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* product);
```

**Parameters:**  
`device` — Borrowed live resource.  
`product` — Writable borrowed UTF-8 pointer output; null on failure.

**Context:** `0x0B`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0B0100` | Propagated native/dependency status | 0x01 | — |
| `0x000B0200` | SUCCESS | 0x02 | — |
| `0x850BF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### athdv_device_get_descriptor

Retrieve the borrowed human-readable descriptor; null on failure.

```c
CATALYST_RESULT athdv_device_get_descriptor(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* descriptor);
```

**Parameters:**  
`device` — Borrowed live resource.  
`descriptor` — Writable borrowed UTF-8 pointer output; null on failure.

**Context:** `0x0C`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0C0100` | Propagated native/dependency status | 0x01 | — |
| `0x000C0200` | SUCCESS | 0x02 | — |
| `0x850CF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### athdv_device_get_revision

Retrieve the borrowed hardware revision; null on failure.

```c
CATALYST_RESULT athdv_device_get_revision(const ATHDVLIB_DEVICE* device, CATALYST_UTF8* revision);
```

**Parameters:**  
`device` — Borrowed live resource.  
`revision` — Writable borrowed UTF-8 pointer output; null on failure.

**Context:** `0x0D`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0D0100` | Propagated native/dependency status | 0x01 | — |
| `0x000D0200` | SUCCESS | 0x02 | — |
| `0x850DF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### athdv_device_has_input

Query whether the device exposes native input capabilities; false on failure.

```c
CATALYST_RESULT athdv_device_has_input(const ATHDVLIB_DEVICE* device, CATALYST_BOOL* supported);
```

**Parameters:**  
`device` — Borrowed live resource.  
`supported` — Writable Boolean output; false on failure.

**Context:** `0x0E`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x850E0100` | ERROR_NOT_IMPLEMENTED | 0x01 | — |
| `0xSS0E0200` | Propagated native/dependency status | 0x02 | — |
| `0x000E0300` | SUCCESS | 0x03 | — |
| `0x850EF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### athdv_device_has_output

Query whether the device exposes native output capabilities; false on failure.

```c
CATALYST_RESULT athdv_device_has_output(const ATHDVLIB_DEVICE* device, CATALYST_BOOL* supported);
```

**Parameters:**  
`device` — Borrowed live resource.  
`supported` — Writable Boolean output; false on failure.

**Context:** `0x0F`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x850F0100` | ERROR_NOT_IMPLEMENTED | 0x01 | — |
| `0xSS0F0200` | Propagated native/dependency status | 0x02 | — |
| `0x000F0300` | SUCCESS | 0x03 | — |
| `0x850FF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### athdv_device_is_display

```c
CATALYST_BOOL athdv_device_is_display(const ATHDVLIB_DEVICE* device);
```

**Parameters:**  
`device` — Valid borrowed device entry.

**Returns:** True if the entry represents a native display endpoint; false otherwise.

### athdv_display_get_edid

```c
CATALYST_RESULT athdv_display_get_edid(const ATHDVLIB_DEVICE* device, const CATALYST_BYTE** bytes, CATALYST_NUINT* size);
```

**Parameters:**  
`device` — Valid borrowed device entry.  
`bytes` — Writable output for a borrowed immutable byte pointer.  
`size` — Writable byte count. Outputs do not overlap; both are zero on failure.

**Context:** `0x10`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x86100100` | System not supported | Not a display | — |
| `0xSS100200` | Native acquisition status | EDID unavailable, ambiguous, or acquisition failed | — |
| `0x00100300` | Success | Borrow EDID bytes and extent | — |
| `0x8510F000` | Not implemented | Target fallback | — |

### athdv_display_get_native_handle

```c
CATALYST_RESULT athdv_display_get_native_handle(const ATHDVLIB_DEVICE* device, CATALYST_NUINT* handle);
```

**Parameters:**  
`device` — Valid borrowed device entry.  
`handle` — Writable pointer-width value; zero on failure. Cast to HMONITOR on Windows or CGDirectDisplayID on Darwin.

**Context:** `0x11`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x86110100` | System not supported | Not a display | — |
| `0x85110200` | Not implemented | No numeric native display identity | — |
| `0x00110300` | Success | Return borrowed native identity | — |
| `0x8511F000` | Not implemented | Target fallback | — |

### athdv_devices_find_identifier

```c
CATALYST_RESULT athdv_devices_find_identifier(const ATHDVLIB_DEVICES* devices, const ATHDVLIB_IDENTIFIER* identifier, const ATHDVLIB_DEVICE** device);
```

**Parameters:**  
`devices` — Live snapshot.  
`identifier` — Valid nonnull, terminated scheme and instance value; exact byte comparison.  
`device` — Writable borrowed entry output; null on failure.

**Context:** `0x12`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1201DD` | Dependency status | Instance retrieval failed | Dependency detail |
| `0x82120200` | Invalid state | Multiple matching entries | — |
| `0x88120300` | Not found | No matching entry | — |
| `0x00120400` | Success | Return unique matching entry | — |

### athdv_devices_find_native_display

```c
CATALYST_RESULT athdv_devices_find_native_display(const ATHDVLIB_DEVICES* devices, CATALYST_NUINT handle, const ATHDVLIB_DEVICE** device);
```

**Parameters:**  
`devices` — Live snapshot.  
`handle` — Platform-native display identity as above.  
`device` — Writable borrowed entry output; null on failure.

**Context:** `0x13`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1301DD` | Dependency status | Native identity retrieval failed | Dependency detail |
| `0x82130200` | Invalid state | Multiple matching endpoints | — |
| `0x88130300` | Not found | No matching numeric handle | — |
| `0x00130400` | Success | Return unique matching display | — |
| `0x85130500` | Not implemented | No supported numeric display handle on this target | — |
