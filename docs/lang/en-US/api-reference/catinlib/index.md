# CATINLIB

**Headers:** `catinlib.h`, `catinlib.hpp`  
**Namespace:** `catalyst::input`  
**Library:** `catin`  
**Dependencies:** CATDVLIB, Catalyst Core (private memory implementation)

## Usage Notes

This initial implementation supports Linux evdev endpoints only. Other discovered native device kinds return ERROR_NOT_IMPLEMENTED when opened. Windows and Darwin return ERROR_NOT_IMPLEMENTED for reporting operations. No native objects are created there. All pointers and indices must meet header preconditions; synchronize access externally. Numeric values are native integer measurements, not normalized; 32-bit measurement APIs require CATALYST_SUPPORTS_32BIT.

A successful open owns a native descriptor and is independent of the snapshot. Raw reads return one Linux struct input_event in native layout (including timestamps and SYN events). Typed digital/relative reads examine the same head event; a different channel or SYN event returns SUCCESS_NOOP without consuming it. Use a raw read to advance past an unmatched event; no-op action/offset is not an event. SYN_DROPPED requires consumer resynchronization. Close consumes the descriptor even when Linux reports an error; memory-release failure is retryable.

## Types and constants

```c
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
```

## Functions

### catin_device_open

Acquire a live native input resource independently of the device snapshot; null on failure.

```c
CATALYST_RESULT catin_device_open(const CATDVLIB_DEVICE* device, CATINLIB_INPUT** input);
```

**Parameters:**  
`device` — Borrowed live resource.  
`input` — Valid resource or writable output slot as described above.

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

### catin_device_close

Release the resource; a failed memory release can be retried, but Linux close consumes the descriptor even on error.

```c
CATALYST_RESULT catin_device_close(CATINLIB_INPUT* input);
```

**Parameters:**  
`input` — Valid resource or writable output slot as described above.

**Context:** `0x02`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0201DD` | Propagated native/dependency status | 0x01 | Dependency detail |
| `0xSS020200` | Propagated native/dependency status | 0x02 | — |
| `0x00020300` | SUCCESS | 0x03 | — |
| `0x8502F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catin_device_read

Read at most one native event without blocking; capacity must accommodate a complete event, received is zero on failure or no data.

```c
CATALYST_RESULT catin_device_read(CATINLIB_INPUT* input, CATMMLIB_POINTERW data, CATALYST_NUINT capacity, CATALYST_NUINT* received);
```

**Parameters:**  
`input` — Valid resource or writable output slot as described above.  
`data` — Value in the declared representation.  
`capacity` — Writable byte capacity.  
`received` — Valid resource or writable output slot as described above.

**Context:** `0x03`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x90030100` | ERROR_BUFFER_OVERFLOW | 0x01 | — |
| `0xSS0302DD` | Propagated native/dependency status | 0x02 | Dependency detail |
| `0x00030300` | SUCCESS | 0x03 | — |
| `0x8503F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catin_device_get_channel_count

Retrieve the number of individually accessible channels; zero on failure.

```c
CATALYST_RESULT catin_device_get_channel_count(const CATINLIB_INPUT* input, CATALYST_NUINT* count);
```

**Parameters:**  
`input` — Borrowed live resource.  
`count` — Valid resource or writable output slot as described above.

**Context:** `0x04`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00040100` | SUCCESS | 0x01 | — |
| `0x8504F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catin_channel_get_type

Retrieve the type of a valid channel index; unknown on failure.

```c
CATALYST_RESULT catin_channel_get_type(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATINLIB_CHANNEL_TYPE* type);
```

**Parameters:**  
`input` — Borrowed live resource.  
`channel` — Valid resource-local channel index.  
`type` — Valid resource or writable output slot as described above.

**Context:** `0x05`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00050100` | SUCCESS | 0x01 | — |
| `0x8505F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catin_channel_get_identifier

Retrieve a channel's borrowed native identifier; zero fields on failure.

```c
CATALYST_RESULT catin_channel_get_identifier(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATDVLIB_IDENTIFIER* identifier);
```

**Parameters:**  
`input` — Borrowed live resource.  
`channel` — Valid resource-local channel index.  
`identifier` — Valid resource or writable output slot as described above.

**Context:** `0x06`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00060100` | SUCCESS | 0x01 | — |
| `0x8506F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catin_digital_get_state

Retrieve the current digital state; false on failure.

```c
CATALYST_RESULT catin_digital_get_state(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_BOOL* state);
```

**Parameters:**  
`input` — Borrowed live resource.  
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

### catin_digital_read

Consume the next occurrence only if it matches the requested channel; otherwise no-op without discarding it.

```c
CATALYST_RESULT catin_digital_read(CATINLIB_INPUT* input, CATALYST_NUINT channel, CATINLIB_DIGITAL_ACTION* action);
```

**Parameters:**  
`input` — Valid resource or writable output slot as described above.  
`channel` — Valid resource-local channel index.  
`action` — Valid resource or writable output slot as described above.

**Context:** `0x08`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x81080101` | ERROR_INVALID_ARGUMENT | 0x01 | 1 |
| `0xSS0802DD` | Propagated native/dependency status | 0x02 | Dependency detail |
| `0x01080300` | SUCCESS_NOOP | 0x03 | — |
| `0x83080400` | ERROR_MALFORMED_INPUT | 0x04 | — |
| `0x00080500` | SUCCESS | 0x05 | — |
| `0x8508F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catin_absolute_get_range

Retrieve native integer bounds; zero both on failure.

```c
CATALYST_RESULT catin_absolute_get_range(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* minimum, CATALYST_INT* maximum);
```

**Parameters:**  
`input` — Borrowed live resource.  
`channel` — Valid resource-local channel index.  
`minimum` — Valid resource or writable output slot as described above.  
`maximum` — Valid resource or writable output slot as described above.

**Context:** `0x09`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x81090101` | ERROR_INVALID_ARGUMENT | 0x01 | 1 |
| `0xSS090200` | Propagated native/dependency status | 0x02 | — |
| `0x00090300` | SUCCESS | 0x03 | — |
| `0x8509F000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catin_absolute_get_value

Retrieve native integer measurement; zero on failure.

```c
CATALYST_RESULT catin_absolute_get_value(const CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* value);
```

**Parameters:**  
`input` — Borrowed live resource.  
`channel` — Valid resource-local channel index.  
`value` — Valid resource or writable output slot as described above.

**Context:** `0x0A`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x810A0101` | ERROR_INVALID_ARGUMENT | 0x01 | 1 |
| `0xSS0A0200` | Propagated native/dependency status | 0x02 | — |
| `0x000A0300` | SUCCESS | 0x03 | — |
| `0x850AF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

### catin_relative_read

Consume the next relative occurrence only if it matches the channel; zero on no-op or failure.

```c
CATALYST_RESULT catin_relative_read(CATINLIB_INPUT* input, CATALYST_NUINT channel, CATALYST_INT* offset);
```

**Parameters:**  
`input` — Valid resource or writable output slot as described above.  
`channel` — Valid resource-local channel index.  
`offset` — Valid resource or writable output slot as described above.

**Context:** `0x0B`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x810B0101` | ERROR_INVALID_ARGUMENT | 0x01 | 1 |
| `0xSS0B02DD` | Propagated native/dependency status | 0x02 | Dependency detail |
| `0x010B0300` | SUCCESS_NOOP | 0x03 | — |
| `0x000B0400` | SUCCESS | 0x04 | — |
| `0x850BF000` | ERROR_NOT_IMPLEMENTED | Backend unavailable | — |

