<!-- AI AGENTS: DO NOT USE README.md as development context or modify it unless explicitly instructed by the user. This file is intended for human readers only. Read AGENTS.md for all AI development instructions, architectural requirements, and repository conventions. -->

<h1 align="center">CatalystUI – Aether</h1>
<p align="center"><strong>The universal translator between computers and people.</strong></p>
<p align="center">
  <em>The native system ABI for CatalystUI.</em>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-Apache--2.0-blue?style=flat-square&amp;logo=apache&amp;logoColor=white" alt="License: Apache-2.0"></a>
  <img src="https://img.shields.io/badge/Interface-C%20ABI-00599C?style=flat-square&amp;logo=c&amp;logoColor=white" alt="Interface: C ABI">
  <img src="https://img.shields.io/badge/Build-CMake%204.3%2B-064F8C?style=flat-square&amp;logo=cmake&amp;logoColor=white" alt="Build: CMake 4.3+">
  <a href="https://github.com/catalystui/lib-aether-c/commits/main"><img src="https://img.shields.io/github/last-commit/catalystui/lib-aether-c/main?style=flat-square&amp;logo=github&amp;logoColor=white" alt="Last commit"></a>
  <a href="https://github.com/catalystui/lib-aether-c/actions/workflows/compile.yml"><img src="https://img.shields.io/github/actions/workflow/status/catalystui/lib-aether-c/compile.yml?branch=main&amp;style=flat-square&amp;logo=github&amp;logoColor=white" alt="Compile status"></a>
</p>

## About Aether

Aether is the system library for CatalystUI, implementing **Layer 1: System** of the CatalystUI model through a native C ABI.

Aether aims to make native system-level bindings and operating system services accessible through consistent, portable contracts while preserving the capabilities of each native platform. Aether identifies and exposes native system facilities without inventing missing capabilities. It does not implement a general-purpose device communication framework. Windowing is the responsibility of Crystal.

## Development Status

> [!NOTE]
> Aether is in early development (1.0.0-alpha.1). Native system identification and device discovery are implemented for Windows, macOS, and Linux. Coverage varies by platform; API and ABI stability are not yet guaranteed.

## Libraries

| Library | Headers | Purpose |
| --- | --- | --- |
| [CATSYLIB](docs/lang/en-US/api-reference/catsylib/index.md) | `catsylib.h`, `catsylib.hpp` | Operating system, distribution, architecture, and hostname identification. |
| [CATDVLIB](docs/lang/en-US/api-reference/catdvlib/index.md) | `catdvlib.h`, `catdvlib.hpp` | Device discovery, hardware and instance identifiers, metadata, capability discovery, display identification, EDID acquisition, and native display lookup. |

Display discovery uses Win32 monitor interfaces, CoreGraphics display IDs, or Linux DRM connectors. EDID availability depends on the native system; Aether returns the bytes supplied by the OS without interpreting or synthesizing them. Numeric native display handles are supported on Windows and macOS; Linux connectors use exact instance identifiers.

See the [documentation](docs/lang/en-US/index.md) and [API reference](docs/lang/en-US/api-reference/index.md) for contracts and platform limitations.

## Getting Started

The repository uses **CMake 4.3 or later**, Git, and a C/C++ toolchain appropriate for the selected build preset.

Clone the repository along with its shared build infrastructure:

```powershell
git clone --recurse-submodules https://github.com/catalystui/lib-aether-c.git
Set-Location lib-aether-c
cmake --list-presets
```

If the repository has already been cloned, initialize its submodule with:

```powershell
git submodule update --init --recursive
```

The shared presets define target-specific compiler and platform configurations. For example, on an Apple Silicon Mac with the required Xcode toolchain:

```powershell
cmake --preset aclang17-darwin24.6-arm64
cmake --build --preset aclang17-darwin24.6-arm64-release
```

Aether uses Core's exported CMake targets, retrieving a pinned Core revision automatically when no parent-provided targets are available. An installed Core package can also be selected explicitly; see the documentation for configuration details.

Source consumers can link `Catalyst::catsy_static`, `Catalyst::catsy_shared`, `Catalyst::catdv_static`, or `Catalyst::catdv_shared`. Preset availability depends on the installed toolchain.

## Verification

Standalone verification and performance programs are enabled by default for top-level builds. From the configured Release output directory, run:

```powershell
./test_catsylib
./test_catdvlib
./perf_catsylib
./perf_catdvlib
```

System identification, HID discovery, display identification, and independent primary-display handle lookup have been exercised on macOS ARM64. The internal display's EDID was unavailable through the current public IOKit path. Successful EDID retrieval and Windows/Linux display hardware execution still require verification. Missing native properties are reported separately from failures.

## AI-Assisted Development

Aether is developed with AI assistance under human direction. AI coding agents assist with implementation and repository maintenance, while the project owner defines the model, requirements, and design decisions.

AI-assisted contributions are subject to the same expectations for correctness, maintainability, attribution, and licensing as human-written contributions. Their presence does not imply independent line-by-line human review.

## License

The original source code and documentation in this repository are licensed under the [Apache License 2.0](LICENSE).
