#if !defined(_POSIX_C_SOURCE) && !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cstring>

#include "athidlib.hpp"
#include "../src/athidlib.internal.hpp"

#if TARGET_PLATFORM_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wchar.h>
#undef TRUE
#undef FALSE
#elif TARGET_PLATFORM_DARWIN || TARGET_PLATFORM_LINUX
#include <sys/utsname.h>
#if TARGET_PLATFORM_LINUX
#include <errno.h>
#elif TARGET_PLATFORM_DARWIN
#include <sys/sysctl.h>
#undef TRUE
#undef FALSE
#endif
#endif


typedef CATALYST_RESULT (*Query)(catalyst::UTF8W, catalyst::NUINT, catalyst::NUINT*);
typedef catalyst::RESULT (*QueryCpp)(catalyst::UTF8W, catalyst::NUINT, catalyst::NUINT*);

struct Benchmark {
    const char* name;
    ATHIDLIB_FIELD field;
    Query query;
    QueryCpp cpp;
};

static const Benchmark benchmarks[] = {
    { "athid_system_get_name", ATHIDLIB_FIELD_NAME, athid_system_get_name, catalyst::system_get_name },
    { "athid_system_get_version", ATHIDLIB_FIELD_VERSION, athid_system_get_version, catalyst::system_get_version },
    { "athid_system_get_distribution", ATHIDLIB_FIELD_DISTRIBUTION, athid_system_get_distribution, catalyst::system_get_distribution },
    { "athid_system_get_distribution_version", ATHIDLIB_FIELD_DISTRIBUTION_VERSION, athid_system_get_distribution_version, catalyst::system_get_distribution_version },
    { "athid_system_get_architecture", ATHIDLIB_FIELD_ARCHITECTURE, athid_system_get_architecture, catalyst::system_get_architecture },
    { "athid_system_get_hostname", ATHIDLIB_FIELD_HOSTNAME, athid_system_get_hostname, catalyst::system_get_hostname }
};

#if defined(CLOCKS_PER_SEC)
// Independent native C reference. Fixed scratch extents keep the reference
// simple; a value that does not fit or cannot be matched is not benchmarked.
static bool native_copy(const char* value, catalyst::UTF8W output, catalyst::NUINT capacity, catalyst::NUINT* required) {
    size_t length = std::strlen(value);
    if (length == 0 || length >= CATALYST_NUINT_MAX) return false;
    *required = (catalyst::NUINT) (length + 1);
    if (output == 0) return capacity == 0;
    if (capacity < *required) return false;
    std::memcpy(output, value, (size_t) *required);
    return true;
}

#if TARGET_PLATFORM_LINUX
// Accept common unescaped os-release assignments. More complicated valid
// assignments are handled by ATHIDLIB but not by this benchmark reference.
static bool native_distribution(ATHIDLIB_FIELD field, catalyst::UTF8W output, catalyst::NUINT capacity, catalyst::NUINT* required) {
    FILE* file = std::fopen("/etc/os-release", "r");
    if (file == 0 && errno == ENOENT) file = std::fopen("/usr/lib/os-release", "r");
    if (file == 0) return false;
    const char* key = field == ATHIDLIB_FIELD_DISTRIBUTION ? "NAME=" : "VERSION_ID=";
    size_t keyLength = std::strlen(key);
    char line[512];
    char value[512];
    value[0] = 0;
    bool valid = true;
    while (std::fgets(line, sizeof(line), file) != 0) {
        size_t length = std::strlen(line);
        if (length == sizeof(line) - 1 && line[length - 1] != '\n') {
            valid = false;
            break;
        }
        if (std::strncmp(line, key, keyLength) != 0) continue;
        char* start = line + keyLength;
        size_t count = std::strcspn(start, "\r\n");
        start[count] = 0;
        if (count != 0 && (start[0] == '"' || start[0] == '\'')) {
            if (count < 2 || start[count - 1] != start[0]) { valid = false; break; }
            start[count - 1] = 0;
            ++start;
        }
        if (std::strpbrk(start, "\\$") != 0) { valid = false; break; }
        std::strcpy(value, start);
    }
    if (std::ferror(file)) valid = false;
    if (std::fclose(file) != 0) valid = false;
    return valid && native_copy(value, output, capacity, required);
}
#endif

