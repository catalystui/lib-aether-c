> **PRIMARY PROJECT CONTEXT** — Read this file before work. Keep it concise, current, and actionable. Do not use README.md as development context or edit it without explicit user authorization.

# Aether Project Contract

`lib-aether-c` (Aether) is the native system ABI for CatalystUI, implementing Layer 1: System. Provide portable contracts for native system-level bindings and operating system functionality. Follow applicable CatalystUI specifications; do not invent new semantics or architectural requirements.

## Authority and architecture

- **ALL-systems rule:** Core owns representations and operations meaningfully supported by **all** Catalyst-supported computing systems, even when their implementations differ by platform. Aether owns only native system capabilities that are system-specific or unavailable on some supported systems. This distinction applies across the entire supported computing ecosystem, not merely desktop operating systems.
- Classify functionality by its contract, not its implementation. Fundamental memory operations belong in Core; system-specific memory facilities may belong in Aether. Do not move universal functionality into Aether merely because it uses native system APIs. Core may implement universal contracts using native mechanisms directly.
- Do not invent emulations or pretend that a system-specific capability exists on every target. Keep platform-dependent availability and requirements explicit. Windowing belongs to Crystal (`lib-crystal-c`), not Aether.
- Consult FDEFSPEC, FRELSPEC, applicable layer specifications, and `lib-core-c` contracts before defining APIs. Reuse foundational types, status codes, memory, and text APIs; do not duplicate or bypass them. Report missing concepts or conflicts rather than inventing replacements.
- The owner defines architecture and contracts. Explain the necessity and consequences of changes to approved foundational representations, APIs, helpers, or behavior and obtain explicit approval first. Never modify or bypass Numeric Definitions, Codepoint, Text Encoding, or Operation Status to simplify Aether. Do not add dependencies without authorization.
- Inspect relevant source, instructions, and Git state first. Preserve unrelated changes. Prefer simple, direct, explicit implementations; no speculative abstractions, duplicated systems or algorithms, unnecessary dependencies, layers, cleanup, or unrequested scaffolding.
- Before stability, legacy API/ABI compatibility is not required; no shims or parallel APIs merely to preserve obsolete behavior. Update affected code, callers, wrappers, existing tests, and references together within the authorized scope.

## Numerics and arithmetic

Core's 8-bit arithmetic guarantee is the foundational portability baseline, not a performance target or a requirement to emulate unsupported native capabilities in Aether.

- Catalyst Numeric Definitions guarantee exact representations. Native types implement Catalyst types; language compliance alone does not establish Catalyst compatibility.
- Use Catalyst-defined numeric types for portable arithmetic, including intermediate values. Do not bypass unavailable representations with native substitutes or rely on wider implicit promotions.
- `CATALYST_SUPPORTS_*` indicates representational availability, not processor width or arithmetic performance. Guard access to and arithmetic on optional types with `#if CATALYST_SUPPORTS_*`, not `#ifdef`. Guard implementations only by actual representation and platform requirements.
- Preserve byte-compatible arithmetic wherever the contract permits; use compile-time-selected wider Catalyst arithmetic when it provides substantial benefits. Equivalent paths must preserve mathematical behavior and observable guarantees. Native capabilities may require wider representations or particular system facilities; document and guard those requirements instead of inventing emulations.
- Reuse approved Core arithmetic and conversion primitives; do not scatter equivalent implementations across Aether. Obtain owner approval before introducing new foundational helpers.
- Preserve defined byte order and representation. Do not assume native endianness, alignment, padding, integer layout, or host/target equivalence. Codepoint remains MSB-first (`byte0`–`byte3`); UTF-16LE code units remain `lsb`, then `msb`.
- Preserve the pointer-width scalar ABI of `CATALYST_NINT`/`CATALYST_NUINT` for addresses, sizes, counts, and indices, including one-byte pointers where applicable. Do not assume a wider minimum or use these types to bypass numeric support guarantees.
- Account for promotions, overflow, truncation, signedness, and shift behavior. Confine required native types, arithmetic, and conversions to platform boundaries; validate representability when translating to Catalyst contracts. Do not impose native floating-point or optional wider instructions on unrelated portable paths.

## Language, ABI, and style

