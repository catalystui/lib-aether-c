<div style="text-align:center;padding:1.4rem 0.65rem 1.15rem;margin-bottom:1.2rem;border-bottom:1px solid color-mix(in srgb, currentColor 15%, transparent);">

# Aether

<p style="font-size:1.22em;font-weight:600;line-height:1.4;margin:0.75rem 0 0.35rem;">The universal translator between computers and people.</p>
<p style="font-style:italic;opacity:0.85;margin:0 0 1.2rem;">The native system ABI for CatalystUI.</p>

<div style="display:flex;flex-wrap:wrap;justify-content:center;align-items:center;gap:0.4rem;margin:0.4rem auto 1.2rem;">
<a href="https://github.com/catalystui/lib-aether-c/blob/main/LICENSE"><img style="display:block;width:auto;height:20px;max-width:100% !important;padding:0 !important;margin:0;" src="https://img.shields.io/badge/License-Apache--2.0-blue?style=flat-square&amp;logo=apache&amp;logoColor=white" alt="License: Apache-2.0"></a>
<img style="display:block;width:auto;height:20px;max-width:100% !important;padding:0 !important;margin:0;" src="https://img.shields.io/badge/Interface-C%20ABI-00599C?style=flat-square&amp;logo=c&amp;logoColor=white" alt="Interface: C ABI">
<img style="display:block;width:auto;height:20px;max-width:100% !important;padding:0 !important;margin:0;" src="https://img.shields.io/badge/Build-CMake%204.3%2B-064F8C?style=flat-square&amp;logo=cmake&amp;logoColor=white" alt="Build: CMake 4.3+">
<a href="https://github.com/catalystui/lib-aether-c/commits/main"><img style="display:block;width:auto;height:20px;max-width:100% !important;padding:0 !important;margin:0;" src="https://img.shields.io/github/last-commit/catalystui/lib-aether-c/main?style=flat-square&amp;logo=github&amp;logoColor=white" alt="Last commit"></a>
<a href="https://github.com/catalystui/lib-aether-c/actions/workflows/compile.yml"><img style="display:block;width:auto;height:20px;max-width:100% !important;padding:0 !important;margin:0;" src="https://img.shields.io/github/actions/workflow/status/catalystui/lib-aether-c/compile.yml?branch=main&amp;style=flat-square&amp;logo=github&amp;logoColor=white" alt="Compile status"></a>
</div>

<div style="display:flex;flex-wrap:wrap;justify-content:center;align-items:center;gap:0.5rem 1.35rem;font-weight:600;margin:0.5rem 0 0.2rem;">

[View source on GitHub](https://github.com/catalystui/lib-aether-c)

[Understand the design](assumptions/index.md)

[API Reference](api-reference/index.md)

</div>
</div>

Aether implements Layer 1: System of the CatalystUI model. It exposes native system facilities through portable C contracts built upon [FDEFSPEC](https://catalystui.org/specifications/languages/fdefspec/) and Core's foundational representations.

Aether exposes capabilities that are system-specific or not available on every Catalyst-compatible system. It preserves native availability and does not emulate unsupported facilities. Windowing belongs to Crystal.

> [!NOTE]
> Aether is in early development (1.0.0-alpha.1). Native system identification is implemented for Windows, macOS, and Linux. API and ABI stability are not yet guaranteed.

## Libraries

<div style="display:flex;flex-wrap:wrap;gap:0.85rem;margin:0.8rem 0 1rem;">
<div style="flex:1 1 calc(50% - 0.5rem);min-width:min(100%, 15rem);box-sizing:border-box;padding:1.2rem;border:1px solid color-mix(in srgb, currentColor 19%, transparent);border-radius:12px;background:color-mix(in srgb, currentColor 3%, transparent);display:flex;flex-direction:column;align-items:center;text-align:center;">
<svg aria-hidden="true" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" style="display:block;width:32px;height:32px;margin:0.15rem auto 0.8rem;opacity:0.9;"><rect x="3" y="4" width="18" height="13" rx="2"/><path d="M8 21h8m-4-4v4M7 9h10M7 13h6"/></svg>

**[CATSYLIB](api-reference/catsylib/index.md)**

<p style="margin:0 0 0.85rem;max-width:20rem;">Identify the operating system, distribution, architecture, and hostname.</p>
<small style="opacity:0.8;margin-top:auto;">Link: <code>catsy</code></small>
</div>
</div>

## Getting Started

You'll need Git, **CMake 4.3 or later**, and a C/C++ toolchain for your target.

Clone Aether and list the available build presets:

```powershell
git clone --recurse-submodules https://github.com/catalystui/lib-aether-c.git
Set-Location lib-aether-c
cmake --list-presets
```

For example, on an Apple Silicon Mac with the required Xcode toolchain:

```powershell
cmake --preset aclang17-darwin24.6-arm64
cmake --build --preset aclang17-darwin24.6-arm64-release
```

Already cloned? Run `git submodule update --init --recursive` to initialize the shared build infrastructure and pinned Core headers. Presets depend on the installed toolchain and do not guarantee runtime support.

The [compile workflow](https://github.com/catalystui/lib-aether-c/actions/workflows/compile.yml) provides the repository's configured build checks.

## Verification

Run `test_catsylib` to verify available query contracts, and `perf_catsylib` in Release to compare size and output queries. The performance program compares the native backend with public C and C++ entry points; independent direct native references are shown where matching output is available. The independent references do not duplicate all validation, so their timings do not isolate reporting overhead. Both programs report unavailable queries separately from failures.

## AI-Assisted Development

Aether is developed with AI assistance under human direction. The owner defines the model, requirements, and design decisions. All contributions follow the same correctness, maintainability, attribution, and licensing expectations; AI-assisted code is not necessarily reviewed independently line by line.

## License

Aether is licensed under the [Apache License 2.0](https://github.com/catalystui/lib-aether-c/blob/main/LICENSE).
