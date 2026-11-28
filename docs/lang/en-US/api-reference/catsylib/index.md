# CATSYLIB

Provides native system identification.

**C Header:** `catsylib.h`  
**C++ Header:** `catsylib.hpp`  
**Namespace:** `catalyst`  
**Library:** `catsy`  
**Dependencies:** Catalyst Core headers; Windows native system and registry APIs, or POSIX system APIs and native iconv.  
**Specification:** [FDEFSPEC](https://catalystui.org/specifications/languages/fdefspec/)

## Usage Notes

All queries return null-terminated UTF-8. The caller owns the destination; capacity and required size include the terminating zero and count code units, not codepoints. A null destination with zero capacity requests only the required size. Otherwise the destination must be writable for the specified capacity, and the required-size pointer must always be writable and non-overlapping. An insufficient destination is unchanged and the required capacity is returned; an unrepresentable extent or other failure sets required to zero. No output is valid on other failures. Values may change between invocations; callers may retry after a short-buffer result. No allocation ownership crosses the ABI.

Availability depends on the native system. The architecture describes the system-reported architecture, which may differ from the executable ABI and may be affected by Linux UTS namespaces or execution personalities. Windows uses IsWow64Process2 where available and GetNativeSystemInfo otherwise; the latter may not distinguish every emulation environment. Darwin queries the Rosetta indicator where available. These values are not stable hardware identifiers.

See [Assumptions](../../assumptions/index.md).

## Summary

| Macro | Meaning |
| --- | --- |
| [CATSYLIB_API](#catsylib_api) | Native export/import annotation. |

| Function | Behavior |
| --- | --- |
| [catsy_system_get_name](#catsy_system_get_name) | Retrieve operating system name. |
| [catsy_system_get_version](#catsy_system_get_version) | Retrieve operating system version. |
| [catsy_system_get_distribution](#catsy_system_get_distribution) | Retrieve operating system distribution name. |
| [catsy_system_get_distribution_version](#catsy_system_get_distribution_version) | Retrieve operating system distribution version. |
| [catsy_system_get_architecture](#catsy_system_get_architecture) | Retrieve native system architecture. |
| [catsy_system_get_hostname](#catsy_system_get_hostname) | Retrieve system hostname. |

## Macros

### CATSYLIB_API

On Windows, CATSYLIB_EXPORTS selects dllexport and CATSYLIB_USE_DLL selects dllimport; otherwise the annotation is empty. Static linking and non-Windows builds use an empty annotation.

## Functions

### catsy_system_get_name

```c
CATALYST_RESULT catsy_system_get_name(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);
```

Retrieve the operating system name. Windows reports Windows NT; POSIX uses uname.sysname.

**Parameters:**  
`output` — Writable UTF-8 storage, or null for a size query.  
`capacity` — Writable code units including the terminator; zero for a size query.  
`required` — Writable required capacity, including the terminator.

**Context:** `0x01`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00010100` | SUCCESS | POSIX size query | — |
| `0x00010200` | SUCCESS | POSIX output copied | — |
| `0x90010300` | ERROR_BUFFER_OVERFLOW | POSIX destination too small; required returned | — |
| `0x88010600` | ERROR_NOT_FOUND | POSIX empty native value | — |
| `0x90010700` | ERROR_BUFFER_OVERFLOW | POSIX required extent unrepresentable | — |
| `0x00014100` | SUCCESS | Windows size query | — |
| `0x00014200` | SUCCESS | Windows output copied | — |
| `0x90014300` | ERROR_BUFFER_OVERFLOW | Windows destination too small; required returned | — |
| `0x88014600` | ERROR_NOT_FOUND | Windows empty native value | — |
| `0x90014700` | ERROR_BUFFER_OVERFLOW | Windows required extent unrepresentable | — |
| `0x84010800` | ERROR_ACCESS_DENIED | POSIX converter initialization | — |
| `0x88010800` | ERROR_NOT_FOUND | POSIX converter initialization | — |
| `0x86010800` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter initialization | — |
| `0x89010800` | ERROR_INTERRUPTED | POSIX converter initialization | — |
| `0x91010800` | ERROR_ALLOCATION_FAILED | POSIX converter initialization | — |
| `0xA0010800` | ERROR_IO_ERROR | POSIX converter initialization | — |
| `0x84010900` | ERROR_ACCESS_DENIED | POSIX converter cleanup | — |
| `0x88010900` | ERROR_NOT_FOUND | POSIX converter cleanup | — |
| `0x86010900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter cleanup | — |
| `0x89010900` | ERROR_INTERRUPTED | POSIX converter cleanup | — |
| `0x91010900` | ERROR_ALLOCATION_FAILED | POSIX converter cleanup | — |
| `0xA0010900` | ERROR_IO_ERROR | POSIX converter cleanup | — |
| `0x83010A00` | ERROR_MALFORMED_INPUT | POSIX conversion or validation | — |
| `0x84010A00` | ERROR_ACCESS_DENIED | POSIX conversion or validation | — |
| `0x88010A00` | ERROR_NOT_FOUND | POSIX conversion or validation | — |
| `0x86010A00` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX conversion or validation | — |
| `0x89010A00` | ERROR_INTERRUPTED | POSIX conversion or validation | — |
| `0x91010A00` | ERROR_ALLOCATION_FAILED | POSIX conversion or validation | — |
| `0xA0010A00` | ERROR_IO_ERROR | POSIX conversion or validation | — |
| `0x84014800` | ERROR_ACCESS_DENIED | Windows UTF-8 size query | — |
| `0x88014800` | ERROR_NOT_FOUND | Windows UTF-8 size query | — |
| `0x86014800` | ERROR_SYSTEM_NOT_SUPPORTED | Windows UTF-8 size query | — |
| `0x91014800` | ERROR_ALLOCATION_FAILED | Windows UTF-8 size query | — |
| `0x83014800` | ERROR_MALFORMED_INPUT | Windows UTF-8 size query | — |
| `0x89014800` | ERROR_INTERRUPTED | Windows UTF-8 size query | — |
| `0xA0014800` | ERROR_IO_ERROR | Windows UTF-8 size query | — |
| `0x84014A00` | ERROR_ACCESS_DENIED | Windows UTF-8 conversion | — |
| `0x88014A00` | ERROR_NOT_FOUND | Windows UTF-8 conversion | — |
| `0x86014A00` | ERROR_SYSTEM_NOT_SUPPORTED | Windows UTF-8 conversion | — |
| `0x91014A00` | ERROR_ALLOCATION_FAILED | Windows UTF-8 conversion | — |
| `0x83014A00` | ERROR_MALFORMED_INPUT | Windows UTF-8 conversion | — |
| `0x89014A00` | ERROR_INTERRUPTED | Windows UTF-8 conversion | — |
| `0xA0014A00` | ERROR_IO_ERROR | Windows UTF-8 conversion | — |
| `0x84011900` | ERROR_ACCESS_DENIED | POSIX uname | — |
| `0x88011900` | ERROR_NOT_FOUND | POSIX uname | — |
| `0x86011900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX uname | — |
| `0x89011900` | ERROR_INTERRUPTED | POSIX uname | — |
| `0x91011900` | ERROR_ALLOCATION_FAILED | POSIX uname | — |
| `0xA0011900` | ERROR_IO_ERROR | POSIX uname | — |

**C++ equivalent:** `catalyst::system_get_name`

### catsy_system_get_version

```c
CATALYST_RESULT catsy_system_get_version(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);
```

Retrieve the operating system version. Windows retrieves the running NT major, minor and build numbers. POSIX uses uname.release.

**Parameters:**  
`output` — Writable UTF-8 storage, or null for a size query.  
`capacity` — Writable code units including the terminator; zero for a size query.  
`required` — Writable required capacity, including the terminator.

**Context:** `0x02`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00020100` | SUCCESS | POSIX size query | — |
| `0x00020200` | SUCCESS | POSIX output copied | — |
| `0x90020300` | ERROR_BUFFER_OVERFLOW | POSIX destination too small; required returned | — |
| `0x88020600` | ERROR_NOT_FOUND | POSIX empty native value | — |
| `0x90020700` | ERROR_BUFFER_OVERFLOW | POSIX required extent unrepresentable | — |
| `0x00024100` | SUCCESS | Windows size query | — |
| `0x00024200` | SUCCESS | Windows output copied | — |
| `0x90024300` | ERROR_BUFFER_OVERFLOW | Windows destination too small; required returned | — |
| `0x88024600` | ERROR_NOT_FOUND | Windows empty native value | — |
| `0x90024700` | ERROR_BUFFER_OVERFLOW | Windows required extent unrepresentable | — |
| `0x84020800` | ERROR_ACCESS_DENIED | POSIX converter initialization | — |
| `0x88020800` | ERROR_NOT_FOUND | POSIX converter initialization | — |
| `0x86020800` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter initialization | — |
| `0x89020800` | ERROR_INTERRUPTED | POSIX converter initialization | — |
| `0x91020800` | ERROR_ALLOCATION_FAILED | POSIX converter initialization | — |
| `0xA0020800` | ERROR_IO_ERROR | POSIX converter initialization | — |
| `0x84020900` | ERROR_ACCESS_DENIED | POSIX converter cleanup | — |
| `0x88020900` | ERROR_NOT_FOUND | POSIX converter cleanup | — |
| `0x86020900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter cleanup | — |
| `0x89020900` | ERROR_INTERRUPTED | POSIX converter cleanup | — |
| `0x91020900` | ERROR_ALLOCATION_FAILED | POSIX converter cleanup | — |
| `0xA0020900` | ERROR_IO_ERROR | POSIX converter cleanup | — |
| `0x83020A00` | ERROR_MALFORMED_INPUT | POSIX conversion or validation | — |
| `0x84020A00` | ERROR_ACCESS_DENIED | POSIX conversion or validation | — |
| `0x88020A00` | ERROR_NOT_FOUND | POSIX conversion or validation | — |
| `0x86020A00` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX conversion or validation | — |
| `0x89020A00` | ERROR_INTERRUPTED | POSIX conversion or validation | — |
| `0x91020A00` | ERROR_ALLOCATION_FAILED | POSIX conversion or validation | — |
| `0xA0020A00` | ERROR_IO_ERROR | POSIX conversion or validation | — |
| `0x84024800` | ERROR_ACCESS_DENIED | Windows UTF-8 size query | — |
| `0x88024800` | ERROR_NOT_FOUND | Windows UTF-8 size query | — |
| `0x86024800` | ERROR_SYSTEM_NOT_SUPPORTED | Windows UTF-8 size query | — |
| `0x91024800` | ERROR_ALLOCATION_FAILED | Windows UTF-8 size query | — |
| `0x83024800` | ERROR_MALFORMED_INPUT | Windows UTF-8 size query | — |
| `0x89024800` | ERROR_INTERRUPTED | Windows UTF-8 size query | — |
| `0xA0024800` | ERROR_IO_ERROR | Windows UTF-8 size query | — |
| `0x84024A00` | ERROR_ACCESS_DENIED | Windows UTF-8 conversion | — |
| `0x88024A00` | ERROR_NOT_FOUND | Windows UTF-8 conversion | — |
| `0x86024A00` | ERROR_SYSTEM_NOT_SUPPORTED | Windows UTF-8 conversion | — |
| `0x91024A00` | ERROR_ALLOCATION_FAILED | Windows UTF-8 conversion | — |
| `0x83024A00` | ERROR_MALFORMED_INPUT | Windows UTF-8 conversion | — |
| `0x89024A00` | ERROR_INTERRUPTED | Windows UTF-8 conversion | — |
| `0xA0024A00` | ERROR_IO_ERROR | Windows UTF-8 conversion | — |
| `0x84021900` | ERROR_ACCESS_DENIED | POSIX uname | — |
| `0x88021900` | ERROR_NOT_FOUND | POSIX uname | — |
| `0x86021900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX uname | — |
| `0x89021900` | ERROR_INTERRUPTED | POSIX uname | — |
| `0x91021900` | ERROR_ALLOCATION_FAILED | POSIX uname | — |
| `0xA0021900` | ERROR_IO_ERROR | POSIX uname | — |
| `0x84025000` | ERROR_ACCESS_DENIED | Windows native version retrieval | — |
| `0x88025000` | ERROR_NOT_FOUND | Windows native version retrieval | — |
| `0x86025000` | ERROR_SYSTEM_NOT_SUPPORTED | Windows native version retrieval | — |
| `0x91025000` | ERROR_ALLOCATION_FAILED | Windows native version retrieval | — |
| `0x83025000` | ERROR_MALFORMED_INPUT | Windows native version retrieval | — |
| `0x89025000` | ERROR_INTERRUPTED | Windows native version retrieval | — |
| `0xA0025000` | ERROR_IO_ERROR | Windows native version retrieval | — |

**C++ equivalent:** `catalyst::system_get_version`

### catsy_system_get_distribution

```c
CATALYST_RESULT catsy_system_get_distribution(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);
```

Retrieve the operating system distribution name. Windows uses ProductName (correcting the Windows 10 compatibility name on known Windows 11 workstation builds); macOS reports macOS; Linux reads NAME from /etc/os-release or /usr/lib/os-release if absent.

**Parameters:**  
`output` — Writable UTF-8 storage, or null for a size query.  
`capacity` — Writable code units including the terminator; zero for a size query.  
`required` — Writable required capacity, including the terminator.

**Context:** `0x03`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00030100` | SUCCESS | POSIX size query | — |
| `0x00030200` | SUCCESS | POSIX output copied | — |
| `0x90030300` | ERROR_BUFFER_OVERFLOW | POSIX destination too small; required returned | — |
| `0x88030600` | ERROR_NOT_FOUND | POSIX empty native value | — |
| `0x90030700` | ERROR_BUFFER_OVERFLOW | POSIX required extent unrepresentable | — |
| `0x00034100` | SUCCESS | Windows size query | — |
| `0x00034200` | SUCCESS | Windows output copied | — |
| `0x90034300` | ERROR_BUFFER_OVERFLOW | Windows destination too small; required returned | — |
| `0x88034600` | ERROR_NOT_FOUND | Windows empty native value | — |
| `0x90034700` | ERROR_BUFFER_OVERFLOW | Windows required extent unrepresentable | — |
| `0x84030800` | ERROR_ACCESS_DENIED | POSIX converter initialization | — |
| `0x88030800` | ERROR_NOT_FOUND | POSIX converter initialization | — |
| `0x86030800` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter initialization | — |
| `0x89030800` | ERROR_INTERRUPTED | POSIX converter initialization | — |
| `0x91030800` | ERROR_ALLOCATION_FAILED | POSIX converter initialization | — |
| `0xA0030800` | ERROR_IO_ERROR | POSIX converter initialization | — |
| `0x84030900` | ERROR_ACCESS_DENIED | POSIX converter cleanup | — |
| `0x88030900` | ERROR_NOT_FOUND | POSIX converter cleanup | — |
| `0x86030900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter cleanup | — |
| `0x89030900` | ERROR_INTERRUPTED | POSIX converter cleanup | — |
| `0x91030900` | ERROR_ALLOCATION_FAILED | POSIX converter cleanup | — |
| `0xA0030900` | ERROR_IO_ERROR | POSIX converter cleanup | — |
| `0x83030A00` | ERROR_MALFORMED_INPUT | POSIX conversion or validation | — |
| `0x84030A00` | ERROR_ACCESS_DENIED | POSIX conversion or validation | — |
| `0x88030A00` | ERROR_NOT_FOUND | POSIX conversion or validation | — |
| `0x86030A00` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX conversion or validation | — |
| `0x89030A00` | ERROR_INTERRUPTED | POSIX conversion or validation | — |
| `0x91030A00` | ERROR_ALLOCATION_FAILED | POSIX conversion or validation | — |
| `0xA0030A00` | ERROR_IO_ERROR | POSIX conversion or validation | — |
| `0x84034800` | ERROR_ACCESS_DENIED | Windows UTF-8 size query | — |
| `0x88034800` | ERROR_NOT_FOUND | Windows UTF-8 size query | — |
| `0x86034800` | ERROR_SYSTEM_NOT_SUPPORTED | Windows UTF-8 size query | — |
| `0x91034800` | ERROR_ALLOCATION_FAILED | Windows UTF-8 size query | — |
| `0x83034800` | ERROR_MALFORMED_INPUT | Windows UTF-8 size query | — |
| `0x89034800` | ERROR_INTERRUPTED | Windows UTF-8 size query | — |
| `0xA0034800` | ERROR_IO_ERROR | Windows UTF-8 size query | — |
| `0x84034A00` | ERROR_ACCESS_DENIED | Windows UTF-8 conversion | — |
| `0x88034A00` | ERROR_NOT_FOUND | Windows UTF-8 conversion | — |
| `0x86034A00` | ERROR_SYSTEM_NOT_SUPPORTED | Windows UTF-8 conversion | — |
| `0x91034A00` | ERROR_ALLOCATION_FAILED | Windows UTF-8 conversion | — |
| `0x83034A00` | ERROR_MALFORMED_INPUT | Windows UTF-8 conversion | — |
| `0x89034A00` | ERROR_INTERRUPTED | Windows UTF-8 conversion | — |
| `0xA0034A00` | ERROR_IO_ERROR | Windows UTF-8 conversion | — |
| `0x84031000` | ERROR_ACCESS_DENIED | Linux distribution file open | — |
| `0x88031000` | ERROR_NOT_FOUND | Linux distribution file open | — |
| `0x86031000` | ERROR_SYSTEM_NOT_SUPPORTED | Linux distribution file open | — |
| `0x89031000` | ERROR_INTERRUPTED | Linux distribution file open | — |
| `0x91031000` | ERROR_ALLOCATION_FAILED | Linux distribution file open | — |
| `0xA0031000` | ERROR_IO_ERROR | Linux distribution file open | — |
| `0x83031100` | ERROR_MALFORMED_INPUT | Linux distribution file read or parsing | — |
| `0x84031100` | ERROR_ACCESS_DENIED | Linux distribution file read or parsing | — |
| `0x88031100` | ERROR_NOT_FOUND | Linux distribution file read or parsing | — |
| `0x86031100` | ERROR_SYSTEM_NOT_SUPPORTED | Linux distribution file read or parsing | — |
| `0x89031100` | ERROR_INTERRUPTED | Linux distribution file read or parsing | — |
| `0x91031100` | ERROR_ALLOCATION_FAILED | Linux distribution file read or parsing | — |
| `0xA0031100` | ERROR_IO_ERROR | Linux distribution file read or parsing | — |
| `0x84031200` | ERROR_ACCESS_DENIED | Linux distribution file close | — |
| `0x88031200` | ERROR_NOT_FOUND | Linux distribution file close | — |
| `0x86031200` | ERROR_SYSTEM_NOT_SUPPORTED | Linux distribution file close | — |
| `0x89031200` | ERROR_INTERRUPTED | Linux distribution file close | — |
| `0x91031200` | ERROR_ALLOCATION_FAILED | Linux distribution file close | — |
| `0xA0031200` | ERROR_IO_ERROR | Linux distribution file close | — |
| `0x88031300` | ERROR_NOT_FOUND | Linux distribution field absent | — |
| `0x84035100` | ERROR_ACCESS_DENIED | Windows registry open | — |
| `0x88035100` | ERROR_NOT_FOUND | Windows registry open | — |
| `0x86035100` | ERROR_SYSTEM_NOT_SUPPORTED | Windows registry open | — |
| `0x91035100` | ERROR_ALLOCATION_FAILED | Windows registry open | — |
| `0x83035100` | ERROR_MALFORMED_INPUT | Windows registry open | — |
| `0x89035100` | ERROR_INTERRUPTED | Windows registry open | — |
| `0xA0035100` | ERROR_IO_ERROR | Windows registry open | — |
| `0x84035200` | ERROR_ACCESS_DENIED | Windows registry size query | — |
| `0x88035200` | ERROR_NOT_FOUND | Windows registry size query | — |
| `0x86035200` | ERROR_SYSTEM_NOT_SUPPORTED | Windows registry size query | — |
| `0x91035200` | ERROR_ALLOCATION_FAILED | Windows registry size query | — |
| `0x83035200` | ERROR_MALFORMED_INPUT | Windows registry size query | — |
| `0x89035200` | ERROR_INTERRUPTED | Windows registry size query | — |
| `0xA0035200` | ERROR_IO_ERROR | Windows registry size query | — |
| `0x83035300` | ERROR_MALFORMED_INPUT | Windows registry value invalid | — |
| `0x90035400` | ERROR_BUFFER_OVERFLOW | Windows native extent unrepresentable | — |
| `0x91035500` | ERROR_ALLOCATION_FAILED | Windows registry allocation | — |
| `0x84035600` | ERROR_ACCESS_DENIED | Windows registry read or close | — |
| `0x88035600` | ERROR_NOT_FOUND | Windows registry read or close | — |
| `0x86035600` | ERROR_SYSTEM_NOT_SUPPORTED | Windows registry read or close | — |
| `0x91035600` | ERROR_ALLOCATION_FAILED | Windows registry read or close | — |
| `0x83035600` | ERROR_MALFORMED_INPUT | Windows registry read or close | — |
| `0x89035600` | ERROR_INTERRUPTED | Windows registry read or close | — |
| `0xA0035600` | ERROR_IO_ERROR | Windows registry read or close | — |
| `0x83035700` | ERROR_MALFORMED_INPUT | Windows registry extent or type changed | — |
| `0x83035F00` | ERROR_MALFORMED_INPUT | Windows embedded registry terminator | — |
| `0x82036000` | ERROR_INVALID_STATE | Windows registry storage release | — |

**C++ equivalent:** `catalyst::system_get_distribution`

### catsy_system_get_distribution_version

```c
CATALYST_RESULT catsy_system_get_distribution_version(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);
```

Retrieve the operating system distribution version. Windows uses DisplayVersion with ReleaseId fallback; macOS uses kern.osproductversion; Linux reads VERSION_ID. A rolling distribution may omit this field.

**Parameters:**  
`output` — Writable UTF-8 storage, or null for a size query.  
`capacity` — Writable code units including the terminator; zero for a size query.  
`required` — Writable required capacity, including the terminator.

**Context:** `0x04`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00040100` | SUCCESS | POSIX size query | — |
| `0x00040200` | SUCCESS | POSIX output copied | — |
| `0x90040300` | ERROR_BUFFER_OVERFLOW | POSIX destination too small; required returned | — |
| `0x88040600` | ERROR_NOT_FOUND | POSIX empty native value | — |
| `0x90040700` | ERROR_BUFFER_OVERFLOW | POSIX required extent unrepresentable | — |
| `0x00044100` | SUCCESS | Windows size query | — |
| `0x00044200` | SUCCESS | Windows output copied | — |
| `0x90044300` | ERROR_BUFFER_OVERFLOW | Windows destination too small; required returned | — |
| `0x88044600` | ERROR_NOT_FOUND | Windows empty native value | — |
| `0x90044700` | ERROR_BUFFER_OVERFLOW | Windows required extent unrepresentable | — |
| `0x84040800` | ERROR_ACCESS_DENIED | POSIX converter initialization | — |
| `0x88040800` | ERROR_NOT_FOUND | POSIX converter initialization | — |
| `0x86040800` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter initialization | — |
| `0x89040800` | ERROR_INTERRUPTED | POSIX converter initialization | — |
| `0x91040800` | ERROR_ALLOCATION_FAILED | POSIX converter initialization | — |
| `0xA0040800` | ERROR_IO_ERROR | POSIX converter initialization | — |
| `0x84040900` | ERROR_ACCESS_DENIED | POSIX converter cleanup | — |
| `0x88040900` | ERROR_NOT_FOUND | POSIX converter cleanup | — |
| `0x86040900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter cleanup | — |
| `0x89040900` | ERROR_INTERRUPTED | POSIX converter cleanup | — |
| `0x91040900` | ERROR_ALLOCATION_FAILED | POSIX converter cleanup | — |
| `0xA0040900` | ERROR_IO_ERROR | POSIX converter cleanup | — |
| `0x83040A00` | ERROR_MALFORMED_INPUT | POSIX conversion or validation | — |
| `0x84040A00` | ERROR_ACCESS_DENIED | POSIX conversion or validation | — |
| `0x88040A00` | ERROR_NOT_FOUND | POSIX conversion or validation | — |
| `0x86040A00` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX conversion or validation | — |
| `0x89040A00` | ERROR_INTERRUPTED | POSIX conversion or validation | — |
| `0x91040A00` | ERROR_ALLOCATION_FAILED | POSIX conversion or validation | — |
| `0xA0040A00` | ERROR_IO_ERROR | POSIX conversion or validation | — |
| `0x84044800` | ERROR_ACCESS_DENIED | Windows UTF-8 size query | — |
| `0x88044800` | ERROR_NOT_FOUND | Windows UTF-8 size query | — |
| `0x86044800` | ERROR_SYSTEM_NOT_SUPPORTED | Windows UTF-8 size query | — |
| `0x91044800` | ERROR_ALLOCATION_FAILED | Windows UTF-8 size query | — |
| `0x83044800` | ERROR_MALFORMED_INPUT | Windows UTF-8 size query | — |
| `0x89044800` | ERROR_INTERRUPTED | Windows UTF-8 size query | — |
| `0xA0044800` | ERROR_IO_ERROR | Windows UTF-8 size query | — |
| `0x84044A00` | ERROR_ACCESS_DENIED | Windows UTF-8 conversion | — |
| `0x88044A00` | ERROR_NOT_FOUND | Windows UTF-8 conversion | — |
| `0x86044A00` | ERROR_SYSTEM_NOT_SUPPORTED | Windows UTF-8 conversion | — |
| `0x91044A00` | ERROR_ALLOCATION_FAILED | Windows UTF-8 conversion | — |
| `0x83044A00` | ERROR_MALFORMED_INPUT | Windows UTF-8 conversion | — |
| `0x89044A00` | ERROR_INTERRUPTED | Windows UTF-8 conversion | — |
| `0xA0044A00` | ERROR_IO_ERROR | Windows UTF-8 conversion | — |
| `0x84041000` | ERROR_ACCESS_DENIED | Linux distribution file open | — |
| `0x88041000` | ERROR_NOT_FOUND | Linux distribution file open | — |
| `0x86041000` | ERROR_SYSTEM_NOT_SUPPORTED | Linux distribution file open | — |
| `0x89041000` | ERROR_INTERRUPTED | Linux distribution file open | — |
| `0x91041000` | ERROR_ALLOCATION_FAILED | Linux distribution file open | — |
| `0xA0041000` | ERROR_IO_ERROR | Linux distribution file open | — |
| `0x83041100` | ERROR_MALFORMED_INPUT | Linux distribution file read or parsing | — |
| `0x84041100` | ERROR_ACCESS_DENIED | Linux distribution file read or parsing | — |
| `0x88041100` | ERROR_NOT_FOUND | Linux distribution file read or parsing | — |
| `0x86041100` | ERROR_SYSTEM_NOT_SUPPORTED | Linux distribution file read or parsing | — |
| `0x89041100` | ERROR_INTERRUPTED | Linux distribution file read or parsing | — |
| `0x91041100` | ERROR_ALLOCATION_FAILED | Linux distribution file read or parsing | — |
| `0xA0041100` | ERROR_IO_ERROR | Linux distribution file read or parsing | — |
| `0x84041200` | ERROR_ACCESS_DENIED | Linux distribution file close | — |
| `0x88041200` | ERROR_NOT_FOUND | Linux distribution file close | — |
| `0x86041200` | ERROR_SYSTEM_NOT_SUPPORTED | Linux distribution file close | — |
| `0x89041200` | ERROR_INTERRUPTED | Linux distribution file close | — |
| `0x91041200` | ERROR_ALLOCATION_FAILED | Linux distribution file close | — |
| `0xA0041200` | ERROR_IO_ERROR | Linux distribution file close | — |
| `0x88041300` | ERROR_NOT_FOUND | Linux distribution field absent | — |
| `0x84045100` | ERROR_ACCESS_DENIED | Windows registry open | — |
| `0x88045100` | ERROR_NOT_FOUND | Windows registry open | — |
| `0x86045100` | ERROR_SYSTEM_NOT_SUPPORTED | Windows registry open | — |
| `0x91045100` | ERROR_ALLOCATION_FAILED | Windows registry open | — |
| `0x83045100` | ERROR_MALFORMED_INPUT | Windows registry open | — |
| `0x89045100` | ERROR_INTERRUPTED | Windows registry open | — |
| `0xA0045100` | ERROR_IO_ERROR | Windows registry open | — |
| `0x84045200` | ERROR_ACCESS_DENIED | Windows registry size query | — |
| `0x88045200` | ERROR_NOT_FOUND | Windows registry size query | — |
| `0x86045200` | ERROR_SYSTEM_NOT_SUPPORTED | Windows registry size query | — |
| `0x91045200` | ERROR_ALLOCATION_FAILED | Windows registry size query | — |
| `0x83045200` | ERROR_MALFORMED_INPUT | Windows registry size query | — |
| `0x89045200` | ERROR_INTERRUPTED | Windows registry size query | — |
| `0xA0045200` | ERROR_IO_ERROR | Windows registry size query | — |
| `0x83045300` | ERROR_MALFORMED_INPUT | Windows registry value invalid | — |
| `0x90045400` | ERROR_BUFFER_OVERFLOW | Windows native extent unrepresentable | — |
| `0x91045500` | ERROR_ALLOCATION_FAILED | Windows registry allocation | — |
| `0x84045600` | ERROR_ACCESS_DENIED | Windows registry read or close | — |
| `0x88045600` | ERROR_NOT_FOUND | Windows registry read or close | — |
| `0x86045600` | ERROR_SYSTEM_NOT_SUPPORTED | Windows registry read or close | — |
| `0x91045600` | ERROR_ALLOCATION_FAILED | Windows registry read or close | — |
| `0x83045600` | ERROR_MALFORMED_INPUT | Windows registry read or close | — |
| `0x89045600` | ERROR_INTERRUPTED | Windows registry read or close | — |
| `0xA0045600` | ERROR_IO_ERROR | Windows registry read or close | — |
| `0x83045700` | ERROR_MALFORMED_INPUT | Windows registry extent or type changed | — |
| `0x83045F00` | ERROR_MALFORMED_INPUT | Windows embedded registry terminator | — |
| `0x82046000` | ERROR_INVALID_STATE | Windows registry storage release | — |
| `0x84041400` | ERROR_ACCESS_DENIED | Darwin product version size query | — |
| `0x88041400` | ERROR_NOT_FOUND | Darwin product version size query | — |
| `0x86041400` | ERROR_SYSTEM_NOT_SUPPORTED | Darwin product version size query | — |
| `0x89041400` | ERROR_INTERRUPTED | Darwin product version size query | — |
| `0x91041400` | ERROR_ALLOCATION_FAILED | Darwin product version size query | — |
| `0xA0041400` | ERROR_IO_ERROR | Darwin product version size query | — |
| `0x88041500` | ERROR_NOT_FOUND | Darwin product version empty | — |
| `0x91041600` | ERROR_ALLOCATION_FAILED | Darwin product version allocation | — |
| `0x84041700` | ERROR_ACCESS_DENIED | Darwin product version retrieval | — |
| `0x88041700` | ERROR_NOT_FOUND | Darwin product version retrieval | — |
| `0x86041700` | ERROR_SYSTEM_NOT_SUPPORTED | Darwin product version retrieval | — |
| `0x89041700` | ERROR_INTERRUPTED | Darwin product version retrieval | — |
| `0x91041700` | ERROR_ALLOCATION_FAILED | Darwin product version retrieval | — |
| `0xA0041700` | ERROR_IO_ERROR | Darwin product version retrieval | — |
| `0x83041800` | ERROR_MALFORMED_INPUT | Darwin product version extent or termination invalid | — |

**C++ equivalent:** `catalyst::system_get_distribution_version`

### catsy_system_get_architecture

```c
CATALYST_RESULT catsy_system_get_architecture(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);
```

Retrieve the native system architecture. Common aliases are normalized to x86, x86_64 and arm64. Other POSIX names are retained.

**Parameters:**  
`output` — Writable UTF-8 storage, or null for a size query.  
`capacity` — Writable code units including the terminator; zero for a size query.  
`required` — Writable required capacity, including the terminator.

**Context:** `0x05`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00050100` | SUCCESS | POSIX size query | — |
| `0x00050200` | SUCCESS | POSIX output copied | — |
| `0x90050300` | ERROR_BUFFER_OVERFLOW | POSIX destination too small; required returned | — |
| `0x88050600` | ERROR_NOT_FOUND | POSIX empty native value | — |
| `0x90050700` | ERROR_BUFFER_OVERFLOW | POSIX required extent unrepresentable | — |
| `0x00054100` | SUCCESS | Windows size query | — |
| `0x00054200` | SUCCESS | Windows output copied | — |
| `0x90054300` | ERROR_BUFFER_OVERFLOW | Windows destination too small; required returned | — |
| `0x88054600` | ERROR_NOT_FOUND | Windows empty native value | — |
| `0x90054700` | ERROR_BUFFER_OVERFLOW | Windows required extent unrepresentable | — |
| `0x84050800` | ERROR_ACCESS_DENIED | POSIX converter initialization | — |
| `0x88050800` | ERROR_NOT_FOUND | POSIX converter initialization | — |
| `0x86050800` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter initialization | — |
| `0x89050800` | ERROR_INTERRUPTED | POSIX converter initialization | — |
| `0x91050800` | ERROR_ALLOCATION_FAILED | POSIX converter initialization | — |
| `0xA0050800` | ERROR_IO_ERROR | POSIX converter initialization | — |
| `0x84050900` | ERROR_ACCESS_DENIED | POSIX converter cleanup | — |
| `0x88050900` | ERROR_NOT_FOUND | POSIX converter cleanup | — |
| `0x86050900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter cleanup | — |
| `0x89050900` | ERROR_INTERRUPTED | POSIX converter cleanup | — |
| `0x91050900` | ERROR_ALLOCATION_FAILED | POSIX converter cleanup | — |
| `0xA0050900` | ERROR_IO_ERROR | POSIX converter cleanup | — |
| `0x83050A00` | ERROR_MALFORMED_INPUT | POSIX conversion or validation | — |
| `0x84050A00` | ERROR_ACCESS_DENIED | POSIX conversion or validation | — |
| `0x88050A00` | ERROR_NOT_FOUND | POSIX conversion or validation | — |
| `0x86050A00` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX conversion or validation | — |
| `0x89050A00` | ERROR_INTERRUPTED | POSIX conversion or validation | — |
| `0x91050A00` | ERROR_ALLOCATION_FAILED | POSIX conversion or validation | — |
| `0xA0050A00` | ERROR_IO_ERROR | POSIX conversion or validation | — |
| `0x84054800` | ERROR_ACCESS_DENIED | Windows UTF-8 size query | — |
| `0x88054800` | ERROR_NOT_FOUND | Windows UTF-8 size query | — |
| `0x86054800` | ERROR_SYSTEM_NOT_SUPPORTED | Windows UTF-8 size query | — |
| `0x91054800` | ERROR_ALLOCATION_FAILED | Windows UTF-8 size query | — |
| `0x83054800` | ERROR_MALFORMED_INPUT | Windows UTF-8 size query | — |
| `0x89054800` | ERROR_INTERRUPTED | Windows UTF-8 size query | — |
| `0xA0054800` | ERROR_IO_ERROR | Windows UTF-8 size query | — |
| `0x84054A00` | ERROR_ACCESS_DENIED | Windows UTF-8 conversion | — |
| `0x88054A00` | ERROR_NOT_FOUND | Windows UTF-8 conversion | — |
| `0x86054A00` | ERROR_SYSTEM_NOT_SUPPORTED | Windows UTF-8 conversion | — |
| `0x91054A00` | ERROR_ALLOCATION_FAILED | Windows UTF-8 conversion | — |
| `0x83054A00` | ERROR_MALFORMED_INPUT | Windows UTF-8 conversion | — |
| `0x89054A00` | ERROR_INTERRUPTED | Windows UTF-8 conversion | — |
| `0xA0054A00` | ERROR_IO_ERROR | Windows UTF-8 conversion | — |
| `0x84051900` | ERROR_ACCESS_DENIED | POSIX uname | — |
| `0x88051900` | ERROR_NOT_FOUND | POSIX uname | — |
| `0x86051900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX uname | — |
| `0x89051900` | ERROR_INTERRUPTED | POSIX uname | — |
| `0x91051900` | ERROR_ALLOCATION_FAILED | POSIX uname | — |
| `0xA0051900` | ERROR_IO_ERROR | POSIX uname | — |
| `0x83051B00` | ERROR_MALFORMED_INPUT | Darwin translation flag malformed | — |
| `0x84051C00` | ERROR_ACCESS_DENIED | Darwin translation query failed | — |
| `0x86051C00` | ERROR_SYSTEM_NOT_SUPPORTED | Darwin translation query failed | — |
| `0x89051C00` | ERROR_INTERRUPTED | Darwin translation query failed | — |
| `0x91051C00` | ERROR_ALLOCATION_FAILED | Darwin translation query failed | — |
| `0xA0051C00` | ERROR_IO_ERROR | Darwin translation query failed | — |
| `0x84055D00` | ERROR_ACCESS_DENIED | Windows native architecture retrieval | — |
| `0x88055D00` | ERROR_NOT_FOUND | Windows native architecture retrieval | — |
| `0x86055D00` | ERROR_SYSTEM_NOT_SUPPORTED | Windows native architecture retrieval | — |
| `0x91055D00` | ERROR_ALLOCATION_FAILED | Windows native architecture retrieval | — |
| `0x83055D00` | ERROR_MALFORMED_INPUT | Windows native architecture retrieval | — |
| `0x89055D00` | ERROR_INTERRUPTED | Windows native architecture retrieval | — |
| `0xA0055D00` | ERROR_IO_ERROR | Windows native architecture retrieval | — |

**C++ equivalent:** `catalyst::system_get_architecture`

### catsy_system_get_hostname

```c
CATALYST_RESULT catsy_system_get_hostname(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);
```

Retrieve the system hostname. Windows retrieves the DNS hostname; POSIX retrieves the UTS node name.

**Parameters:**  
`output` — Writable UTF-8 storage, or null for a size query.  
`capacity` — Writable code units including the terminator; zero for a size query.  
`required` — Writable required capacity, including the terminator.

**Context:** `0x06`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00060100` | SUCCESS | POSIX size query | — |
| `0x00060200` | SUCCESS | POSIX output copied | — |
| `0x90060300` | ERROR_BUFFER_OVERFLOW | POSIX destination too small; required returned | — |
| `0x88060600` | ERROR_NOT_FOUND | POSIX empty native value | — |
| `0x90060700` | ERROR_BUFFER_OVERFLOW | POSIX required extent unrepresentable | — |
| `0x00064100` | SUCCESS | Windows size query | — |
| `0x00064200` | SUCCESS | Windows output copied | — |
| `0x90064300` | ERROR_BUFFER_OVERFLOW | Windows destination too small; required returned | — |
| `0x88064600` | ERROR_NOT_FOUND | Windows empty native value | — |
| `0x90064700` | ERROR_BUFFER_OVERFLOW | Windows required extent unrepresentable | — |
| `0x84060800` | ERROR_ACCESS_DENIED | POSIX converter initialization | — |
| `0x88060800` | ERROR_NOT_FOUND | POSIX converter initialization | — |
| `0x86060800` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter initialization | — |
| `0x89060800` | ERROR_INTERRUPTED | POSIX converter initialization | — |
| `0x91060800` | ERROR_ALLOCATION_FAILED | POSIX converter initialization | — |
| `0xA0060800` | ERROR_IO_ERROR | POSIX converter initialization | — |
| `0x84060900` | ERROR_ACCESS_DENIED | POSIX converter cleanup | — |
| `0x88060900` | ERROR_NOT_FOUND | POSIX converter cleanup | — |
| `0x86060900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX converter cleanup | — |
| `0x89060900` | ERROR_INTERRUPTED | POSIX converter cleanup | — |
| `0x91060900` | ERROR_ALLOCATION_FAILED | POSIX converter cleanup | — |
| `0xA0060900` | ERROR_IO_ERROR | POSIX converter cleanup | — |
| `0x83060A00` | ERROR_MALFORMED_INPUT | POSIX conversion or validation | — |
| `0x84060A00` | ERROR_ACCESS_DENIED | POSIX conversion or validation | — |
| `0x88060A00` | ERROR_NOT_FOUND | POSIX conversion or validation | — |
| `0x86060A00` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX conversion or validation | — |
| `0x89060A00` | ERROR_INTERRUPTED | POSIX conversion or validation | — |
| `0x91060A00` | ERROR_ALLOCATION_FAILED | POSIX conversion or validation | — |
| `0xA0060A00` | ERROR_IO_ERROR | POSIX conversion or validation | — |
| `0x84064800` | ERROR_ACCESS_DENIED | Windows UTF-8 size query | — |
| `0x88064800` | ERROR_NOT_FOUND | Windows UTF-8 size query | — |
| `0x86064800` | ERROR_SYSTEM_NOT_SUPPORTED | Windows UTF-8 size query | — |
| `0x91064800` | ERROR_ALLOCATION_FAILED | Windows UTF-8 size query | — |
| `0x83064800` | ERROR_MALFORMED_INPUT | Windows UTF-8 size query | — |
| `0x89064800` | ERROR_INTERRUPTED | Windows UTF-8 size query | — |
| `0xA0064800` | ERROR_IO_ERROR | Windows UTF-8 size query | — |
| `0x84064A00` | ERROR_ACCESS_DENIED | Windows UTF-8 conversion | — |
| `0x88064A00` | ERROR_NOT_FOUND | Windows UTF-8 conversion | — |
| `0x86064A00` | ERROR_SYSTEM_NOT_SUPPORTED | Windows UTF-8 conversion | — |
| `0x91064A00` | ERROR_ALLOCATION_FAILED | Windows UTF-8 conversion | — |
| `0x83064A00` | ERROR_MALFORMED_INPUT | Windows UTF-8 conversion | — |
| `0x89064A00` | ERROR_INTERRUPTED | Windows UTF-8 conversion | — |
| `0xA0064A00` | ERROR_IO_ERROR | Windows UTF-8 conversion | — |
| `0x84061900` | ERROR_ACCESS_DENIED | POSIX uname | — |
| `0x88061900` | ERROR_NOT_FOUND | POSIX uname | — |
| `0x86061900` | ERROR_SYSTEM_NOT_SUPPORTED | POSIX uname | — |
| `0x89061900` | ERROR_INTERRUPTED | POSIX uname | — |
| `0x91061900` | ERROR_ALLOCATION_FAILED | POSIX uname | — |
| `0xA0061900` | ERROR_IO_ERROR | POSIX uname | — |
| `0x84065800` | ERROR_ACCESS_DENIED | Windows hostname size query | — |
| `0x88065800` | ERROR_NOT_FOUND | Windows hostname size query | — |
| `0x86065800` | ERROR_SYSTEM_NOT_SUPPORTED | Windows hostname size query | — |
| `0x91065800` | ERROR_ALLOCATION_FAILED | Windows hostname size query | — |
| `0x83065800` | ERROR_MALFORMED_INPUT | Windows hostname size query | — |
| `0xA0065800` | ERROR_IO_ERROR | Windows hostname size query | — |
| `0x88065900` | ERROR_NOT_FOUND | Windows hostname empty | — |
| `0x90065A00` | ERROR_BUFFER_OVERFLOW | Windows native extent unrepresentable | — |
| `0x91065B00` | ERROR_ALLOCATION_FAILED | Windows hostname allocation | — |
| `0x84065C00` | ERROR_ACCESS_DENIED | Windows hostname retrieval | — |
| `0x88065C00` | ERROR_NOT_FOUND | Windows hostname retrieval | — |
| `0x86065C00` | ERROR_SYSTEM_NOT_SUPPORTED | Windows hostname retrieval | — |
| `0x91065C00` | ERROR_ALLOCATION_FAILED | Windows hostname retrieval | — |
| `0x83065C00` | ERROR_MALFORMED_INPUT | Windows hostname retrieval | — |
| `0x89065C00` | ERROR_INTERRUPTED | Windows hostname retrieval | — |
| `0xA0065C00` | ERROR_IO_ERROR | Windows hostname retrieval | — |
| `0x82066100` | ERROR_INVALID_STATE | Windows hostname storage release | — |

**C++ equivalent:** `catalyst::system_get_hostname`