- Portable public headers target C90; portable implementations and wrappers target C++98. Preserve target-preset language standards and use newer language features only through existing guards or where a native platform requires them. Keep platform requirements isolated and explicit.
- Maintain public C ABI contracts in `include/<library>.h`, implementations in `src/<library>.cpp` and platform-specific source files, and thin inline C++ wrappers in `include/<library>.hpp` when provided. Wrappers forward without duplicating logic. Keep C++ definitions in .hpp files; arrays crossing the C ABI use actual C element types.
- Preserve include guards, `extern "C"`, exports, const/read-write distinctions, ownership, lifetimes, alignment, and buffer contracts. Keep native platform details private where practical; public interfaces expose portable Catalyst contracts unless the approved capability specifically requires native interoperability.
- Use Catalyst types and `BOOL`/`TRUE`/`FALSE` (C: `CATALYST_BOOL`/`CATALYST_TRUE`/`CATALYST_FALSE`). No native numeric substitutes, native booleans, exceptions, references, STL containers, heap abstractions, or external dependencies in portable code without approval. Confine OS APIs to platform-specific files (e.g. `*.win32.cpp`, `*.linux.cpp`, `*.darwin.mm`).
- C types/constants: uppercase snake case with the appropriate library prefix; functions: lowercase snake case, library prefix + subject + operation. Reserve `CATALYST_` and `cat_` for approved foundational definitions and functions. C++ equivalents omit C prefixes within the appropriate Catalyst namespace. Parameters/fields/private functions use lowercase snake case; descriptive lower-camel-case locals are acceptable.
- Four spaces, same-line braces, null pointer `0`, C-compatible typedefs, explicit casts. Prefer early returns, simple loops, static private helpers, section dividers, concise comments explaining intent rather than obvious code. Follow .editorconfig/.gitattributes where present (UTF-8, CRLF working tree, final newline, no trailing whitespace).

## Validation and function contracts

- **Caller:** Establish documented preconditions (valid initialized objects/handles, pointers, extents, termination, direct-access indices, alignment, lifetime, non-overlap, synchronization). Aether assumes these hold; violations are outside its guarantees. Document public and private preconditions; do not redundantly recheck them or provide parallel checked variants.
- **Function:** Validate operational conditions even with valid inputs: malformed data, unrepresentable values, insufficient capacity, overflow, unsupported system capabilities, permission failures, timeouts, interruption, allocation/I/O/dependency/cleanup failures, failed retrieval. Validate during the operation, before unsafe access or irreversible changes; never exceed accessible extents. Do not disguise native operational failures as caller preconditions.
- Establish valid internal state on creation and preserve it through mutation. Trust established guarantees; do not repeatedly inspect metadata to detect misuse or arbitrary corruption. Preserve promised failure state, release owned resources, and report genuine operational failures. Document native handle ownership, borrowing, lifetime, and cleanup behavior.
- Determine the operation's approved contract before selecting the first applicable return convention:
  1. Warning/error/fatal outcome must be distinguished: return `CATALYST_RESULT` (`RESULT` in C++) by value; additional data via output arguments.
  2. Single ordinary value: return its Catalyst type directly.
  3. Otherwise: return `void`; mutate storage or write output arguments.
- Ordinary zero, false, equality, absence, emptiness, carry/borrow, and no-op are not inherently failures. Multiple outputs alone do not require a Result. Do not invent sentinel failures or success-only Results. Reporting operations retain their documented no-op contracts.
- Report at the outermost boundary owning operational validation and failure handling; use non-reporting primitives once preconditions are established. Internal steps still report when their caller must distinguish a warning or failure. Never suppress required diagnostics for performance.
- Apply these rules equally to exports, wrappers, and private helpers. Document output validity for every outcome. Obtain approval before changing intended behavior or approved validation/reporting boundaries.
- Benchmark equivalent work including output storage and status consumption when evaluating performance; report absolute time, frequency, and relative overhead. Instruction counts are not timings. Do not assume Core's numeric performance targets apply to native system calls.

## Operation results

`RESULT` represents four ordered bytes: status, context, operation, detail. Guarded `UINT` packing, conversion, and operations in `0xSSCCOODD` order are valid; preserve the four-field C ABI and by-value return. No exceptions, result output pointers, last-result state, or larger combined return structures.

