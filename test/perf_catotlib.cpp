#include "device_io_test.hpp"
#include "catotlib.hpp"
#if TARGET_PLATFORM_LINUX
#include <linux/input.h>
#endif

struct Fixture {
    const CATDVLIB_DEVICE* device;
    CATOTLIB_OUTPUT* resource;
    NUINT channel;
    BYTE field;
    BOOL state;
#if CATALYST_SUPPORTS_32BIT
    INT value;
#endif
#if TARGET_PLATFORM_LINUX
    struct input_event native;
#endif
};
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
        case 0x08: r = cpp ? catalyst::output::digital_set_state(f->resource, f->channel, f->state) : RESULT(catot_digital_set_state(f->resource, f->channel, f->state)); digest = f->state; break;
#if TARGET_PLATFORM_LINUX
        case 0x03: {
            NUINT written = 0;
            r = cpp ? catalyst::output::device_write(f->resource, &f->native, sizeof(f->native), &written) :
                RESULT(catot_device_write(f->resource, &f->native, sizeof(f->native), &written));
            digest = (BYTE) written;
            break;
        }
#endif
        case 0x07: { BOOL state = FALSE; r = cpp ? catalyst::output::digital_get_state(f->resource, f->channel, &state) : RESULT(catot_digital_get_state(f->resource, f->channel, &state)); digest = state; break; }
#if CATALYST_SUPPORTS_32BIT
        case 0x0B: r = cpp ? catalyst::output::absolute_set_value(f->resource, f->channel, f->value) : RESULT(catot_absolute_set_value(f->resource, f->channel, f->value)); digest = (BYTE) f->value; break;
        case 0x09: { INT low = 0, high = 0; r = cpp ? catalyst::output::absolute_get_range(f->resource, f->channel, &low, &high) : RESULT(catot_absolute_get_range(f->resource, f->channel, &low, &high)); digest = (BYTE) (low ^ high); break; }
        case 0x0A: { INT value = 0; r = cpp ? catalyst::output::absolute_get_value(f->resource, f->channel, &value) : RESULT(catot_absolute_get_value(f->resource, f->channel, &value)); digest = (BYTE) value; break; }

#endif
        default: r = RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, f->field, 0xFF, 0); break;
    }
    return PerfOutcome(r, digest);
}
/* A pending same-value write is the only request that may be retried.
 * Do not switch to another output channel until completion is observed. */
static bool drain_write(Fixture* f) {
    for (unsigned long j = 0; j < 100000; ++j) {
        PerfOutcome result = query(f, false);
        if (result.result.status == STATUS_CODE_SUCCESS) return true;
        if (result.result.status != STATUS_CODE_SUCCESS_NOOP) return false;
    }
    return false;
}

int main(int argc, char** argv) {
    NUINT selected = 0;
    bool write = argc == 3 && std::strcmp(argv[1], "--write") == 0 && parse_index(argv[2], &selected);
    if (argc != 1 && !write) { std::printf("Usage: perf_catotlib [--write DEVICE_INDEX]\n"); return 1; }
    std::printf("CATOTLIB Performance (Release recommended)\nTarget: %s/%s/%s\n", TARGET_VENDOR, TARGET_SYSTEM, TARGET_ARCHITECTURE);
    std::printf("Median of three CPU-time samples; C API baseline vs equivalent C++ wrapper.\n");
    std::printf("Read-only by default. --write DEVICE_INDEX benchmarks same-value writes on the selected snapshot entry; no guessed reports.\n");
    CATDVLIB_DEVICES* snapshot = 0;
    RESULT r = catdv_devices_enumerate(&snapshot);
    if (r.status != STATUS_CODE_SUCCESS || !snapshot) return 2;
    int failed = 0, unavailable = 0;
    NUINT opened = 0;
    if (write && selected >= catdv_devices_get_count(snapshot)) {
        std::printf("Selected device index does not exist.\n");
        catdv_devices_free(snapshot);
        return 1;
    }
    for (NUINT j = 0; j < catdv_devices_get_count(snapshot); ++j) {
        if (write && j != selected) continue;
        const CATDVLIB_DEVICE* d = catdv_devices_get(snapshot, j);
        BOOL supported = FALSE;
        if (catdv_device_has_output(d, &supported).status != STATUS_CODE_SUCCESS || !supported) continue;
        Fixture f;
        std::memset(&f, 0, sizeof(f));
        f.device = d;
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
                    if (write && catot_digital_get_state(f.resource, k, &f.state).status == STATUS_CODE_SUCCESS) {
                        f.field = 0x08;
                        int measured = benchmark_io("reapply digital state", query, &f, f.field, 64, false);
                        perf_record(measured, &failed, &unavailable);
                        if (measured != 0 || !drain_write(&f)) { ++unavailable; break; }
#if TARGET_PLATFORM_LINUX
                        CATDVLIB_IDENTIFIER id = {0, 0};
                        unsigned eventType, eventCode;
                        if (catot_channel_get_identifier(f.resource, k, &id).status == STATUS_CODE_SUCCESS &&
                            valid_identifier(id) && std::strcmp((const char*) id.scheme, "linux-evdev") == 0 &&
                            std::sscanf((const char*) id.value, "%u:%u", &eventType, &eventCode) == 2 &&
                            eventType == EV_LED && eventCode < LED_CNT) {
                            std::memset(&f.native, 0, sizeof(f.native));
                            f.native.type = EV_LED; f.native.code = (unsigned short) eventCode;
                            f.native.value = f.state ? 1 : 0;
                            f.field = 0x03;
                            measured = benchmark_io("native LED record submission", query, &f, f.field, 64, false);
                            perf_record(measured, &failed, &unavailable);
                            if (measured != 0 || !drain_write(&f)) { ++unavailable; break; }
                        }
#endif
                    }

                }
#if CATALYST_SUPPORTS_32BIT
                if (type == CATOTLIB_CHANNEL_ABSOLUTE && !absolute) {
                    absolute = true; f.field = 0x09;
                    perf_record(benchmark_io("absolute range", query, &f, f.field, 32768), &failed, &unavailable);
                    f.field = 0x0A;
                    perf_record(benchmark_io("absolute value", query, &f, f.field, 32768, false), &failed, &unavailable);
                    if (write && catot_absolute_get_value(f.resource, k, &f.value).status == STATUS_CODE_SUCCESS) {
                        f.field = 0x0B;
                        int measured = benchmark_io("reapply absolute value", query, &f, f.field, 64, false);
                        perf_record(measured, &failed, &unavailable);
                        if (measured != 0 || !drain_write(&f)) { ++unavailable; break; }
                    }
                }

#endif
            }
        }
        r = catot_device_close(f.resource);
#if TARGET_PLATFORM_DARWIN
        for (unsigned long attempt = 0; r.status == STATUS_CODE_ERROR_INVALID_STATE && attempt < 100000; ++attempt)
            r = catot_device_close(f.resource);
#endif
        if (r.status != STATUS_CODE_SUCCESS) ++failed;
        /* Benchmark one accessible device by default; avoid large or changing workloads. */
        break;
    }
    if (!opened) { std::printf("No accessible output device.\n"); ++unavailable; }
    if (!write) std::printf("Output write timing not requested.\n");
#if !TARGET_PLATFORM_LINUX
    std::printf("Raw output record timing unavailable without a device-specific known report.\n");
#endif
    if (catdv_devices_free(snapshot).status != STATUS_CODE_SUCCESS) ++failed;
    std::printf("\nFailures: %d; unavailable benchmarks: %d\n", failed, unavailable);
    return failed ? 1 : unavailable ? 2 : 0;
}
