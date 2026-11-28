#include "device_io_test.hpp"
#include "catinlib.hpp"

struct Fixture { const CATDVLIB_DEVICE* device; CATINLIB_INPUT* resource; NUINT channel; BYTE field; };
static PerfOutcome lifecycle(void* fixture, bool cpp) {
    Fixture* f = (Fixture*) fixture;
    CATINLIB_INPUT* resource = 0;
    RESULT r = cpp ? catalyst::input::device_open(f->device, &resource) : RESULT(catin_device_open(f->device, &resource));
    if (r.status != STATUS_CODE_SUCCESS) return PerfOutcome(r, 0);
    if (!resource) return PerfOutcome(RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x01, 0xFF, 0), 0);
    RESULT close = cpp ? catalyst::input::device_close(resource) : RESULT(catin_device_close(resource));
    if (close.status != STATUS_CODE_SUCCESS) return PerfOutcome(RESULT(close.status, 0x01, 0xFF, close.detail), 0);
    return PerfOutcome(r, 1);
}
static PerfOutcome query(void* fixture, bool cpp) {
    Fixture* f = (Fixture*) fixture;
    RESULT r;
    BYTE digest = 0;
    switch (f->field) {
        case 0x03: { BYTE bytes[4096]; NUINT received = 0; r = cpp ? catalyst::input::device_read(f->resource, bytes, (NUINT) sizeof(bytes), &received) : RESULT(catin_device_read(f->resource, bytes, (NUINT) sizeof(bytes), &received)); digest = (BYTE) received; if (received && received <= sizeof(bytes)) digest ^= bytes[0]; break; }
        case 0x08: { CATINLIB_DIGITAL_ACTION action = CATINLIB_DIGITAL_RELEASE; r = cpp ? catalyst::input::digital_read(f->resource, f->channel, &action) : RESULT(catin_digital_read(f->resource, f->channel, &action)); digest = action; break; }
        case 0x04: { NUINT count = 0; r = cpp ? catalyst::input::device_get_channel_count(f->resource, &count) : RESULT(catin_device_get_channel_count(f->resource, &count)); digest = (BYTE) count; break; }
        case 0x05: { CATINLIB_CHANNEL_TYPE type = CATINLIB_CHANNEL_UNKNOWN; r = cpp ? catalyst::input::channel_get_type(f->resource, f->channel, &type) : RESULT(catin_channel_get_type(f->resource, f->channel, &type)); digest = type; break; }
        case 0x06: { CATDVLIB_IDENTIFIER id = {0, 0}; r = cpp ? catalyst::input::channel_get_identifier(f->resource, f->channel, &id) : RESULT(catin_channel_get_identifier(f->resource, f->channel, &id)); if (valid_identifier(id)) digest = (BYTE) (id.scheme[0] ^ id.value[0]); break; }
        case 0x07: { BOOL state = FALSE; r = cpp ? catalyst::input::digital_get_state(f->resource, f->channel, &state) : RESULT(catin_digital_get_state(f->resource, f->channel, &state)); digest = state; break; }
#if CATALYST_SUPPORTS_32BIT
        case 0x09: { INT low = 0, high = 0; r = cpp ? catalyst::input::absolute_get_range(f->resource, f->channel, &low, &high) : RESULT(catin_absolute_get_range(f->resource, f->channel, &low, &high)); digest = (BYTE) (low ^ high); break; }
        case 0x0A: { INT value = 0; r = cpp ? catalyst::input::absolute_get_value(f->resource, f->channel, &value) : RESULT(catin_absolute_get_value(f->resource, f->channel, &value)); digest = (BYTE) value; break; }
        case 0x0B: { INT offset = 0; r = cpp ? catalyst::input::relative_read(f->resource, f->channel, &offset) : RESULT(catin_relative_read(f->resource, f->channel, &offset)); digest = (BYTE) offset; break; }
#endif
        default: r = RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, f->field, 0xFF, 0); break;
    }
    return PerfOutcome(r, digest);
}
int main() {
    std::printf("CATINLIB Performance (Release recommended)\nTarget: %s/%s/%s\n", TARGET_VENDOR, TARGET_SYSTEM, TARGET_ARCHITECTURE);
    std::printf("Median of three CPU-time samples; C API baseline vs equivalent C++ wrapper.\n");
    std::printf("Nonblocking polling measures CPU cost, not physical input throughput. Raw and typed polling consume events.\n");
    CATDVLIB_DEVICES* snapshot = 0;
    RESULT r = catdv_devices_enumerate(&snapshot);
    if (r.status != STATUS_CODE_SUCCESS || !snapshot) return 2;
    int failed = 0, unavailable = 0;
    NUINT opened = 0;
    for (NUINT j = 0; j < catdv_devices_get_count(snapshot); ++j) {
        const CATDVLIB_DEVICE* d = catdv_devices_get(snapshot, j);
        BOOL supported = FALSE;
        if (catdv_device_has_input(d, &supported).status != STATUS_CODE_SUCCESS || !supported) continue;
        Fixture f = {d, 0, 0, 0};
        print_device(j, d);
        perf_record(benchmark_io("open / close", lifecycle, &f, 0x01, 64), &failed, &unavailable);
        r = catin_device_open(d, &f.resource);
        if (r.status != STATUS_CODE_SUCCESS || !f.resource) { ++unavailable; continue; }
        ++opened;
        if (CATALYST_NUINT_MAX >= 4096) {
            f.field = 0x03;
            perf_record(benchmark_io("native read polling", query, &f, f.field, 32768, false), &failed, &unavailable);
        } else ++unavailable;
        f.field = 0x04;
        perf_record(benchmark_io("channel count", query, &f, f.field, 1048576UL), &failed, &unavailable);
        NUINT count = 0;
        if (catin_device_get_channel_count(f.resource, &count).status == STATUS_CODE_SUCCESS) {
            bool digital = false, absolute = false, relative = false;
            if (count) {
                f.channel = 0; f.field = 0x05;
                perf_record(benchmark_io("channel type", query, &f, f.field, 1048576UL), &failed, &unavailable);
                f.field = 0x06;
                perf_record(benchmark_io("channel identifier", query, &f, f.field, 1048576UL), &failed, &unavailable);
            }
            for (NUINT k = 0; k < count; ++k) {
                CATINLIB_CHANNEL_TYPE type = CATINLIB_CHANNEL_UNKNOWN;
                if (catin_channel_get_type(f.resource, k, &type).status != STATUS_CODE_SUCCESS) continue;
                f.channel = k;
                if (type == CATINLIB_CHANNEL_DIGITAL && !digital) {
                    digital = true; f.field = 0x07;
                    perf_record(benchmark_io("digital state", query, &f, f.field, 32768, false), &failed, &unavailable);
                    f.field = 0x08;
                    perf_record(benchmark_io("digital occurrence polling", query, &f, f.field, 32768, false), &failed, &unavailable);
                }
#if CATALYST_SUPPORTS_32BIT
                if (type == CATINLIB_CHANNEL_ABSOLUTE && !absolute) {
                    absolute = true; f.field = 0x09;
                    perf_record(benchmark_io("absolute range", query, &f, f.field, 32768), &failed, &unavailable);
                    f.field = 0x0A;
                    perf_record(benchmark_io("absolute value", query, &f, f.field, 32768, false), &failed, &unavailable);
                }
                if (type == CATINLIB_CHANNEL_RELATIVE && !relative) {
                    relative = true; f.field = 0x0B;
                    perf_record(benchmark_io("relative occurrence polling", query, &f, f.field, 32768, false), &failed, &unavailable);
                }
#endif
            }
        }
        r = catin_device_close(f.resource);
        if (r.status != STATUS_CODE_SUCCESS) ++failed;
        /* Benchmark one accessible device by default; avoid large or changing workloads. */
        break;
    }
    if (!opened) { std::printf("No accessible input device.\n"); ++unavailable; }
    if (catdv_devices_free(snapshot).status != STATUS_CODE_SUCCESS) ++failed;
    std::printf("\nFailures: %d; unavailable benchmarks: %d\n", failed, unavailable);
    return failed ? 1 : unavailable ? 2 : 0;
}
