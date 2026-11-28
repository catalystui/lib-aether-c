#include "device_native.hpp"

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
        case 0x0F: r = cpp ? catalyst::device::device_has_output(f->device, &supported) : catalyst::RESULT(catdv_device_has_output(f->device, &supported)); break;
        case 0x10: {
            const CATALYST_BYTE* bytes = 0;
            r = cpp ? catalyst::device::display_get_edid(f->device, &bytes, &count) : catalyst::RESULT(catdv_display_get_edid(f->device, &bytes, &count));
            if (bytes && count) supported = bytes[0];
            break;
        }
        default: r = cpp ? catalyst::device::display_get_native_handle(f->device, &count) : catalyst::RESULT(catdv_display_get_native_handle(f->device, &count)); break;
    }
    catalyst::BYTE digest = (catalyst::BYTE) (count ^ supported);
    if (valid_identifier(id)) digest = (catalyst::BYTE) (digest ^ id.scheme[0] ^ id.value[0]);
    if (value) digest = (catalyst::BYTE) (digest ^ value[0]);
    return PerfOutcome(r, digest);
}
struct LookupFixture {
    const CATDVLIB_DEVICES* snapshot;
    CATDVLIB_IDENTIFIER identifier;
    catalyst::NUINT native;
    bool byHandle;
};
static PerfOutcome lookup(void* fixture, bool cpp) {
    LookupFixture* f = (LookupFixture*) fixture;
    const CATDVLIB_DEVICE* device = 0;
    catalyst::RESULT r;
    if (f->byHandle)
        r = cpp ? catalyst::device::devices_find_native_display(f->snapshot, f->native, &device) :
            catalyst::RESULT(catdv_devices_find_native_display(f->snapshot, f->native, &device));
    else
        r = cpp ? catalyst::device::devices_find_identifier(f->snapshot, &f->identifier, &device) :
            catalyst::RESULT(catdv_devices_find_identifier(f->snapshot, &f->identifier, &device));
    return PerfOutcome(r, device ? 1 : 0);
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
    const char* labels[] = {"instance identifier", "HWID count", "HWID", "serial", "manufacturer", "product", "descriptor", "revision", "input capability", "output capability", "cached EDID", "native display handle"};
    catalyst::NUINT count = catdv_devices_get_count(snapshot);
    for (catalyst::BYTE field = 0x06; field <= 0x11; ++field) {
        bool found = false;
        for (catalyst::NUINT j = 0; j < count; ++j) {
            DeviceFixture f = {catdv_devices_get(snapshot, j), 0, field};
            if (field == 0x08) {
                catalyst::NUINT ids = 0;
                if (catdv_device_get_hwid_count(f.device, &ids).status != catalyst::STATUS_CODE_SUCCESS || !ids) continue;
            }
            if (field >= 0x10 && !catdv_device_is_display(f.device)) continue;
            if (query(&f, false).result.status != catalyst::STATUS_CODE_SUCCESS) continue;
            print_device(j, f.device);
            perf_record(benchmark_io(labels[field - 0x06], query, &f, field, 16777216UL), &failed, &unavailable);
            found = true; break;
        }
        if (!found) { std::printf("\n%s: unavailable\n", labels[field - 0x06]); ++unavailable; }
    }
    for (catalyst::BYTE mode = 0; mode < 2; ++mode) {
        bool found = false;
        const char* label = mode ? "native display lookup" : "instance identifier lookup";
        for (catalyst::NUINT j = 0; j < count; ++j) {
            const CATDVLIB_DEVICE* entry = catdv_devices_get(snapshot, j);
            LookupFixture f = { snapshot, {0, 0}, 0, mode != 0 };
            if (mode) {
                if (!catdv_device_is_display(entry) ||
                    catdv_display_get_native_handle(entry, &f.native).status != catalyst::STATUS_CODE_SUCCESS) continue;
            } else if (catdv_device_get_identifier(entry, &f.identifier).status != catalyst::STATUS_CODE_SUCCESS) continue;
            const CATDVLIB_DEVICE* match = 0;
            catalyst::RESULT preflight = mode ?
                catalyst::RESULT(catdv_devices_find_native_display(snapshot, f.native, &match)) :
                catalyst::RESULT(catdv_devices_find_identifier(snapshot, &f.identifier, &match));
            if (preflight.status != catalyst::STATUS_CODE_SUCCESS) continue;
            if (match != entry) { ++failed; continue; }
            print_device(j, entry);
            perf_record(benchmark_io(label, lookup, &f, mode ? 0x13 : 0x12, 1048576UL), &failed, &unavailable);
            found = true; break;
        }
        if (!found) { std::printf("\n%s: unavailable\n", label); ++unavailable; }
    }
    r = catdv_devices_free(snapshot);
    if (r.status != catalyst::STATUS_CODE_SUCCESS) ++failed;
    std::printf("\nFailures: %d; unavailable benchmarks: %d\n", failed, unavailable);
    return failed ? 1 : unavailable ? 2 : 0;
}
