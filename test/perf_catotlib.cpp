#include "device_io_native.hpp"
#include "catotlib.hpp"
#if TARGET_PLATFORM_LINUX
#include <linux/input.h>
#endif

struct Fixture {
    const CATDVLIB_DEVICE* device;
    CATOTLIB_OUTPUT* resource;
    catalyst::NUINT channel;
    catalyst::BYTE field;
    catalyst::BOOL state;
#if CATALYST_SUPPORTS_32BIT
    catalyst::INT value;
#endif
#if TARGET_PLATFORM_LINUX
    struct input_event native;
#endif
};
static PerfOutcome lifecycle(void* fixture, bool cpp) {
    Fixture* f = (Fixture*) fixture;
    CATOTLIB_OUTPUT* resource = 0;
    catalyst::RESULT r = cpp ? catalyst::output::device_open(f->device, &resource) : catalyst::RESULT(catot_device_open(f->device, &resource));
    if (r.status != catalyst::STATUS_CODE_SUCCESS) return PerfOutcome(r, 0);
    if (!resource) return PerfOutcome(catalyst::RESULT(catalyst::STATUS_CODE_ERROR_INVALID_STATE, 0x01, 0xFF, 0), 0);
    catalyst::RESULT close = cpp ? catalyst::output::device_close(resource) : catalyst::RESULT(catot_device_close(resource));
    if (close.status != catalyst::STATUS_CODE_SUCCESS) return PerfOutcome(catalyst::RESULT(close.status, 0x01, 0xFF, close.detail), 0);
    return PerfOutcome(r, 1);
}
static PerfOutcome query(void* fixture, bool cpp) {
    Fixture* f = (Fixture*) fixture;
    catalyst::RESULT r;
    catalyst::BYTE digest = 0;
    switch (f->field) {

        case 0x04: { catalyst::NUINT count = 0; r = cpp ? catalyst::output::device_get_channel_count(f->resource, &count) : catalyst::RESULT(catot_device_get_channel_count(f->resource, &count)); digest = (catalyst::BYTE) count; break; }
        case 0x05: { CATOTLIB_CHANNEL_TYPE type = CATOTLIB_CHANNEL_UNKNOWN; r = cpp ? catalyst::output::channel_get_type(f->resource, f->channel, &type) : catalyst::RESULT(catot_channel_get_type(f->resource, f->channel, &type)); digest = type; break; }
        case 0x06: { CATDVLIB_IDENTIFIER id = {0, 0}; r = cpp ? catalyst::output::channel_get_identifier(f->resource, f->channel, &id) : catalyst::RESULT(catot_channel_get_identifier(f->resource, f->channel, &id)); if (valid_identifier(id)) digest = (catalyst::BYTE) (id.scheme[0] ^ id.value[0]); break; }
        case 0x08: r = cpp ? catalyst::output::digital_set_state(f->resource, f->channel, f->state) : catalyst::RESULT(catot_digital_set_state(f->resource, f->channel, f->state)); digest = f->state; break;
#if TARGET_PLATFORM_LINUX
        case 0x03: {
            catalyst::NUINT written = 0;
            r = cpp ? catalyst::output::device_write(f->resource, &f->native, sizeof(f->native), &written) :
                catalyst::RESULT(catot_device_write(f->resource, &f->native, sizeof(f->native), &written));
            digest = (catalyst::BYTE) written;
            break;
        }
#endif
        case 0x07: { catalyst::BOOL state = catalyst::FALSE; r = cpp ? catalyst::output::digital_get_state(f->resource, f->channel, &state) : catalyst::RESULT(catot_digital_get_state(f->resource, f->channel, &state)); digest = state; break; }
#if CATALYST_SUPPORTS_32BIT
        case 0x0B: r = cpp ? catalyst::output::absolute_set_value(f->resource, f->channel, f->value) : catalyst::RESULT(catot_absolute_set_value(f->resource, f->channel, f->value)); digest = (catalyst::BYTE) f->value; break;
        case 0x09: { catalyst::INT low = 0, high = 0; r = cpp ? catalyst::output::absolute_get_range(f->resource, f->channel, &low, &high) : catalyst::RESULT(catot_absolute_get_range(f->resource, f->channel, &low, &high)); digest = (catalyst::BYTE) (low ^ high); break; }
        case 0x0A: { catalyst::INT value = 0; r = cpp ? catalyst::output::absolute_get_value(f->resource, f->channel, &value) : catalyst::RESULT(catot_absolute_get_value(f->resource, f->channel, &value)); digest = (catalyst::BYTE) value; break; }

#endif
        default: r = catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, f->field, 0xFF, 0); break;
    }
    return PerfOutcome(r, digest);
}
/* One sample call submits and observes completion of one request.
 * Never transfer an unfinished operation between C and C++ samples. */
