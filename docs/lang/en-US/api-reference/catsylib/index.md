# CATSYLIB

System identification.

**Headers:** `catsylib.h`, `catsylib.hpp`  
**C++ namespace:** `catalyst`  
**Library:** `catsy`  
**Dependencies:** Catalyst Core headers; native Windows registry/system APIs or POSIX system APIs and native iconv. No Core binary is required.

## Usage Notes

All operations retrieve a null-terminated UTF-8 string. The caller owns the destination; the library may use temporary native storage internally. `capacity` and `required` count UTF-8 code units including the terminator, not Unicode codepoints. `required` must point to writable, non-overlapping storage. Pass a null output and zero capacity to query the required size. Passing null with nonzero capacity is outside the contract. Otherwise output must identify writable storage of the specified capacity. A short buffer is not modified. On other failures, `required` is zero except when a cleanup failure follows a short-buffer result; the destination must not be used. An unrepresentable required size returns buffer overflow with zero required. Information can change between queries; retry with the newly reported capacity as appropriate.

The implementation does not invent missing system data. In particular Linux distribution information comes from `/etc/os-release` (or `/usr/lib/os-release` if absent), using NAME and VERSION_ID. Rolling distributions need not provide VERSION_ID. Windows distribution data uses ProductName and DisplayVersion (ReleaseId fallback); Windows 11 workstation branding is corrected when its compatibility ProductName retains Windows 10. macOS uses kern.osproductversion and the Darwin system release. The architecture is the native system-reported architecture, not the executable ABI; Linux UTS namespaces and execution personalities can affect reported values. On translated macOS processes, the Rosetta indicator is used to report arm64. Common architecture names are normalized to x86, x86_64, arm64; other POSIX names are retained. Windows uses IsWow64Process2 when available and GetNativeSystemInfo otherwise; older native APIs may not distinguish every emulation environment. Windows returns the DNS hostname; POSIX returns the UTS node name. These are not stable hardware identifiers.

## Summary

