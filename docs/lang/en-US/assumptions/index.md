# Assumptions

**The caller is responsible for the validity of the invocation. The function is responsible for the outcome of the operation.**

These principles apply to public C functions, C++ wrappers, and internal helpers alike.

## Architecture

Core implements foundational representations and relations described by [FDEFSPEC](https://catalystui.org/specifications/languages/fdefspec/) and [FRELSPEC](https://catalystui.org/specifications/languages/frelspec/). Aether exposes native system capabilities that are not meaningfully supported by every Catalyst-compatible computing system. The classification depends on the contract, not whether its implementation invokes native APIs. Windowing belongs to Crystal.

## Numerics and Arithmetic

Aether shares Core's exact numeric representations, pointer-width sizes and counts, text encoding, and Result ABI. Portable arithmetic uses Catalyst representations; native types and conversions are confined to platform boundaries. Optional representations and native capabilities must be available where required; Aether does not emulate unavailable system facilities. Eight-bit compatibility is a foundational baseline, not a guarantee that every native facility is available on eight-bit systems.

## Validation

### Contract Validation

The caller establishes documented pointer validity, buffer extents, non-overlap, object lifetimes, synchronization and other preconditions. Functions do not redundantly validate these requirements.

### Operational Validation

Functions distinguish genuine operational failures, including absent information, unsupported facilities, invalid native data, insufficient capacity, permission failures and resource failures. They preserve documented failure state and release owned native resources. Native capabilities may depend on the runtime environment and permissions.

## Results

Operations that distinguish warnings or failures return the four ordered bytes of `CATALYST_RESULT` by value: status, context, operation and detail. Context identifies the reporting method, operation identifies a reporting site, and detail conveys status-specific information. Other functions return a single ordinary value directly, or use void for mutation or multiple outputs without reportable failure. Native errors are translated to existing FDEFSPEC statuses rather than truncated or exposed as incompatible result representations.

For individual contracts, see the [API Reference](../api-reference/index.md).