- **Status:** Use the most specific FDEFSPEC code. Undefined codes are reserved. Translate native outcomes to their applicable Catalyst status; use `ERROR_SYSTEM_NOT_SUPPORTED` when an operation reports that the target does not support the requested capability, not for features merely unimplemented by Aether.
- **Context:** Unique per module; fixed method ID on every return, including success and no-op. Zero means unspecified, not success.
- **Operation:** Unique per method and result-reporting site, including success. Distinct sites never share an opcode; one site may have multiple details. No need to retain obsolete IDs before stability.
- **Detail:** Meaning precedence: status > context > operation. Use useful additional information without duplication; zero when unused. `ERROR_INVALID_ARGUMENT` uses the zero-based index in the full signature (including by-value arguments). Do not truncate native error codes or reinterpret defined detail semantics to store them.
- Dependency failures: preserve applicable status, fatal severity, and useful detail, retaining the reporting method's context and call-site opcode. Do not blindly copy fields or default to `ERROR_DEPENDENCY_FAILURE`.
- Define assignments before implementation; use `return RESULT(...);` with defined status constants/severity helpers on every reporting exit. Non-reporting functions do not construct, propagate, or document Results.

## Documentation

- Specifications define concepts; explanatory docs teach them; references describe actual contracts. Keep source, wrappers, existing tests, and docs consistent; report discrepancies instead of silently reconciling them. Use concise exact terminology, no redundant prose, task histories, or agent memory.
- Keep README.md human-facing; maintain persistent development instructions here. As documentation grows, follow Core's Overview, Assumptions, and API Reference (last) structure and `docs/manifest.json` navigation with `"alignment": "block"` per page. Assumptions explains shared architecture, arithmetic, validation, reporting, and native availability. New guides require approval; no duplicate contracts or module-level Conventions sections. Do not create unrequested documentation scaffolding.
- Public declarations: short `/** ... */` behavior/precondition comments. API reference: one comprehensive `docs/lang/en-US/api-reference/<module>/index.md` per module, linked from the reference index. Include headers, namespace, **Library:** actual linker name or Header-only, **Dependencies:** if applicable, concise Usage Notes for shared module contracts, and linked type/constant/function summary and details.
- Type details: description before declaration/representation, fields, C++ equivalent where provided. Show concrete, typed **C90:**/**C99+:** examples when different; explain target-dependent native alternatives, guards, and unavailable variants. Use Core's availability badge convention: `<span data-heading-badge="MACRO" data-heading-badge-background="#b4232c" data-heading-badge-color="#ffffff" title="Requires MACRO to be 1"></span>` after heading/description; no separate REQUIRES paragraph.
- Function details: exact C name heading, fenced C signature, description, **Parameters:** individual entries (not a table), **Returns:** for ordinary values or **Results:** for Results, targeted **See Also:** when useful. Use Markdown hard line breaks for consecutive metadata/parameter/field entries; separate See Also. Document units, capacities, nullability, preconditions, ownership, failure outputs, edge cases, and platform availability.
- Reporting methods: **Context:** `0xCC` immediately before **Results:** table with exactly `Result | Status | Operation | Detail`. One row per distinct result, including each fixed detail. First column `0xSSCCOODD` (uppercase, logical byte order regardless of endianness); other columns describe meaning, not redundant bytes. Document genuine variable ranges/patterns; em dash only for unused fields, not meaningful zero. Link specifications to the live website.

## Build and verification

- Preserve the pinned shared build-cmake submodule, cmakecfg.h, existing presets, target conventions, and static/shared targets where provided. Target capabilities belong in build-cmake and cmakecfg.h; no parallel configuration headers. Initialize the cmake submodule recursively; no build/CI/submodule workarounds without approval.
- Keep authored configuration separate from generated files and build artifacts; do not commit generated output. Runtime executables, tests, DLLs, versioned shared libraries and ABI aliases belong directly in debug/ or release/. Development archives/import libraries/unversioned linker aliases belong in flat lib/. Unix runtime dependencies resolve relative to binaries; linker aliases point to runtime ABI aliases. Shared build-cmake derives names/versions from metadata; preserve actual library names and targets.
- Relevant builds and existing verification programs are allowed. Do not create automated tests, test harnesses, or custom CI workflows unless explicitly requested. The programmer owns manual verification.
- Where applicable, verify portable C90/C++98 and C99/C++11 headers/builds, optional-width fallbacks, and relevant native targets. Review arithmetic and representation assumptions independently. Cross-compilation does not establish hardware execution.
- When verifying functionality, cover normal/boundary/no-op/failure behavior, wrappers, all four Result bytes, resource lifetimes, and supported/unavailable platform paths without violating caller preconditions. Verify equivalent arithmetic paths and claimed optimization benefits where applicable.
- Report actual checks, limitations, conflicts, and unverified behavior; suggest at most 2–3 focused manual checks when applicable.

## Commits

Focused authorized changes, concise imperative subjects. Preserve Apache-2.0 licensing, required attribution, and existing Git author/committer identity where supported. Every created or amended commit must include:

`Co-authored-by: Codex <noreply@openai.com>`
