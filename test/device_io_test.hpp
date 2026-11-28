#ifndef DEVICE_IO_TEST_HPP
#define DEVICE_IO_TEST_HPP
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "catdvlib.hpp"

using namespace catalyst;

struct TestReport {
    unsigned long passed, failed, skipped;
    TestReport() : passed(0), failed(0), skipped(0) {}

    void check(bool condition, const char* name) {
        if (condition) { ++passed; std::printf("PASS: %s\n", name); }
        else { ++failed; std::printf("FAIL: %s\n", name); }
    }
    void skip(const char* name) { ++skipped; std::printf("SKIP: %s\n", name); }
    bool result(const char* name, RESULT r, BYTE context, bool allowNoop = false) {
        if (r.context != context || r.operation == 0) {
            check(false, name);
        } else if (r.status == STATUS_CODE_SUCCESS || (allowNoop && r.status == STATUS_CODE_SUCCESS_NOOP)) {
            check(r.detail == 0, name);
            return r.detail == 0;
        } else if (r.status == STATUS_CODE_ERROR_NOT_FOUND || r.status == STATUS_CODE_ERROR_ACCESS_DENIED ||
                   r.status == STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED || r.status == STATUS_CODE_ERROR_NOT_IMPLEMENTED) {
            skip(name);
        } else {
            check(false, name);
        }
        std::printf("      Result: %02X %02X %02X %02X\n", (unsigned) r.status,
            (unsigned) r.context, (unsigned) r.operation, (unsigned) r.detail);
        return false;
    }
    int finish() const {
        std::printf("\nPassed: %lu; failures: %lu; unavailable: %lu\n", passed, failed, skipped);
        return failed ? 1 : skipped ? 2 : 0;
    }
};

static inline bool same_result(RESULT a, RESULT b) {
    return a.status == b.status && a.context == b.context &&
        a.operation == b.operation && a.detail == b.detail;
}

static inline bool valid_identifier(const CATDVLIB_IDENTIFIER& id) {
    return id.scheme && *id.scheme && id.value && *id.value;
}
static inline bool same_identifier(const CATDVLIB_IDENTIFIER& a, const CATDVLIB_IDENTIFIER& b) {
    return valid_identifier(a) && valid_identifier(b) &&
        std::strcmp((const char*) a.scheme, (const char*) b.scheme) == 0 &&
        std::strcmp((const char*) a.value, (const char*) b.value) == 0;
}

static inline void print_device(NUINT index, const CATDVLIB_DEVICE* d) {
    CATDVLIB_IDENTIFIER id = {0, 0};
    RESULT r = catdv_device_get_identifier(d, &id);
    std::printf("\nDevice %lu", (unsigned long) index);
    if (r.status == STATUS_CODE_SUCCESS && valid_identifier(id))
        std::printf(" [%s] %s", (const char*) id.scheme, (const char*) id.value);
    std::printf("\n");
}

/* Device indices are snapshot-local. No implicit write target. */
static inline bool parse_index(const char* text, NUINT* index) {
    if (!*text) return false;
    NUINT n = 0;
    while (*text) {
        if (*text < '0' || *text > '9') return false;
        BYTE digit = (BYTE) (*text++ - '0');
        if (n > (CATALYST_NUINT_MAX - digit) / 10) return false;
        n = (NUINT) (n * 10 + digit);
    }
    *index = n;
    return true;
}

static inline CATDVLIB_DEVICES* test_snapshot(TestReport* report) {
    CATDVLIB_DEVICES* snapshot = 0;
    RESULT r = catdv_devices_enumerate(&snapshot);
    bool ok = report->result("enumerate devices", r, 0x01);
    report->check((r.status == STATUS_CODE_SUCCESS) == (snapshot != 0), "snapshot ownership");
    return ok ? snapshot : 0;
}

/* Timing follows the existing CATSYLIB CPU-time convention. Count no-ops
 * separately: polling rate is not physical device throughput. */
struct PerfOutcome {
    RESULT result;
    BYTE digest;
    PerfOutcome(RESULT r, BYTE d) : result(r), digest(d) {}
};
typedef PerfOutcome (*PerfCall)(void*, bool);
struct PerfSample {
    double seconds;
    unsigned long complete, noop;
};
static volatile BYTE device_io_sink = 0;

static inline bool measure_io(PerfCall call, void* fixture, bool cpp, unsigned long calls,
                              BYTE context, PerfSample* sample) {
    sample->complete = sample->noop = 0;
    std::clock_t begin = std::clock();
    if (begin == (std::clock_t) -1) return false;
    for (unsigned long j = 0; j < calls; ++j) {
        PerfOutcome value = call(fixture, cpp);
        RESULT r = value.result;
        device_io_sink = (BYTE) (device_io_sink ^ r.status ^ r.context ^ r.operation ^ r.detail ^ value.digest);
        if (r.context != context || r.operation == 0 || r.detail != 0) return false;
        if (r.status == STATUS_CODE_SUCCESS) ++sample->complete;
        else if (r.status == STATUS_CODE_SUCCESS_NOOP) ++sample->noop;
        else {
            std::printf("    Result: %02X %02X %02X %02X\n", (unsigned) r.status,
                (unsigned) r.context, (unsigned) r.operation, (unsigned) r.detail);
            return false;
        }
    }
    std::clock_t end = std::clock();
    if (end == (std::clock_t) -1 || end < begin) return false;
    sample->seconds = (double) (end - begin) / CLOCKS_PER_SEC;
    return true;
}