#if TARGET_PLATFORM_WIN32
static bool native_wide(const WCHAR* value, catalyst::UTF8W output, catalyst::NUINT capacity, catalyst::NUINT* required) {
    if (*value == 0) return false;
    int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, 0, 0, 0, 0);
    if (bytes <= 0 || (size_t) bytes > CATALYST_NUINT_MAX) return false;
    *required = (catalyst::NUINT) bytes;
    if (output == 0) return capacity == 0;
    if (capacity < *required) return false;
    return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, (char*) output, bytes, 0, 0) == bytes;
}

static bool native_version(OSVERSIONINFOEXW* version) {
    typedef LONG (WINAPI *VersionFunction)(OSVERSIONINFOW*);
    HMODULE module = GetModuleHandleW(L"ntdll.dll");
    if (module == 0) return false;
    VersionFunction function = reinterpret_cast<VersionFunction>(GetProcAddress(module, "RtlGetVersion"));
    if (function == 0) return false;
    ZeroMemory(version, sizeof(*version));
    version->dwOSVersionInfoSize = sizeof(*version);
    return function((OSVERSIONINFOW*) version) == 0;
}

static bool native_registry(ATHIDLIB_FIELD field, catalyst::UTF8W output, catalyst::NUINT capacity, catalyst::NUINT* required) {
    HKEY key = 0;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) return false;
    WCHAR value[256];
    DWORD bytes = sizeof(value) - sizeof(WCHAR);
    DWORD type = 0;
    LONG error = RegQueryValueExW(key, field == ATHIDLIB_FIELD_DISTRIBUTION ? L"ProductName" :
        L"DisplayVersion", 0, &type, (BYTE*) value, &bytes);
    if (error == ERROR_FILE_NOT_FOUND && field == ATHIDLIB_FIELD_DISTRIBUTION_VERSION) {
        bytes = sizeof(value) - sizeof(WCHAR);
        error = RegQueryValueExW(key, L"ReleaseId", 0, &type, (BYTE*) value, &bytes);
    }
    LONG closeError = RegCloseKey(key);
    if (error != ERROR_SUCCESS || closeError != ERROR_SUCCESS || type != REG_SZ ||
        bytes % sizeof(WCHAR) != 0 || bytes >= sizeof(value)) return false;
    value[bytes / sizeof(WCHAR)] = 0;
    if (field == ATHIDLIB_FIELD_DISTRIBUTION) {
        OSVERSIONINFOEXW version;
        if (wcsncmp(value, L"Windows 10", 10) == 0 && native_version(&version) &&
            version.wProductType == VER_NT_WORKSTATION && version.dwMajorVersion == 10 &&
            version.dwBuildNumber >= 22000) value[9] = L'1';
    }
    return native_wide(value, output, capacity, required);
}

static const char* native_architecture() {
    // Native system info is a direct reference. On emulation environments
    // where it differs from IsWow64Process2, preflight omits this reference.
    SYSTEM_INFO info;
    GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
        case PROCESSOR_ARCHITECTURE_AMD64: return "x86_64";
        case PROCESSOR_ARCHITECTURE_ARM: return "arm";
        case 12: return "arm64";
        case PROCESSOR_ARCHITECTURE_IA64: return "ia64";
        default: return 0;
    }
}
#endif