static PerfOutcome completed_write(void* fixture, bool cpp) {
    Fixture* f = (Fixture*) fixture;
    std::time_t start = std::time(0);
    for (;;) {
        PerfOutcome outcome = query(fixture, cpp);
        if (outcome.result.status != catalyst::STATUS_CODE_SUCCESS_NOOP) return outcome;
        if (outcome.result.context != f->field || !outcome.result.operation || outcome.result.detail)
            return PerfOutcome(catalyst::RESULT(catalyst::STATUS_CODE_ERROR_INVALID_STATE, f->field, 0xFF, 0), 0);
        std::time_t now = std::time(0);
        if (start == (std::time_t) -1 || now == (std::time_t) -1 || now < start || std::difftime(now, start) >= 5.0)
            return PerfOutcome(catalyst::RESULT(catalyst::STATUS_CODE_ERROR_TIMEOUT, f->field, 0xFF, 0), 0);
    }
}

int main(int argc, char** argv) {
    bool write = (argc == 2 || argc == 4) && std::strcmp(argv[1], "--write") == 0;
    if (argc != 1 && !write) { std::printf("Usage: perf_catotlib [--write [SCHEME INSTANCE_ID]]\n"); return 1; }
    std::printf("CATOTLIB Performance (Release recommended)\nTarget: %s/%s/%s\n", TARGET_VENDOR, TARGET_SYSTEM, TARGET_ARCHITECTURE);
    std::printf("Median of three CPU-time samples; C API vs C++ wrapper; independent direct native state/range reference when the same device/channel/value is available.\n");
    std::printf("Read-only by default. --write selects a device for same-value write benchmarks; no guessed reports.\n");
    if (write) std::printf("Write timing includes completion polling; CPU time is not wall-clock device latency.\n");
    CATDVLIB_DEVICES* snapshot = 0;
    catalyst::RESULT r = catdv_devices_enumerate(&snapshot);
    if (r.status != catalyst::STATUS_CODE_SUCCESS || !snapshot) return 2;
    int failed = 0, unavailable = 0;
    catalyst::NUINT opened = 0;
    bool lifecycleDone = false, countDone = false, metadataDone = false;
    bool digital = false;
    unsigned completedWriteBenchmarks = 0;
#if CATALYST_SUPPORTS_32BIT
    bool absolute = false;
#endif

    const CATDVLIB_DEVICE* selected = 0;
    if (write) {
        selected = select_device(snapshot, false, argc == 4 ? argv[2] : 0, argc == 4 ? argv[3] : 0);
        if (!selected) { catdv_devices_free(snapshot); return 2; }
    }
    for (catalyst::NUINT j = 0; j < catdv_devices_get_count(snapshot); ++j) {
        const CATDVLIB_DEVICE* d = catdv_devices_get(snapshot, j);
        if (selected && d != selected) continue;
        catalyst::BOOL supported = catalyst::FALSE;
        if (catdv_device_has_output(d, &supported).status != catalyst::STATUS_CODE_SUCCESS || !supported) continue;
        Fixture f;
        std::memset(&f, 0, sizeof(f));
        f.device = d;
        print_device(j, d);
        r = catot_device_open(d, &f.resource);
        if (r.status != catalyst::STATUS_CODE_SUCCESS || !f.resource) {
            std::printf("  Open unavailable or failed: %02X %02X %02X %02X\n",
                (unsigned) r.status, (unsigned) r.context, (unsigned) r.operation, (unsigned) r.detail);
            if (r.status == catalyst::STATUS_CODE_ERROR_NOT_FOUND || r.status == catalyst::STATUS_CODE_ERROR_ACCESS_DENIED ||
                r.status == catalyst::STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED || r.status == catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED)
                ++unavailable;
            else ++failed;
            continue;
        }
        ++opened;
        if (!lifecycleDone) {
            // Close this probe before measuring lifecycle; some devices allow only one open.
            r = catot_device_close(f.resource);
            if (r.status != catalyst::STATUS_CODE_SUCCESS) { ++failed; break; }
            f.resource = 0;
            perf_record(benchmark_io("open / close", lifecycle, &f, 0x01, 64, false), &failed, &unavailable);
            lifecycleDone = true;
            r = catot_device_open(d, &f.resource);
            if (r.status != catalyst::STATUS_CODE_SUCCESS || !f.resource) { ++failed; continue; }
        }

        if (!countDone) {
            f.field = 0x04;
            int measured = benchmark_io("channel count", query, &f, f.field, 16777216UL);
            perf_record(measured, &failed, &unavailable);
            countDone = measured != 2;
        }
        catalyst::NUINT count = 0;
        if (catot_device_get_channel_count(f.resource, &count).status == catalyst::STATUS_CODE_SUCCESS) {
            if (count && !metadataDone) {
                metadataDone = true;
                f.channel = 0; f.field = 0x05;
                perf_record(benchmark_io("channel type", query, &f, f.field, 16777216UL), &failed, &unavailable);
                f.field = 0x06;
                perf_record(benchmark_io("channel identifier", query, &f, f.field, 16777216UL), &failed, &unavailable);
            }
            for (catalyst::NUINT k = 0; k < count; ++k) {
                CATOTLIB_CHANNEL_TYPE type = CATOTLIB_CHANNEL_UNKNOWN;
                if (catot_channel_get_type(f.resource, k, &type).status != catalyst::STATUS_CODE_SUCCESS) continue;
                f.channel = k;
                if (type == CATOTLIB_CHANNEL_DIGITAL && !digital) {
                    catalyst::BOOL probe = catalyst::FALSE;
                    catalyst::RESULT stateResult = catot_digital_get_state(f.resource, k, &probe);
                    if (stateResult.status != catalyst::STATUS_CODE_SUCCESS) {
                        // A channel may exist but not support a state query.
                        if (stateResult.status != catalyst::STATUS_CODE_ERROR_NOT_FOUND &&
                            stateResult.status != catalyst::STATUS_CODE_ERROR_ACCESS_DENIED &&
                            stateResult.status != catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED &&
                            stateResult.status != catalyst::STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED) ++failed;
                        continue;
                    }
                    digital = true; f.field = 0x07;
                    double cNs = 0;
                    perf_record(benchmark_io("digital state", query, &f, f.field, 16777216UL, false, &cNs), &failed, &unavailable);
#if CATALYST_SUPPORTS_32BIT
                    CATDVLIB_IDENTIFIER channelId = {0, 0};
                    catalyst::BOOL state = catalyst::FALSE;
                    if (catot_channel_get_identifier(f.resource, k, &channelId).status == catalyst::STATUS_CODE_SUCCESS &&
                        catot_digital_get_state(f.resource, k, &state).status == catalyst::STATUS_CODE_SUCCESS)
                        benchmark_native_state(d, channelId, true, false, true, state ? 1 : 0, 0, cNs);
#endif
                    if (write && catot_digital_get_state(f.resource, k, &f.state).status == catalyst::STATUS_CODE_SUCCESS) {
                        f.field = 0x08;
                        int measured = benchmark_io("reapply digital state (through completion)", completed_write, &f, f.field, 16, false);
                        perf_record(measured, &failed, &unavailable);
                        if (measured == 0) ++completedWriteBenchmarks;
                        if (measured != 0) break;
#if TARGET_PLATFORM_LINUX
                        CATDVLIB_IDENTIFIER id = {0, 0};
                        unsigned eventType, eventCode;
                        if (catot_channel_get_identifier(f.resource, k, &id).status == catalyst::STATUS_CODE_SUCCESS &&
                            valid_identifier(id) && std::strcmp((const char*) id.scheme, "linux-evdev") == 0 &&
                            std::sscanf((const char*) id.value, "%u:%u", &eventType, &eventCode) == 2 &&
                            eventType == EV_LED && eventCode < LED_CNT) {
                            std::memset(&f.native, 0, sizeof(f.native));
                            f.native.type = EV_LED; f.native.code = (unsigned short) eventCode;
                            f.native.value = f.state ? 1 : 0;
                            f.field = 0x03;
                            measured = benchmark_io("native LED record submission (through completion)", completed_write, &f, f.field, 16, false);
                            perf_record(measured, &failed, &unavailable);
                            if (measured == 0) ++completedWriteBenchmarks;
                        if (measured != 0) break;
                        }
#endif
                    }

                }
#if CATALYST_SUPPORTS_32BIT
                if (type == CATOTLIB_CHANNEL_ABSOLUTE && !absolute) {
                    catalyst::INT probeLow = 0, probeHigh = 0, probeValue = 0;
                    catalyst::RESULT rangeResult = catot_absolute_get_range(f.resource, k, &probeLow, &probeHigh);
                    catalyst::RESULT valueResult = catot_absolute_get_value(f.resource, k, &probeValue);
                    if (rangeResult.status != catalyst::STATUS_CODE_SUCCESS || valueResult.status != catalyst::STATUS_CODE_SUCCESS) {
                        catalyst::STATUS_CODE statuses[] = {rangeResult.status, valueResult.status};
                        for (catalyst::BYTE n = 0; n < 2; ++n)
                            if (statuses[n] != catalyst::STATUS_CODE_SUCCESS &&
                                statuses[n] != catalyst::STATUS_CODE_ERROR_NOT_FOUND &&
                                statuses[n] != catalyst::STATUS_CODE_ERROR_ACCESS_DENIED &&
                                statuses[n] != catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED &&
                                statuses[n] != catalyst::STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED) ++failed;
                        continue;
                    }
                    absolute = true; f.field = 0x09;
                    double cRange = 0, cValue = 0;
                    perf_record(benchmark_io("absolute range", query, &f, f.field, 16777216UL, true, &cRange), &failed, &unavailable);
                    CATDVLIB_IDENTIFIER channelId = {0, 0};
                    catalyst::INT low = 0, high = 0, value = 0;
                    bool identified = catot_channel_get_identifier(f.resource, k, &channelId).status == catalyst::STATUS_CODE_SUCCESS;
                    if (identified && catot_absolute_get_range(f.resource, k, &low, &high).status == catalyst::STATUS_CODE_SUCCESS)
                        benchmark_native_state(d, channelId, true, true, false, low, high, cRange);
                    f.field = 0x0A;
                    perf_record(benchmark_io("absolute value", query, &f, f.field, 16777216UL, false, &cValue), &failed, &unavailable);
                    if (identified && catot_absolute_get_value(f.resource, k, &value).status == catalyst::STATUS_CODE_SUCCESS)
                        benchmark_native_state(d, channelId, true, false, false, value, 0, cValue);
                    if (write && catot_absolute_get_value(f.resource, k, &f.value).status == catalyst::STATUS_CODE_SUCCESS) {
                        f.field = 0x0B;
                        int measured = benchmark_io("reapply absolute value (through completion)", completed_write, &f, f.field, 16, false);
                        perf_record(measured, &failed, &unavailable);
                        if (measured == 0) ++completedWriteBenchmarks;
                        if (measured != 0) break;
                    }
                }

#endif
            }
        }
        r = catot_device_close(f.resource);
#if TARGET_PLATFORM_DARWIN
        std::time_t start = std::time(0);
        while (r.status == catalyst::STATUS_CODE_ERROR_INVALID_STATE) {
            std::time_t now = std::time(0);
            if (start == (std::time_t) -1 || now == (std::time_t) -1 || now < start || std::difftime(now, start) >= 5.0) break;
            r = catot_device_close(f.resource);
        }
#endif
        if (r.status != catalyst::STATUS_CODE_SUCCESS) ++failed;
        // Each operation is measured on its first suitable representative.
        if (write) break;
        if (lifecycleDone && countDone && metadataDone && digital
#if CATALYST_SUPPORTS_32BIT
            && absolute
#endif
            ) break;
    }
    if (!opened) { std::printf("No accessible output device.\n"); ++unavailable; }
    else {
        if (!metadataDone) { std::printf("Channel metadata: unavailable\n"); ++unavailable; }
        if (!digital) { std::printf("Digital output state: unavailable\n"); ++unavailable; }
#if CATALYST_SUPPORTS_32BIT
        if (!absolute) { std::printf("Absolute output range/value: unavailable\n"); ++unavailable; }
#endif
    }
    if (!write) std::printf("Output write timing not requested.\n");
    else if (!completedWriteBenchmarks) { std::printf("No completed output write benchmark.\n"); ++unavailable; }
#if !TARGET_PLATFORM_LINUX
    std::printf("Raw output record timing unavailable without a device-specific known report.\n");
#endif
    if (catdv_devices_free(snapshot).status != catalyst::STATUS_CODE_SUCCESS) ++failed;
    std::printf("\nFailures: %d; unavailable benchmarks: %d\n", failed, unavailable);
    return failed ? 1 : unavailable ? 2 : 0;
}