static inline PerfSample median_io(PerfSample* samples) {
    if (samples[0].seconds > samples[1].seconds) { PerfSample t = samples[0]; samples[0] = samples[1]; samples[1] = t; }
    if (samples[1].seconds > samples[2].seconds) { PerfSample t = samples[1]; samples[1] = samples[2]; samples[2] = t; }
    return samples[0].seconds > samples[1].seconds ? samples[0] : samples[1];
}

static inline int benchmark_io(const char* name, PerfCall call, void* fixture, BYTE context,
                              unsigned long limit, bool stable = true, double* cNanoseconds = 0) {
    if (cNanoseconds) *cNanoseconds = 0;
    std::printf("\n%s\n", name);
    PerfOutcome a = call(fixture, false), b = call(fixture, true);
    if (a.result.context != context || b.result.context != context ||
        !a.result.operation || !b.result.operation) {
        std::printf("  FAIL: result context/operation\n"); return 1;
    }
    if ((a.result.status != STATUS_CODE_SUCCESS && a.result.status != STATUS_CODE_SUCCESS_NOOP) ||
        (b.result.status != STATUS_CODE_SUCCESS && b.result.status != STATUS_CODE_SUCCESS_NOOP)) {
        std::printf("  Unavailable or failed: C %02X, C++ %02X\n",
            (unsigned) a.result.status, (unsigned) b.result.status);
        STATUS_CODE statuses[] = {a.result.status, b.result.status};
        for (BYTE j = 0; j < 2; ++j) {
            STATUS_CODE s = statuses[j];
            if (s != STATUS_CODE_SUCCESS && s != STATUS_CODE_SUCCESS_NOOP &&
                s != STATUS_CODE_ERROR_NOT_FOUND && s != STATUS_CODE_ERROR_ACCESS_DENIED &&
                s != STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED && s != STATUS_CODE_ERROR_NOT_IMPLEMENTED)
                return 1;
        }
        return 2;
    }
    if (stable && (!same_result(a.result, b.result) || a.digest != b.digest)) {
        std::printf("  FAIL: C/C++ preflight mismatch\n"); return 1;
    }
    unsigned long calls = 1;
    PerfSample calibration;
    for (;;) {
        if (!measure_io(call, fixture, false, calls, context, &calibration)) return 1;
        if (calibration.seconds >= 0.025 || calls >= limit) break;
        calls = calls > limit / 2 ? limit : calls * 2;
    }
    PerfSample samples[2][3];
    for (BYTE j = 0; j < 3; ++j)
        for (BYTE k = 0; k < 2; ++k) {
            BYTE mode = (j & 1) ? (BYTE) (1 - k) : k;
            if (!measure_io(call, fixture, mode != 0, calls, context, &samples[mode][j])) return 1;
        }
    PerfSample c = median_io(samples[0]), cpp = median_io(samples[1]);
    if (cNanoseconds && c.seconds >= 0.001 && c.seconds * CLOCKS_PER_SEC >= 20)
        *cNanoseconds = c.seconds * 1e9 / calls;
    PerfSample values[] = {c, cpp};
    const char* labels[] = {"Catalyst C", "Catalyst C++"};
    for (BYTE j = 0; j < 2; ++j) {
        PerfSample s = values[j];
        std::printf("  %-13s %lu calls, %lu completed, %lu no-op", labels[j], calls, s.complete, s.noop);
        if (s.seconds >= 0.001 && s.seconds * CLOCKS_PER_SEC >= 20) {
            std::printf("; %.1f ns/call; %.0f calls/s", s.seconds * 1e9 / calls, calls / s.seconds);
            if (j && c.seconds >= 0.001 && c.seconds * CLOCKS_PER_SEC >= 20)
                std::printf("; %+.2f%% vs C", (s.seconds / c.seconds - 1) * 100);
        } else std::printf("; sample too short for reliable timing");
        std::printf("\n");
    }
    if (c.seconds < 0.001 || cpp.seconds < 0.001 ||
        c.seconds * CLOCKS_PER_SEC < 20 || cpp.seconds * CLOCKS_PER_SEC < 20) return 2;
    return 0;
}
static inline void perf_record(int result, int* failed, int* unavailable) {
    if (result == 1) ++*failed;
    else if (result == 2) ++*unavailable;
}
#endif
