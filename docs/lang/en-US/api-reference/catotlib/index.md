# CATOTLIB

**Headers:** `catotlib.h`, `catotlib.hpp`  
**Namespace:** `catalyst::output`  
**Library:** `catot`  
**Dependencies:** CATDVLIB, Catalyst Core (private memory implementation)

## Usage Notes

Production code uses Catalyst Core for memory operations and text validation. Linux native filesystem and evdev interfaces require the platform's POSIX/Linux ABI (normally supplied by libc); sysfs and `/proc/self/fd` must be accessible for device resolution. No general-purpose C/C++ standard-library string, formatting, allocation or conversion routines are called directly by these modules. This is not a freestanding or libc-free binary guarantee.

This initial implementation supports Linux evdev endpoints only. Other discovered native device kinds return ERROR_NOT_IMPLEMENTED when opened. Windows and Darwin return ERROR_NOT_IMPLEMENTED for reporting operations. No native objects are created there. All pointers and indices must meet header preconditions; synchronize access externally. Numeric values are native integer measurements, not normalized; 32-bit measurement APIs require CATALYST_SUPPORTS_32BIT.

A successful open owns a native descriptor independently of the snapshot. Only EV_LED channels and single EV_LED records are implemented; force feedback, sound, display and audio are not implemented. Raw writes require exactly sizeof(struct input_event), with valid LED code and 0/1 value; native timestamps are ignored. Absolute output is unsupported by this backend. Close consumes the descriptor even when Linux reports an error; memory-release failure is retryable.

## Types and constants

```c
typedef struct CATOTLIB_OUTPUT CATOTLIB_OUTPUT;
typedef CATALYST_BYTE CATOTLIB_CHANNEL_TYPE;
#define CATOTLIB_CHANNEL_UNKNOWN  0x00
#define CATOTLIB_CHANNEL_DIGITAL  0x01
#define CATOTLIB_CHANNEL_ABSOLUTE 0x02
```

## Functions

### catot_device_open

Acquire a live native output resource independently of the device snapshot; null on failure.

```c
CATALYST_RESULT catot_device_open(const CATDVLIB_DEVICE* device, CATOTLIB_OUTPUT** output);
```

**Parameters:**  
`device` — Borrowed live resource.  
`output` — Valid resource or writable output slot as described above.

**Context:** `0x01`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0101DD` | Propagated native/dependency status | 0x01 | Dependency detail |
| `0xSS010200` | Propagated native/dependency status | 0x02 | — |
| `0xSS0103DD` | Propagated native/dependency status | 0x03 | Dependency detail |
| `0xSS0104DD` | Propagated native/dependency status | 0x04 | Dependency detail |
| `0xSS010500` | Propagated native/dependency status | 0x05 | — |
| `0xSS0106DD` | Propagated native/dependency status | 0x06 | Dependency detail |
| `0x00010700` | SUCCESS | 0x07 | — |
| `0x8501F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catot_device_close

Release the resource; a failed memory release can be retried, but Linux close consumes the descriptor even on error.

```c
CATALYST_RESULT catot_device_close(CATOTLIB_OUTPUT* output);
```

**Parameters:**  
`output` — Valid resource or writable output slot as described above.

**Context:** `0x02`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0201DD` | Propagated native/dependency status | 0x01 | Dependency detail |
| `0xSS020200` | Propagated native/dependency status | 0x02 | — |
| `0x00020300` | SUCCESS | 0x03 | — |
| `0x8502F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catot_device_write

Submit exactly one native LED event without blocking; written is zero on failure or no progress.

```c
CATALYST_RESULT catot_device_write(CATOTLIB_OUTPUT* output, CATMMLIB_POINTER data, CATALYST_NUINT size, CATALYST_NUINT* written);
```

**Parameters:**  
`output` — Valid resource or writable output slot as described above.  
`data` — Value in the declared representation.  
`size` — Source byte extent.  
`written` — Valid resource or writable output slot as described above.

**Context:** `0x03`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x81030102` | ERROR_INVALID_ARGUMENT | 0x01 | 2 |
| `0x81030201` | ERROR_INVALID_ARGUMENT | 0x02 | 1 |
| `0x81030301` | ERROR_INVALID_ARGUMENT | 0x03 | 1 |
| `0xSS0304DD` | Propagated native/dependency status | 0x04 | Dependency detail |
| `0x00030500` | SUCCESS | 0x05 | — |
| `0x8503F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catot_device_get_channel_count