- [`catsy_system_get_name`](#catsy_system_get_name) — Retrieve the operating system name.
- [`catsy_system_get_version`](#catsy_system_get_version) — Retrieve the operating system version.
- [`catsy_system_get_distribution`](#catsy_system_get_distribution) — Retrieve the operating system distribution name.
- [`catsy_system_get_distribution_version`](#catsy_system_get_distribution_version) — Retrieve the operating system distribution version.
- [`catsy_system_get_architecture`](#catsy_system_get_architecture) — Retrieve the native system architecture.
- [`catsy_system_get_hostname`](#catsy_system_get_hostname) — Retrieve the system hostname.

## Functions

### catsy_system_get_name

```c
CATALYST_RESULT catsy_system_get_name(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);
```

Retrieve the operating system name.

**Parameters:**

- `output` — Caller-owned UTF-8 storage, or null for a size query.
- `capacity` — Writable UTF-8 code units including the terminator; zero for a size query.
- `required` — Writable required capacity, including the terminator. Must not overlap output.

**Context:** `0x01`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00010100` | SUCCESS | Required size retrieved | — |
| `0x00010200` | SUCCESS | Output copied | — |
| `0x90010300` | ERROR_BUFFER_OVERFLOW | Short output buffer | — |
| `0x88010600` | ERROR_NOT_FOUND | Empty native value | — |
| `0x90010700` | ERROR_BUFFER_OVERFLOW | Unrepresentable required extent | — |
| `0x84010800` | ERROR_ACCESS_DENIED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x88010800` | ERROR_NOT_FOUND | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x86010800` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x89010800` | ERROR_INTERRUPTED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x91010800` | ERROR_ALLOCATION_FAILED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0xA0010800` | ERROR_IO_ERROR | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x83010800` | ERROR_MALFORMED_INPUT | Windows UTF-8 size query | — |
| `0x84010900` | ERROR_ACCESS_DENIED | POSIX converter cleanup | — |
| `0x88010900` | ERROR_NOT_FOUND | POSIX converter cleanup | — |
| `0x86010900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter cleanup | — |
| `0x89010900` | ERROR_INTERRUPTED | POSIX converter cleanup | — |
| `0x91010900` | ERROR_ALLOCATION_FAILED | POSIX converter cleanup | — |
| `0xA0010900` | ERROR_IO_ERROR | POSIX converter cleanup | — |
| `0x83010A00` | ERROR_MALFORMED_INPUT | Invalid native UTF-8; Windows UTF-8 conversion | — |
| `0x84010A00` | ERROR_ACCESS_DENIED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x88010A00` | ERROR_NOT_FOUND | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x86010A00` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x89010A00` | ERROR_INTERRUPTED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x91010A00` | ERROR_ALLOCATION_FAILED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0xA0010A00` | ERROR_IO_ERROR | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x84011900` | ERROR_ACCESS_DENIED | POSIX uname | — |
| `0x88011900` | ERROR_NOT_FOUND | POSIX uname | — |
| `0x86011900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX uname | — |
| `0x89011900` | ERROR_INTERRUPTED | POSIX uname | — |
| `0x91011900` | ERROR_ALLOCATION_FAILED | POSIX uname | — |
| `0xA0011900` | ERROR_IO_ERROR | POSIX uname | — |

C++ equivalent: `system_get_name`.

### catsy_system_get_version

```c
CATALYST_RESULT catsy_system_get_version(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);
```

Retrieve the operating system version.

**Parameters:**

- `output` — Caller-owned UTF-8 storage, or null for a size query.
- `capacity` — Writable UTF-8 code units including the terminator; zero for a size query.
- `required` — Writable required capacity, including the terminator. Must not overlap output.

**Context:** `0x02`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00020100` | SUCCESS | Required size retrieved | — |
| `0x00020200` | SUCCESS | Output copied | — |
| `0x90020300` | ERROR_BUFFER_OVERFLOW | Short output buffer | — |
| `0x88020600` | ERROR_NOT_FOUND | Empty native value | — |
| `0x90020700` | ERROR_BUFFER_OVERFLOW | Unrepresentable required extent | — |
| `0x84020800` | ERROR_ACCESS_DENIED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x88020800` | ERROR_NOT_FOUND | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x86020800` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x89020800` | ERROR_INTERRUPTED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x91020800` | ERROR_ALLOCATION_FAILED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0xA0020800` | ERROR_IO_ERROR | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x83020800` | ERROR_MALFORMED_INPUT | Windows UTF-8 size query | — |
| `0x84020900` | ERROR_ACCESS_DENIED | POSIX converter cleanup | — |
| `0x88020900` | ERROR_NOT_FOUND | POSIX converter cleanup | — |
| `0x86020900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter cleanup | — |
| `0x89020900` | ERROR_INTERRUPTED | POSIX converter cleanup | — |
| `0x91020900` | ERROR_ALLOCATION_FAILED | POSIX converter cleanup | — |
| `0xA0020900` | ERROR_IO_ERROR | POSIX converter cleanup | — |
| `0x83020A00` | ERROR_MALFORMED_INPUT | Invalid native UTF-8; Windows UTF-8 conversion | — |
| `0x84020A00` | ERROR_ACCESS_DENIED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x88020A00` | ERROR_NOT_FOUND | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x86020A00` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x89020A00` | ERROR_INTERRUPTED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x91020A00` | ERROR_ALLOCATION_FAILED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0xA0020A00` | ERROR_IO_ERROR | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x84021900` | ERROR_ACCESS_DENIED | POSIX uname | — |
| `0x88021900` | ERROR_NOT_FOUND | POSIX uname | — |
| `0x86021900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX uname | — |
| `0x89021900` | ERROR_INTERRUPTED | POSIX uname | — |
| `0x91021900` | ERROR_ALLOCATION_FAILED | POSIX uname | — |
| `0xA0021900` | ERROR_IO_ERROR | POSIX uname | — |
| `0x86021000` | ERROR_SYSTEM_NOT_SUPPORTED | Native version query unavailable; Native version module lookup | — |
| `0xA0021000` | ERROR_IO_ERROR | Native version query failed; Native version module lookup | — |
| `0x84021000` | ERROR_ACCESS_DENIED | Native version module lookup | — |
| `0x88021000` | ERROR_NOT_FOUND | Native version module lookup | — |
| `0x91021000` | ERROR_ALLOCATION_FAILED | Native version module lookup | — |
| `0x83021000` | ERROR_MALFORMED_INPUT | Native version module lookup | — |
| `0x89021000` | ERROR_INTERRUPTED | Native version module lookup | — |

C++ equivalent: `system_get_version`.

### catsy_system_get_distribution

```c
CATALYST_RESULT catsy_system_get_distribution(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);
```

Retrieve the operating system distribution name.

**Parameters:**

- `output` — Caller-owned UTF-8 storage, or null for a size query.
- `capacity` — Writable UTF-8 code units including the terminator; zero for a size query.
- `required` — Writable required capacity, including the terminator. Must not overlap output.

**Context:** `0x03`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00030100` | SUCCESS | Required size retrieved | — |
| `0x00030200` | SUCCESS | Output copied | — |
| `0x90030300` | ERROR_BUFFER_OVERFLOW | Short output buffer | — |
| `0x88030600` | ERROR_NOT_FOUND | Empty native value | — |
| `0x90030700` | ERROR_BUFFER_OVERFLOW | Unrepresentable required extent | — |
| `0x84030800` | ERROR_ACCESS_DENIED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x88030800` | ERROR_NOT_FOUND | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x86030800` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x89030800` | ERROR_INTERRUPTED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x91030800` | ERROR_ALLOCATION_FAILED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0xA0030800` | ERROR_IO_ERROR | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x83030800` | ERROR_MALFORMED_INPUT | Windows UTF-8 size query | — |
| `0x84030900` | ERROR_ACCESS_DENIED | POSIX converter cleanup | — |
| `0x88030900` | ERROR_NOT_FOUND | POSIX converter cleanup | — |
| `0x86030900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter cleanup | — |
| `0x89030900` | ERROR_INTERRUPTED | POSIX converter cleanup | — |
| `0x91030900` | ERROR_ALLOCATION_FAILED | POSIX converter cleanup | — |
| `0xA0030900` | ERROR_IO_ERROR | POSIX converter cleanup | — |
| `0x83030A00` | ERROR_MALFORMED_INPUT | Invalid native UTF-8; Windows UTF-8 conversion | — |
| `0x84030A00` | ERROR_ACCESS_DENIED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x88030A00` | ERROR_NOT_FOUND | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x86030A00` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x89030A00` | ERROR_INTERRUPTED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x91030A00` | ERROR_ALLOCATION_FAILED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0xA0030A00` | ERROR_IO_ERROR | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x84031000` | ERROR_ACCESS_DENIED | Linux distribution file open | — |
| `0x88031000` | ERROR_NOT_FOUND | Linux distribution file open | — |
| `0x86031000` | ERROR_SYSTEM_NOT_SUPPORTED | Linux distribution file open | — |
| `0x89031000` | ERROR_INTERRUPTED | Linux distribution file open | — |
| `0x91031000` | ERROR_ALLOCATION_FAILED | Linux distribution file open | — |
| `0xA0031000` | ERROR_IO_ERROR | Linux distribution file open | — |
| `0x84031100` | ERROR_ACCESS_DENIED | Linux distribution file read; Windows registry open | — |
| `0x88031100` | ERROR_NOT_FOUND | Linux distribution file read; Windows registry open | — |
| `0x86031100` | ERROR_SYSTEM_NOT_SUPPORTED | Linux distribution file read; Windows registry open | — |
| `0x89031100` | ERROR_INTERRUPTED | Linux distribution file read; Windows registry open | — |
| `0x91031100` | ERROR_ALLOCATION_FAILED | Linux distribution file read; Windows registry open | — |
| `0xA0031100` | ERROR_IO_ERROR | Linux distribution file read; Windows registry open | — |
| `0x83031100` | ERROR_MALFORMED_INPUT | Invalid Linux distribution assignment; Windows registry open | — |
| `0x84031200` | ERROR_ACCESS_DENIED | Linux distribution file close; Windows registry size query | — |
| `0x88031200` | ERROR_NOT_FOUND | Linux distribution file close; Windows registry size query | — |
| `0x86031200` | ERROR_SYSTEM_NOT_SUPPORTED | Linux distribution file close; Windows registry size query | — |
| `0x89031200` | ERROR_INTERRUPTED | Linux distribution file close; Windows registry size query | — |
| `0x91031200` | ERROR_ALLOCATION_FAILED | Linux distribution file close; Windows registry size query | — |
| `0xA0031200` | ERROR_IO_ERROR | Linux distribution file close; Windows registry size query | — |
| `0x88031300` | ERROR_NOT_FOUND | Linux distribution field absent | — |
| `0x83031200` | ERROR_MALFORMED_INPUT | Windows registry size query | — |
| `0x83031300` | ERROR_MALFORMED_INPUT | Invalid Windows registry value | — |
| `0x90031400` | ERROR_BUFFER_OVERFLOW | Native extent cannot be represented | — |
| `0x91031500` | ERROR_ALLOCATION_FAILED | Registry storage | — |
| `0x84031600` | ERROR_ACCESS_DENIED | Windows registry read or close | — |
| `0x88031600` | ERROR_NOT_FOUND | Windows registry read or close | — |
| `0x86031600` | ERROR_SYSTEM_NOT_SUPPORTED | Windows registry read or close | — |
| `0x91031600` | ERROR_ALLOCATION_FAILED | Windows registry read or close | — |
| `0x83031600` | ERROR_MALFORMED_INPUT | Windows registry read or close | — |
| `0x89031600` | ERROR_INTERRUPTED | Windows registry read or close | — |
| `0xA0031600` | ERROR_IO_ERROR | Windows registry read or close | — |
| `0x83031700` | ERROR_MALFORMED_INPUT | Registry value changed or has invalid extent | — |
| `0x83031F00` | ERROR_MALFORMED_INPUT | Embedded terminator in registry value | — |
| `0x82032000` | ERROR_INVALID_STATE | Registry storage cleanup failed | — |

C++ equivalent: `system_get_distribution`.

### catsy_system_get_distribution_version

```c
CATALYST_RESULT catsy_system_get_distribution_version(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);
```

Retrieve the operating system distribution version.

**Parameters:**

- `output` — Caller-owned UTF-8 storage, or null for a size query.
- `capacity` — Writable UTF-8 code units including the terminator; zero for a size query.
- `required` — Writable required capacity, including the terminator. Must not overlap output.

**Context:** `0x04`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00040100` | SUCCESS | Required size retrieved | — |
| `0x00040200` | SUCCESS | Output copied | — |
| `0x90040300` | ERROR_BUFFER_OVERFLOW | Short output buffer | — |
| `0x88040600` | ERROR_NOT_FOUND | Empty native value | — |
| `0x90040700` | ERROR_BUFFER_OVERFLOW | Unrepresentable required extent | — |
| `0x84040800` | ERROR_ACCESS_DENIED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x88040800` | ERROR_NOT_FOUND | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x86040800` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x89040800` | ERROR_INTERRUPTED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x91040800` | ERROR_ALLOCATION_FAILED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0xA0040800` | ERROR_IO_ERROR | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x83040800` | ERROR_MALFORMED_INPUT | Windows UTF-8 size query | — |
| `0x84040900` | ERROR_ACCESS_DENIED | POSIX converter cleanup | — |
| `0x88040900` | ERROR_NOT_FOUND | POSIX converter cleanup | — |
| `0x86040900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter cleanup | — |
| `0x89040900` | ERROR_INTERRUPTED | POSIX converter cleanup | — |
| `0x91040900` | ERROR_ALLOCATION_FAILED | POSIX converter cleanup | — |
| `0xA0040900` | ERROR_IO_ERROR | POSIX converter cleanup | — |
| `0x83040A00` | ERROR_MALFORMED_INPUT | Invalid native UTF-8; Windows UTF-8 conversion | — |
| `0x84040A00` | ERROR_ACCESS_DENIED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x88040A00` | ERROR_NOT_FOUND | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x86040A00` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x89040A00` | ERROR_INTERRUPTED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x91040A00` | ERROR_ALLOCATION_FAILED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0xA0040A00` | ERROR_IO_ERROR | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x84041000` | ERROR_ACCESS_DENIED | Linux distribution file open | — |
| `0x88041000` | ERROR_NOT_FOUND | Linux distribution file open | — |
| `0x86041000` | ERROR_SYSTEM_NOT_SUPPORTED | Linux distribution file open | — |
| `0x89041000` | ERROR_INTERRUPTED | Linux distribution file open | — |
| `0x91041000` | ERROR_ALLOCATION_FAILED | Linux distribution file open | — |
| `0xA0041000` | ERROR_IO_ERROR | Linux distribution file open | — |
| `0x84041100` | ERROR_ACCESS_DENIED | Linux distribution file read; Windows registry open | — |
| `0x88041100` | ERROR_NOT_FOUND | Linux distribution file read; Windows registry open | — |
| `0x86041100` | ERROR_SYSTEM_NOT_SUPPORTED | Linux distribution file read; Windows registry open | — |
| `0x89041100` | ERROR_INTERRUPTED | Linux distribution file read; Windows registry open | — |
| `0x91041100` | ERROR_ALLOCATION_FAILED | Linux distribution file read; Windows registry open | — |
| `0xA0041100` | ERROR_IO_ERROR | Linux distribution file read; Windows registry open | — |
| `0x83041100` | ERROR_MALFORMED_INPUT | Invalid Linux distribution assignment; Windows registry open | — |
| `0x84041200` | ERROR_ACCESS_DENIED | Linux distribution file close; Windows registry size query | — |
| `0x88041200` | ERROR_NOT_FOUND | Linux distribution file close; Windows registry size query | — |
| `0x86041200` | ERROR_SYSTEM_NOT_SUPPORTED | Linux distribution file close; Windows registry size query | — |
| `0x89041200` | ERROR_INTERRUPTED | Linux distribution file close; Windows registry size query | — |
| `0x91041200` | ERROR_ALLOCATION_FAILED | Linux distribution file close; Windows registry size query | — |
| `0xA0041200` | ERROR_IO_ERROR | Linux distribution file close; Windows registry size query | — |
| `0x88041300` | ERROR_NOT_FOUND | Linux distribution field absent | — |
| `0x83041200` | ERROR_MALFORMED_INPUT | Windows registry size query | — |
| `0x83041300` | ERROR_MALFORMED_INPUT | Invalid Windows registry value | — |
| `0x90041400` | ERROR_BUFFER_OVERFLOW | Native extent cannot be represented | — |
| `0x91041500` | ERROR_ALLOCATION_FAILED | Registry storage | — |
| `0x84041600` | ERROR_ACCESS_DENIED | Windows registry read or close | — |
| `0x88041600` | ERROR_NOT_FOUND | Windows registry read or close | — |
| `0x86041600` | ERROR_SYSTEM_NOT_SUPPORTED | Windows registry read or close | — |
| `0x91041600` | ERROR_ALLOCATION_FAILED | Windows registry read or close; Darwin product version storage | — |
| `0x83041600` | ERROR_MALFORMED_INPUT | Windows registry read or close | — |
| `0x89041600` | ERROR_INTERRUPTED | Windows registry read or close | — |
| `0xA0041600` | ERROR_IO_ERROR | Windows registry read or close | — |
| `0x83041700` | ERROR_MALFORMED_INPUT | Registry value changed or has invalid extent | — |
| `0x83041F00` | ERROR_MALFORMED_INPUT | Embedded terminator in registry value | — |
| `0x82042000` | ERROR_INVALID_STATE | Registry storage cleanup failed | — |
| `0x84041400` | ERROR_ACCESS_DENIED | Darwin product version size query | — |
| `0x88041400` | ERROR_NOT_FOUND | Darwin product version size query | — |
| `0x86041400` | ERROR_SYSTEM_NOT_SUPPORTED | Darwin product version size query | — |
| `0x89041400` | ERROR_INTERRUPTED | Darwin product version size query | — |
| `0x91041400` | ERROR_ALLOCATION_FAILED | Darwin product version size query | — |
| `0xA0041400` | ERROR_IO_ERROR | Darwin product version size query | — |
| `0x88041500` | ERROR_NOT_FOUND | Empty Darwin product version | — |
| `0x84041700` | ERROR_ACCESS_DENIED | Darwin product version read | — |
| `0x88041700` | ERROR_NOT_FOUND | Darwin product version read | — |
| `0x86041700` | ERROR_SYSTEM_NOT_SUPPORTED | Darwin product version read | — |
| `0x89041700` | ERROR_INTERRUPTED | Darwin product version read | — |
| `0x91041700` | ERROR_ALLOCATION_FAILED | Darwin product version read | — |
| `0xA0041700` | ERROR_IO_ERROR | Darwin product version read | — |
| `0x83041800` | ERROR_MALFORMED_INPUT | Invalid Darwin product version string | — |

C++ equivalent: `system_get_distribution_version`.

### catsy_system_get_architecture

```c
CATALYST_RESULT catsy_system_get_architecture(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);
```

Retrieve the native system architecture.

**Parameters:**

- `output` — Caller-owned UTF-8 storage, or null for a size query.
- `capacity` — Writable UTF-8 code units including the terminator; zero for a size query.
- `required` — Writable required capacity, including the terminator. Must not overlap output.

**Context:** `0x05`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00050100` | SUCCESS | Required size retrieved | — |
| `0x00050200` | SUCCESS | Output copied | — |
| `0x90050300` | ERROR_BUFFER_OVERFLOW | Short output buffer | — |
| `0x88050600` | ERROR_NOT_FOUND | Empty native value | — |
| `0x90050700` | ERROR_BUFFER_OVERFLOW | Unrepresentable required extent | — |
| `0x84050800` | ERROR_ACCESS_DENIED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x88050800` | ERROR_NOT_FOUND | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x86050800` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x89050800` | ERROR_INTERRUPTED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x91050800` | ERROR_ALLOCATION_FAILED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0xA0050800` | ERROR_IO_ERROR | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x83050800` | ERROR_MALFORMED_INPUT | Windows UTF-8 size query | — |
| `0x84050900` | ERROR_ACCESS_DENIED | POSIX converter cleanup | — |
| `0x88050900` | ERROR_NOT_FOUND | POSIX converter cleanup | — |
| `0x86050900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter cleanup | — |
| `0x89050900` | ERROR_INTERRUPTED | POSIX converter cleanup | — |
| `0x91050900` | ERROR_ALLOCATION_FAILED | POSIX converter cleanup | — |
| `0xA0050900` | ERROR_IO_ERROR | POSIX converter cleanup | — |
| `0x83050A00` | ERROR_MALFORMED_INPUT | Invalid native UTF-8; Windows UTF-8 conversion | — |
| `0x84050A00` | ERROR_ACCESS_DENIED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x88050A00` | ERROR_NOT_FOUND | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x86050A00` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x89050A00` | ERROR_INTERRUPTED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x91050A00` | ERROR_ALLOCATION_FAILED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0xA0050A00` | ERROR_IO_ERROR | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x84051900` | ERROR_ACCESS_DENIED | POSIX uname | — |
| `0x88051900` | ERROR_NOT_FOUND | POSIX uname | — |
| `0x86051900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX uname | — |
| `0x89051900` | ERROR_INTERRUPTED | POSIX uname | — |
| `0x91051900` | ERROR_ALLOCATION_FAILED | POSIX uname | — |
| `0xA0051900` | ERROR_IO_ERROR | POSIX uname | — |
| `0x84051D00` | ERROR_ACCESS_DENIED | Windows native architecture query | — |
| `0x88051D00` | ERROR_NOT_FOUND | Windows native architecture query | — |
| `0x86051D00` | ERROR_SYSTEM_NOT_SUPPORTED | Windows native architecture query | — |
| `0x91051D00` | ERROR_ALLOCATION_FAILED | Windows native architecture query | — |
| `0x83051D00` | ERROR_MALFORMED_INPUT | Windows native architecture query | — |
| `0x89051D00` | ERROR_INTERRUPTED | Windows native architecture query | — |
| `0xA0051D00` | ERROR_IO_ERROR | Windows native architecture query | — |

C++ equivalent: `system_get_architecture`.

### catsy_system_get_hostname

```c
CATALYST_RESULT catsy_system_get_hostname(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);
```

Retrieve the system hostname.

**Parameters:**

- `output` — Caller-owned UTF-8 storage, or null for a size query.
- `capacity` — Writable UTF-8 code units including the terminator; zero for a size query.
- `required` — Writable required capacity, including the terminator. Must not overlap output.

**Context:** `0x06`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00060100` | SUCCESS | Required size retrieved | — |
| `0x00060200` | SUCCESS | Output copied | — |
| `0x90060300` | ERROR_BUFFER_OVERFLOW | Short output buffer | — |
| `0x88060600` | ERROR_NOT_FOUND | Empty native value | — |
| `0x90060700` | ERROR_BUFFER_OVERFLOW | Unrepresentable required extent | — |
| `0x84060800` | ERROR_ACCESS_DENIED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x88060800` | ERROR_NOT_FOUND | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x86060800` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x89060800` | ERROR_INTERRUPTED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x91060800` | ERROR_ALLOCATION_FAILED | POSIX converter initialization; Windows UTF-8 size query | — |
| `0xA0060800` | ERROR_IO_ERROR | POSIX converter initialization; Windows UTF-8 size query | — |
| `0x83060800` | ERROR_MALFORMED_INPUT | Windows UTF-8 size query | — |
| `0x84060900` | ERROR_ACCESS_DENIED | POSIX converter cleanup | — |
| `0x88060900` | ERROR_NOT_FOUND | POSIX converter cleanup | — |
| `0x86060900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter cleanup | — |
| `0x89060900` | ERROR_INTERRUPTED | POSIX converter cleanup | — |
| `0x91060900` | ERROR_ALLOCATION_FAILED | POSIX converter cleanup | — |
| `0xA0060900` | ERROR_IO_ERROR | POSIX converter cleanup | — |
| `0x83060A00` | ERROR_MALFORMED_INPUT | Invalid native UTF-8; Windows UTF-8 conversion | — |
| `0x84060A00` | ERROR_ACCESS_DENIED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x88060A00` | ERROR_NOT_FOUND | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x86060A00` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x89060A00` | ERROR_INTERRUPTED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x91060A00` | ERROR_ALLOCATION_FAILED | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0xA0060A00` | ERROR_IO_ERROR | POSIX conversion failure; Windows UTF-8 conversion | — |
| `0x84061900` | ERROR_ACCESS_DENIED | POSIX uname | — |
| `0x88061900` | ERROR_NOT_FOUND | POSIX uname; Empty Windows hostname | — |
| `0x86061900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX uname | — |
| `0x89061900` | ERROR_INTERRUPTED | POSIX uname | — |
| `0x91061900` | ERROR_ALLOCATION_FAILED | POSIX uname | — |
| `0xA0061900` | ERROR_IO_ERROR | POSIX uname | — |
| `0x84061800` | ERROR_ACCESS_DENIED | Windows hostname size query | — |
| `0x88061800` | ERROR_NOT_FOUND | Windows hostname size query | — |
| `0x86061800` | ERROR_SYSTEM_NOT_SUPPORTED | Windows hostname size query | — |
| `0x91061800` | ERROR_ALLOCATION_FAILED | Windows hostname size query | — |
| `0x83061800` | ERROR_MALFORMED_INPUT | Windows hostname size query | — |
| `0x89061800` | ERROR_INTERRUPTED | Windows hostname size query | — |
| `0xA0061800` | ERROR_IO_ERROR | Windows hostname size query | — |
| `0x90061A00` | ERROR_BUFFER_OVERFLOW | Native extent cannot be represented | — |
| `0x91061B00` | ERROR_ALLOCATION_FAILED | Hostname storage | — |
| `0x84061C00` | ERROR_ACCESS_DENIED | Windows hostname retrieval | — |
| `0x88061C00` | ERROR_NOT_FOUND | Windows hostname retrieval | — |
| `0x86061C00` | ERROR_SYSTEM_NOT_SUPPORTED | Windows hostname retrieval | — |
| `0x91061C00` | ERROR_ALLOCATION_FAILED | Windows hostname retrieval | — |
| `0x83061C00` | ERROR_MALFORMED_INPUT | Windows hostname retrieval | — |
| `0x89061C00` | ERROR_INTERRUPTED | Windows hostname retrieval | — |
| `0xA0061C00` | ERROR_IO_ERROR | Windows hostname retrieval | — |
| `0x82062100` | ERROR_INVALID_STATE | Hostname storage cleanup failed | — |

C++ equivalent: `system_get_hostname`.
