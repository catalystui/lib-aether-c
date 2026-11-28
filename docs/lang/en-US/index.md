# Aether

The native system ABI for CatalystUI.

Aether implements Layer 1: System of the CatalystUI model. It exposes native system facilities through portable C contracts while preserving platform-specific capabilities and availability. It builds upon [FDEFSPEC](https://catalystui.org/specifications/languages/fdefspec/) and Core's foundational representations.

[View source on GitHub](https://github.com/catalystui/lib-aether-c) · [Understand the design](assumptions/index.md) · [API Reference](api-reference/index.md)

> [!NOTE]
> Aether is in early development (1.0.0-alpha.1). System identification is implemented for Windows, macOS and Linux; API and ABI stability are not yet guaranteed.

## Libraries

| Library | Description |
| --- | --- |
| [CATSYLIB](api-reference/catsylib/index.md) | Identify the operating system, distribution, architecture and hostname. |

## Building

Use CMake 4.3 or later and the shared target presets. Initialize the pinned submodules recursively.

```powershell
git clone --recurse-submodules https://github.com/catalystui/lib-aether-c.git
Set-Location lib-aether-c
cmake --list-presets
cmake --preset aclang17-darwin24.6-arm64
cmake --build --preset aclang17-darwin24.6-arm64-release
```

The example requires a suitable macOS toolchain. Other targets use their corresponding presets. Static and shared libraries and standalone verification programs are produced in the configured build directory.

## Verification

Run `test_catsylib` to verify available query contracts, and `perf_catsylib` in Release to compare size and output queries. The performance program compares the native backend with public C and C++ entry points; independent direct native references are shown where matching output is available. The independent references do not duplicate all validation, so their timings do not isolate reporting overhead. Both programs report unavailable queries separately from failures.
