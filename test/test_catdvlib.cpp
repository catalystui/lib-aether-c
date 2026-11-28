#include "device_io_test.hpp"

typedef CATALYST_RESULT (*TextQuery)(const CATDVLIB_DEVICE*, CATALYST_UTF8*);
typedef RESULT (*TextQueryCpp)(const CATDVLIB_DEVICE*, UTF8*);
struct TextCase { const char* name; BYTE context; TextQuery c; TextQueryCpp cpp; };
static const TextCase textCases[] = {
    {"serial", 0x09, catdv_device_get_serial, device::device_get_serial},
    {"manufacturer", 0x0A, catdv_device_get_manufacturer, device::device_get_manufacturer},
    {"product", 0x0B, catdv_device_get_product, device::device_get_product},
    {"descriptor", 0x0C, catdv_device_get_descriptor, device::device_get_descriptor},
    {"revision", 0x0D, catdv_device_get_revision, device::device_get_revision}
};

int main() {
    std::printf("CATDVLIB Tests\nTarget: %s/%s/%s\n", TARGET_VENDOR, TARGET_SYSTEM, TARGET_ARCHITECTURE);
    TestReport report;
    CATDVLIB_DEVICES* snapshot = test_snapshot(&report);
    if (!snapshot) return report.finish();
    NUINT count = catdv_devices_get_count(snapshot);
    report.check(count == device::devices_get_count(snapshot), "C/C++ snapshot count");
    std::printf("Devices: %lu\n", (unsigned long) count);
    if (!count) report.skip("no devices discovered");
    for (NUINT j = 0; j < count; ++j) {
        const CATDVLIB_DEVICE* d = catdv_devices_get(snapshot, j);
        print_device(j, d);
        report.check(d && d == device::devices_get(snapshot, j), "borrowed device / C++ parity");
        if (!d) continue;
        CATDVLIB_DEVICE_TYPE type = catdv_device_get_type(d);
        report.check(type <= CATDVLIB_DEVICE_LOGICAL && type == device::device_get_type(d), "device type");
        CATDVLIB_IDENTIFIER id = {0, 0}, other = {0, 0};
        RESULT r = catdv_device_get_identifier(d, &id), c = device::device_get_identifier(d, &other);
        if (report.result("instance identifier", r, 0x06))
            report.check(same_identifier(id, other) && same_result(r, c), "identifier C/C++ parity");
        else report.check(!id.scheme && !id.value, "identifier failure outputs");
        NUINT ids = 0, cppIds = 0;
        r = catdv_device_get_hwid_count(d, &ids); c = device::device_get_hwid_count(d, &cppIds);
        if (report.result("HWID count", r, 0x07)) {
            report.check(ids == cppIds && same_result(r, c), "HWID count parity");
            for (NUINT k = 0; k < ids; ++k) {
                CATDVLIB_IDENTIFIER hw = {0, 0}, cppHw = {0, 0};
                r = catdv_device_get_hwid(d, k, &hw); c = device::device_get_hwid(d, k, &cppHw);
                if (report.result("HWID", r, 0x08)) {
                    report.check(same_identifier(hw, cppHw) && same_result(r, c), "HWID parity");
                    if (valid_identifier(hw)) std::printf("      [%s] %s\n", (const char*) hw.scheme, (const char*) hw.value);
                } else report.check(!hw.scheme && !hw.value, "HWID failure outputs");
            }
        } else report.check(ids == 0, "HWID count failure output");
        for (BYTE k = 0; k < sizeof(textCases) / sizeof(textCases[0]); ++k) {
            const TextCase& field = textCases[k];
            UTF8 value = 0, cpp = 0;
            r = field.c(d, &value); c = field.cpp(d, &cpp);
            report.check(same_result(r, c), "metadata result parity");
            if (report.result(field.name, r, field.context))
                report.check(value && *value && value == cpp, "borrowed metadata");
            else report.check(!value && !cpp, "metadata failure outputs");
        }
        BOOL supported = FALSE, cppSupported = FALSE;
        r = catdv_device_has_input(d, &supported); c = device::device_has_input(d, &cppSupported);
        if (report.result("input capability", r, 0x0E))
            report.check(same_result(r, c) && supported == cppSupported, "input capability parity");
        else report.check(!supported, "input failure output");
        supported = cppSupported = FALSE;
        r = catdv_device_has_output(d, &supported); c = device::device_has_output(d, &cppSupported);
        if (report.result("output capability", r, 0x0F))
            report.check(same_result(r, c) && supported == cppSupported, "output capability parity");
        else report.check(!supported, "output failure output");
    }
    report.result("free snapshot", device::devices_free(snapshot), 0x02);
    snapshot = 0;
    RESULT r = device::devices_enumerate(&snapshot);
    if (report.result("C++ re-enumeration", r, 0x01)) report.result("C free", catdv_devices_free(snapshot), 0x02);
    else report.check(snapshot == 0, "re-enumeration failure output");
    return report.finish();
}