static bool native_query(ATHIDLIB_FIELD field, catalyst::UTF8W output, catalyst::NUINT capacity, catalyst::NUINT* required) {
#if TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN
#if TARGET_PLATFORM_LINUX
    if (field == ATHIDLIB_FIELD_DISTRIBUTION || field == ATHIDLIB_FIELD_DISTRIBUTION_VERSION)
        return native_distribution(field, output, capacity, required);
#elif TARGET_PLATFORM_DARWIN
    if (field == ATHIDLIB_FIELD_DISTRIBUTION)
        return native_copy("macOS", output, capacity, required);
    if (field == ATHIDLIB_FIELD_DISTRIBUTION_VERSION) {
        char value[128];
        size_t length = sizeof(value);
        if (sysctlbyname("kern.osproductversion", value, &length, 0, 0) != 0 ||
            length == 0 || length > sizeof(value) || value[length - 1] != 0) return false;
        return native_copy(value, output, capacity, required);
    }
    if (field == ATHIDLIB_FIELD_ARCHITECTURE) {
        int translated = 0;
        size_t length = sizeof(translated);
        if (sysctlbyname("sysctl.proc_translated", &translated, &length, 0, 0) == 0 && translated == 1)
            return native_copy("arm64", output, capacity, required);
    }
#endif
    struct utsname info;
    if (uname(&info) != 0) return false;
    const char* value = 0;
    switch (field) {
        case ATHIDLIB_FIELD_NAME: value = info.sysname; break;
        case ATHIDLIB_FIELD_VERSION: value = info.release; break;
        case ATHIDLIB_FIELD_HOSTNAME: value = info.nodename; break;
        case ATHIDLIB_FIELD_ARCHITECTURE:
            value = info.machine;
            if (std::strcmp(value, "aarch64") == 0) value = "arm64";
            else if (std::strcmp(value, "amd64") == 0) value = "x86_64";
            else if (std::strcmp(value, "i386") == 0 || std::strcmp(value, "i486") == 0 ||
                std::strcmp(value, "i586") == 0 || std::strcmp(value, "i686") == 0) value = "x86";
            break;
        default: return false;
    }
    return native_copy(value, output, capacity, required);
#elif TARGET_PLATFORM_WIN32
    switch (field) {
        case ATHIDLIB_FIELD_NAME: return native_copy("Windows NT", output, capacity, required);
        case ATHIDLIB_FIELD_VERSION: {
            OSVERSIONINFOEXW version;
            if (!native_version(&version)) return false;
            char value[33];
            std::sprintf(value, "%lu.%lu.%lu", (unsigned long) version.dwMajorVersion,
                (unsigned long) version.dwMinorVersion, (unsigned long) version.dwBuildNumber);
            return native_copy(value, output, capacity, required);
        }
        case ATHIDLIB_FIELD_DISTRIBUTION:
        case ATHIDLIB_FIELD_DISTRIBUTION_VERSION:
            return native_registry(field, output, capacity, required);
        case ATHIDLIB_FIELD_ARCHITECTURE: {
            const char* value = native_architecture();
            return value != 0 && native_copy(value, output, capacity, required);
        }
        case ATHIDLIB_FIELD_HOSTNAME: {
            WCHAR value[256];
            DWORD length = 256;
            if (!GetComputerNameExW(ComputerNameDnsHostname, value, &length)) return false;
            return native_wide(value, output, capacity, required);
        }
        default: return false;
    }
#else
    (void) field; (void) output; (void) capacity; (void) required;
    return false;
#endif
}

static volatile CATALYST_BYTE sink = 0;

static bool invoke(const Benchmark& benchmark, unsigned int mode, catalyst::UTF8W output, catalyst::NUINT capacity,
    catalyst::NUINT* required, bool sizeOnly) {
    *required = 0;
    if (mode == 3) return native_query(benchmark.field, output, capacity, required);
    CATALYST_RESULT result;
    if (mode == 0) result = athid_platform_get(benchmark.field, output, capacity, required);
    else if (mode == 1) result = benchmark.query(output, capacity, required);
    else result = benchmark.cpp(output, capacity, required);
#if TARGET_PLATFORM_WIN32
    const CATALYST_BYTE operation = sizeOnly ? 0x41 : 0x42;
#else
    const CATALYST_BYTE operation = sizeOnly ? 0x01 : 0x02;
#endif
    sink = (CATALYST_BYTE) (sink ^ result.status ^ result.context ^ result.operation ^ result.detail);
    return result.status == CATALYST_STATUS_CODE_SUCCESS && result.context == (CATALYST_BYTE) benchmark.field &&
        result.operation == operation && result.detail == 0;
}