Retrieve the number of individually accessible channels; zero on failure.

```c
CATALYST_RESULT catot_device_get_channel_count(const CATOTLIB_OUTPUT* output, CATALYST_NUINT* count);
```

**Parameters:**  
`output` — Borrowed live resource.  
`count` — Valid resource or writable output slot as described above.

**Context:** `0x04`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00040100` | SUCCESS | 0x01 | — |
| `0x8504F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catot_channel_get_type

Retrieve the type of a valid channel index; unknown on failure.

```c
CATALYST_RESULT catot_channel_get_type(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATOTLIB_CHANNEL_TYPE* type);
```

**Parameters:**  
`output` — Borrowed live resource.  
`channel` — Valid resource-local channel index.  
`type` — Valid resource or writable output slot as described above.

**Context:** `0x05`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00050100` | SUCCESS | 0x01 | — |
| `0x8505F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catot_channel_get_identifier

Retrieve a channel's borrowed native identifier; zero fields on failure.

```c
CATALYST_RESULT catot_channel_get_identifier(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATDVLIB_IDENTIFIER* identifier);
```

**Parameters:**  
`output` — Borrowed live resource.  
`channel` — Valid resource-local channel index.  
`identifier` — Valid resource or writable output slot as described above.

**Context:** `0x06`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00060100` | SUCCESS | 0x01 | — |
| `0x8506F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catot_digital_get_state

Retrieve the current digital state; false on failure.

```c
CATALYST_RESULT catot_digital_get_state(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_BOOL* state);
```

**Parameters:**  
`output` — Borrowed live resource.  
`channel` — Valid resource-local channel index.  
`state` — Valid resource or writable output slot as described above.

**Context:** `0x07`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x81070101` | ERROR_INVALID_ARGUMENT | 0x01 | 1 |
| `0xSS070200` | Propagated native/dependency status | 0x02 | — |
| `0x00070300` | SUCCESS | 0x03 | — |
| `0x8507F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catot_digital_set_state

Assign a digital output state to a valid digital channel without blocking.

```c
CATALYST_RESULT catot_digital_set_state(CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_BOOL state);
```

**Parameters:**  
`output` — Valid resource or writable output slot as described above.  
`channel` — Valid resource-local channel index.  
`state` — Value in the declared representation.

**Context:** `0x08`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x81080101` | ERROR_INVALID_ARGUMENT | 0x01 | 1 |
| `0xSS0802DD` | Propagated native/dependency status | 0x02 | Dependency detail |
| `0x8508F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catot_absolute_get_range

Retrieve native integer bounds; zero both on failure.

```c
CATALYST_RESULT catot_absolute_get_range(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT* minimum, CATALYST_INT* maximum);
```

**Parameters:**  
`output` — Borrowed live resource.  
`channel` — Valid resource-local channel index.  
`minimum` — Valid resource or writable output slot as described above.  
`maximum` — Valid resource or writable output slot as described above.

**Context:** `0x09`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x86090100` | ERROR_SYSTEM_NOT_SUPPORTED | 0x01 | — |
| `0x8509F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catot_absolute_get_value

Retrieve native integer measurement; zero on failure.

```c
CATALYST_RESULT catot_absolute_get_value(const CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT* value);
```

**Parameters:**  
`output` — Borrowed live resource.  
`channel` — Valid resource-local channel index.  
`value` — Valid resource or writable output slot as described above.

**Context:** `0x0A`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x860A0100` | ERROR_SYSTEM_NOT_SUPPORTED | 0x01 | — |
| `0x850AF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catot_absolute_set_value

Assign a native absolute output value where supported.

```c
CATALYST_RESULT catot_absolute_set_value(CATOTLIB_OUTPUT* output, CATALYST_NUINT channel, CATALYST_INT value);
```

**Parameters:**  
`output` — Valid resource or writable output slot as described above.  
`channel` — Valid resource-local channel index.  
`value` — Value in the declared representation.

**Context:** `0x0B`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x860B0100` | ERROR_SYSTEM_NOT_SUPPORTED | 0x01 | — |
| `0x850BF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

