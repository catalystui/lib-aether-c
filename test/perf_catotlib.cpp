#include "device_io_test.hpp"
#include "catotlib.hpp"

struct Fixture { const CATDVLIB_DEVICE* device; CATOTLIB_OUTPUT* resource; NUINT channel; BYTE field; };
static PerfOutcome lifecycle(void* fixture, bool cpp) {
    Fixture* f = (Fixture*) fixture;
    CATOTLIB_OUTPUT* resource = 0;
    RESULT r = cpp ? catalyst::output::device_open(f->device, &resource) : RESULT(catot_device_open(f->device, &resource));
    if (r.status != STATUS_CODE_SUCCESS) return PerfOutcome(r, 0);
    if (!resource) return PerfOutcome(RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x01, 0xFF, 0), 0);
    RESULT close = cpp ? catalyst::output::device_close(resource) : RESULT(catot_device_close(resource));
    if (close.status != STATUS_CODE_SUCCESS) return PerfOutcome(RESULT(close.status, 0x01, 0xFF, close.detail), 0);
    return PerfOutcome(r, 1);
}
static PerfOutcome query(void* fixture, bool cpp) {
    Fixture* f = (Fixture*) fixture;
    RESULT r;
    BYTE digest = 0;
    switch (f->field) {

        case 0x04: { NUINT count = 0; r = cpp ? catalyst::output::device_get_channel_count(f->resource, &count) : RESULT(catot_device_get_channel_count(f->resource, &count)); digest = (BYTE) count; break; }
        case 0x05: { CATOTLIB_CHANNEL_TYPE type = CATOTLIB_CHANNEL_UNKNOWN; r = cpp ? catalyst::output::channel_get_type(f->resource, f->channel, &type) : RESULT(catot_channel_get_type(f->resource, f->channel, &type)); digest = type; break; }
        case 0x06: { CATDVLIB_IDENTIFIER id = {0, 0}; r = cpp ? catalyst::output::channel_get_identifier(f->resource, f->channel, &id) : RESULT(catot_channel_get_identifier(f->resource, f->channel, &id)); if (valid_identifier(id)) digest = (BYTE) (id.scheme[0] ^ id.value[0]); break; }
        case 0x07: { BOOL state = FALSE; r = cpp ? catalyst::output::digital_get_state(f->resource, f->channel, &state) : RESULT(catot_digital_get_state(f->resource, f->channel, &state)); digest = state; break; }
#if CATALYST_SUPPORTS_32BIT
        case 0x09: { INT low = 0, high = 0; r = cpp ? catalyst::output::absolute_get_range(f->resource, f->channel, &low, &high) : RESULT(catot_absolute_get_range(f->resource, f->channel, &low, &high)); digest = (BYTE) (low ^ high); break; }
        case 0x0A: { INT value = 0; r = cpp ? catalyst::output::absolute_get_value(f->resource, f->channel, &value) : RESULT(catot_absolute_get_value(f->resource, f->channel, &value)); digest = (BYTE) value; break; }

#endif
        default: r = RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, f->field, 0xFF, 0); break;
    }
    return PerfOutcome(r, digest);
}
int main() {
    std::printf("CATOTLIB Performance (Release recommended)\nTarget: %s/%s/%s\n", TARGET_VENDOR, TARGET_SYSTEM, TARGET_ARCHITECTURE);
    std::printf("Median of three CPU-time samples; C API baseline vs equivalent C++ wrapper.\n");
    std::printf("Read-only benchmarking; native output submission and setter timing deliberately omitted to avoid repeatedly commanding hardware.\n");
    CATDVLIB_DEVICES* snapshot = 0;
    RESULT r = catdv_devices_enumerate(&snapshot);
    if (r.status != STATUS_CODE_SUCCESS || !snapshot) return 2;
    int failed = 0, unavailable = 0;
    NUINT opened = 0;
    for (NUINT j = 0; j < catdv_devices_get_count(snapshot); ++j) {
        const CATDVLIB_DEVICE* d = catdv_devices_get(snapshot, j);
        BOOL supported = FALSE;
        if (catdv_device_has_output(d, &supported).status != STATUS_CODE_SUCCESS || !supported) continue;
        Fixture f = {d, 0, 0, 0};
        print_device(j, d);
        perf_record(benchmark_io("open / close", lifecycle, &f, 0x01, 64), &failed, &unavailable);
        r = catot_device_open(d, &f.resource);
        if (r.status != STATUS_CODE_SUCCESS || !f.resource) { ++unavailable; continue; }
        ++opened;

        f.field = 0x04;
        perf_record(benchmark_io("channel count", query, &f, f.field, 1048576UL), &failed, &unavailable);
        NUINT count = 0;
        if (catot_device_get_channel_count(f.resource, &count).status == STATUS_CODE_SUCCESS) {
            bool digital = false, absolute = false;
            if (count) {
                f.channel = 0; f.field = 0x05;
                perf_record(benchmark_io("channel type", query, &f, f.field, 1048576UL), &failed, &unavailable);
                f.field = 0x06;
                perf_record(benchmark_io("channel identifier", query, &f, f.field, 1048576UL), &failed, &unavailable);
            }
            for (NUINT k = 0; k < count; ++k) {
                CATOTLIB_CHANNEL_TYPE type = CATOTLIB_CHANNEL_UNKNOWN;
                if (catot_channel_get_type(f.resource, k, &type).status != STATUS_CODE_SUCCESS) continue;
                f.channel = k;
                if (type == CATOTLIB_CHANNEL_DIGITAL && !digital) {
                    digital = true; f.field = 0x07;
                    perf_record(benchmark_io("digital state", query, &f, f.field, 32768, false), &failed, &unavailable);

                }
#if CATALYST_SUPPORTS_32BIT
                if (type == CATOTLIB_CHANNEL_ABSOLUTE && !absolute) {
                    absolute = true; f.field = 0x09;
                    perf_record(benchmark_io("absolute range", query, &f, f.field, 32768), &failed, &unavailable);
                    f.field = 0x0A;
                    perf_record(benchmark_io("absolute value", query, &f, f.field, 32768, false), &failed, &unavailable);
                }

#endif
            }
        }
        r = catot_device_close(f.resource);
        if (r.status != STATUS_CODE_SUCCESS) ++failed;
        /* Benchmark one accessible device by default; avoid large or changing workloads. */
        break;
    }
    if (!opened) { std::printf("No accessible output device.\n"); ++unavailable; }
    if (catdv_devices_free(snapshot).status != STATUS_CODE_SUCCESS) ++failed;
    std::printf("\nFailures: %d; unavailable benchmarks: %d\n", failed, unavailable);
    return failed ? 1 : unavailable ? 2 : 0;
}