static bool measure(const Benchmark& benchmark, unsigned int mode, catalyst::UTF8W output, catalyst::NUINT capacity,
    catalyst::NUINT expected, unsigned long calls, bool sizeOnly, double* seconds) {
    std::clock_t start = std::clock();
    if (start == (std::clock_t) -1) return false;
    for (unsigned long i = 0; i < calls; ++i) {
        catalyst::NUINT required;
        if (!invoke(benchmark, mode, output, capacity, &required, sizeOnly) || required != expected) return false;
        if (!sizeOnly) sink = (CATALYST_BYTE) (sink ^ output[0] ^ output[required - (catalyst::NUINT) 1]);
        else sink = (CATALYST_BYTE) (sink ^ (CATALYST_BYTE) required);
    }
    std::clock_t end = std::clock();
    if (end == (std::clock_t) -1 || end < start) return false;
    *seconds = (double) (end - start) / (double) CLOCKS_PER_SEC;
    return true;
}

static double median(double* values) {
    if (values[0] > values[1]) { double t = values[0]; values[0] = values[1]; values[1] = t; }
    if (values[1] > values[2]) { double t = values[1]; values[1] = values[2]; values[2] = t; }
    return values[0] > values[1] ? values[0] : values[1];
}

static int benchmark_case(const Benchmark& benchmark, bool sizeOnly, catalyst::UTF8W output, catalyst::UTF8W reference, catalyst::NUINT expected) {
    catalyst::UTF8W destination = sizeOnly ? 0 : output;
    catalyst::NUINT capacity = sizeOnly ? 0 : expected;
    unsigned int modes = 3;
    catalyst::NUINT required = 0;
    if (native_query(benchmark.field, output, expected, &required) &&
        required == expected && std::memcmp(output, reference, (size_t) expected) == 0) modes = 4;
    for (unsigned int mode = 0; mode < modes; ++mode) {
        if (!invoke(benchmark, mode, destination, capacity, &required, sizeOnly) || required != expected ||
            (!sizeOnly && std::memcmp(output, reference, (size_t) expected) != 0)) {
            std::printf("  FAIL: preflight output/status mismatch (%u)\n", mode);
            return 1;
        }
    }
    unsigned long calls = 64;
    double elapsed = 0;
    do {
        if (!measure(benchmark, 0, destination, capacity, expected, calls, sizeOnly, &elapsed)) {
            std::printf("  FAIL: baseline timing/verification\n");
            return 1;
        }
        if (elapsed >= 0.025 || calls >= 16777216UL) break;
        calls *= 2;
    } while (true);
    unsigned long counts[4] = { calls, calls, calls, calls };
    // A constant-valued native reference can be much faster than the backend.
    // Calibrate its own batch size while retaining paired backend/C/C++ batches.
    if (modes == 4) {
        for (;;) {
            if (!measure(benchmark, 3, destination, capacity, expected, counts[3], sizeOnly, &elapsed)) {
                std::printf("  FAIL: direct reference timing/verification\n");
                return 1;
            }
            if (elapsed >= 0.025 || counts[3] >= 16777216UL) break;
            counts[3] *= 2;
        }
    }
    double samples[4][3];
    for (unsigned int sample = 0; sample < 3; ++sample) {
        for (unsigned int step = 0; step < modes; ++step) {
            unsigned int mode = sample % 2 == 0 ? step : modes - 1 - step;
            if (!measure(benchmark, mode, destination, capacity, expected, counts[mode], sizeOnly, &samples[mode][sample]) ||
                (!sizeOnly && std::memcmp(output, reference, (size_t) expected) != 0)) {
                std::printf("  FAIL: timed output/status mismatch (%u)\n", mode);
                return 1;
            }
        }
    }
    double base = median(samples[0]);
    const char* labels[] = { "Native backend", "Catalyst C", "Catalyst C++", "Direct native C API" };
    std::printf("  %s (%lu calls/sample for backend and public APIs)\n", sizeOnly ? "Size query" : "Output query", calls);
    for (unsigned int mode = 0; mode < modes; ++mode) {
        double elapsedTime = median(samples[mode]);
        if (elapsedTime < 0.001 || elapsedTime * CLOCKS_PER_SEC < 20) {
            std::printf("    %-21s short sample; timing unavailable (%lu calls)\n", labels[mode], counts[mode]);
            continue;
        }
        std::printf("    %-21s %12.1f ns/call  %12.0f calls/s", labels[mode],
            elapsedTime * 1e9 / (double) counts[mode], (double) counts[mode] / elapsedTime);
        if (mode != 0 && mode != 3 && base >= 0.001 && base * CLOCKS_PER_SEC >= 20)
            std::printf("  %+8.2f%% vs backend", (elapsedTime / base - 1.0) * 100.0);
        if (mode == 3) std::printf("  (%lu calls/sample)", counts[mode]);
        std::printf("\n");
    }
    double direct = modes == 4 ? median(samples[3]) : 0;
    double comparison = median(samples[1]);
    if (direct >= 0.001 && direct * CLOCKS_PER_SEC >= 20 && comparison >= 0.001 &&
        comparison * CLOCKS_PER_SEC >= 20)
        std::printf("    C vs direct native: %+8.2f%% (includes validation/normalization differences)\n",
            ((comparison / (double) counts[1]) / (direct / (double) counts[3]) - 1.0) * 100.0);
    else if (modes == 3) std::printf("    Direct native reference unavailable or not equivalent on this target.\n");
    return 0;
}
#endif

