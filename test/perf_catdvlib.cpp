#include "device_io_native.hpp"

struct DeviceFixture { const CATDVLIB_DEVICE* device; catalyst::NUINT index; catalyst::BYTE field; };
static PerfOutcome enumerate(void*, bool cpp) {
    CATDVLIB_DEVICES* snapshot = 0;
    catalyst::RESULT r = cpp ? catalyst::device::devices_enumerate(&snapshot) : catalyst::RESULT(catdv_devices_enumerate(&snapshot));
    if (r.status != catalyst::STATUS_CODE_SUCCESS) return PerfOutcome(r, 0);
    if (!snapshot) return PerfOutcome(catalyst::RESULT(catalyst::STATUS_CODE_ERROR_INVALID_STATE, 0x01, 0xFF, 0), 0);
    catalyst::NUINT count = cpp ? catalyst::device::devices_get_count(snapshot) : catdv_devices_get_count(snapshot);
    catalyst::RESULT released = cpp ? catalyst::device::devices_free(snapshot) : catalyst::RESULT(catdv_devices_free(snapshot));
    if (released.status != catalyst::STATUS_CODE_SUCCESS) return PerfOutcome(catalyst::RESULT(released.status, 0x01, 0xFF, released.detail), 0);
    return PerfOutcome(r, (catalyst::BYTE) count);
}
static PerfOutcome query(void* fixture, bool cpp) {
    DeviceFixture* f = (DeviceFixture*) fixture;
    CATDVLIB_IDENTIFIER id = {0, 0};
    catalyst::UTF8 value = 0;
    catalyst::NUINT count = 0;
    catalyst::BOOL supported = catalyst::FALSE;
    catalyst::RESULT r;
    switch (f->field) {
        case 0x06: r = cpp ? catalyst::device::device_get_identifier(f->device, &id) : catalyst::RESULT(catdv_device_get_identifier(f->device, &id)); break;
        case 0x07: r = cpp ? catalyst::device::device_get_hwid_count(f->device, &count) : catalyst::RESULT(catdv_device_get_hwid_count(f->device, &count)); break;
        case 0x08: r = cpp ? catalyst::device::device_get_hwid(f->device, f->index, &id) : catalyst::RESULT(catdv_device_get_hwid(f->device, f->index, &id)); break;
        case 0x09: r = cpp ? catalyst::device::device_get_serial(f->device, &value) : catalyst::RESULT(catdv_device_get_serial(f->device, &value)); break;
        case 0x0A: r = cpp ? catalyst::device::device_get_manufacturer(f->device, &value) : catalyst::RESULT(catdv_device_get_manufacturer(f->device, &value)); break;
        case 0x0B: r = cpp ? catalyst::device::device_get_product(f->device, &value) : catalyst::RESULT(catdv_device_get_product(f->device, &value)); break;
        case 0x0C: r = cpp ? catalyst::device::device_get_descriptor(f->device, &value) : catalyst::RESULT(catdv_device_get_descriptor(f->device, &value)); break;
        case 0x0D: r = cpp ? catalyst::device::device_get_revision(f->device, &value) : catalyst::RESULT(catdv_device_get_revision(f->device, &value)); break;
        case 0x0E: r = cpp ? catalyst::device::device_has_input(f->device, &supported) : catalyst::RESULT(catdv_device_has_input(f->device, &supported)); break;
        default: r = cpp ? catalyst::device::device_has_output(f->device, &supported) : catalyst::RESULT(catdv_device_has_output(f->device, &supported)); break;
    }
    catalyst::BYTE digest = (catalyst::BYTE) (count ^ supported);
    if (valid_identifier(id)) digest = (catalyst::BYTE) (digest ^ id.scheme[0] ^ id.value[0]);
    if (value) digest = (catalyst::BYTE) (digest ^ value[0]);
    return PerfOutcome(r, digest);
}
int main() {
    std::printf("CATDVLIB Performance (Release recommended)\nTarget: %s/%s/%s\n", TARGET_VENDOR, TARGET_SYSTEM, TARGET_ARCHITECTURE);
    std::printf("Median of three CPU-time samples. Enumeration includes allocation and release; other queries use a cached snapshot.\n");
    std::printf("C is the public API baseline; C++ measures the equivalent wrapper. Direct native discovery, where provided, is narrower than a complete snapshot and is not used for a relative overhead claim.\n");
    int failed = 0, unavailable = 0;
    perf_record(benchmark_io("enumerate / count / free", enumerate, 0, 0x01, 64, false), &failed, &unavailable);
#if TARGET_PLATFORM_DARWIN
    benchmark_native_discovery();
#endif
    CATDVLIB_DEVICES* snapshot = 0;
    catalyst::RESULT r = catdv_devices_enumerate(&snapshot);
    if (r.status != catalyst::STATUS_CODE_SUCCESS || !snapshot) return failed ? 1 : 2;
    const char* labels[] = {"instance identifier", "HWID count", "HWID", "serial", "manufacturer", "product", "descriptor", "revision", "input capability", "output capability"};
    catalyst::NUINT count = catdv_devices_get_count(snapshot);
    for (catalyst::BYTE field = 0x06; field <= 0x0F; ++field) {
        bool found = false;
        for (catalyst::NUINT j = 0; j < count; ++j) {
            DeviceFixture f = {catdv_devices_get(snapshot, j), 0, field};
            if (field == 0x08) {
                catalyst::NUINT ids = 0;
                if (catdv_device_get_hwid_count(f.device, &ids).status != catalyst::STATUS_CODE_SUCCESS || !ids) continue;
            }
            if (query(&f, false).result.status != catalyst::STATUS_CODE_SUCCESS) continue;
            print_device(j, f.device);
            perf_record(benchmark_io(labels[field - 0x06], query, &f, field, 16777216UL), &failed, &unavailable);
            found = true; break;
        }
        if (!found) { std::printf("\n%s: unavailable\n", labels[field - 0x06]); ++unavailable; }
    }
    r = catdv_devices_free(snapshot);
    if (r.status != catalyst::STATUS_CODE_SUCCESS) ++failed;
    std::printf("\nFailures: %d; unavailable benchmarks: %d\n", failed, unavailable);
    return failed ? 1 : unavailable ? 2 : 0;
}