int main() {
#if defined(CLOCKS_PER_SEC)
    if (CLOCKS_PER_SEC <= 0 || std::clock() == (std::clock_t) -1) {
        std::printf("ATHIDLIB performance unavailable: no usable standard C clock.\n");
        return 2;
    }
    std::printf("ATHIDLIB Performance (Release recommended)\n");
    std::printf("Target: %s/%s/%s\n", TARGET_VENDOR, TARGET_SYSTEM, TARGET_ARCHITECTURE);
    std::printf("Median of three interleaved CPU-time samples; %lu ticks/second.\n", (unsigned long) CLOCKS_PER_SEC);
    std::printf("Backend baseline isolates public dispatch/wrapper overhead, not native acquisition overhead.\n");
    std::printf("Independent direct C API reference covers all six fields where its bounded fixtures match; it omits full POSIX UTF-8 validation.\n");
    int failed = 0;
    int unavailable = 0;
    for (CATALYST_BYTE i = 0; i < sizeof(benchmarks) / sizeof(benchmarks[0]); ++i) {
        const Benchmark& benchmark = benchmarks[i];
        std::printf("\n%s\n", benchmark.name);
        catalyst::NUINT required = 0;
        if (!invoke(benchmark, 1, 0, 0, &required, true) || required == 0) {
            std::printf("  Query unavailable or failed; run test_athidlib for status details.\n");
            ++unavailable;
            continue;
        }
        catalyst::UTF8W output = (catalyst::UTF8W) std::malloc((size_t) required);
        catalyst::UTF8W reference = (catalyst::UTF8W) std::malloc((size_t) required);
        catalyst::NUINT actual = 0;
        if (output == 0 || reference == 0 || !invoke(benchmark, 1, reference, required, &actual, false) || actual != required) {
            std::printf("  FAIL: fixture initialization\n");
            ++failed;
        } else {
            failed += benchmark_case(benchmark, true, output, reference, required);
            failed += benchmark_case(benchmark, false, output, reference, required);
        }
        std::free(output);
        std::free(reference);
    }
    std::printf("\nFailures: %d; unavailable queries: %d\n", failed, unavailable);
    return failed != 0 ? 1 : unavailable != 0 ? 2 : 0;
#else
    std::printf("ATHIDLIB performance unavailable: standard C clock not provided.\n");
    return 2;
#endif
}
