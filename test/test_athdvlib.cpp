#include "device_test.hpp"
#if TARGET_PLATFORM_DARWIN
#include <CoreGraphics/CoreGraphics.h>
#elif TARGET_PLATFORM_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#undef TRUE
#undef FALSE

typedef CATALYST_RESULT (*TextQuery)(const ATHDVLIB_DEVICE*, CATALYST_UTF8*);
typedef catalyst::RESULT (*TextQueryCpp)(const ATHDVLIB_DEVICE*, catalyst::UTF8*);
struct TextCase { const char* name; catalyst::BYTE context; TextQuery c; TextQueryCpp cpp; };
static const TextCase textCases[] = {
    {"serial", 0x09, athdv_device_get_serial, catalyst::device::device_get_serial},
    {"manufacturer", 0x0A, athdv_device_get_manufacturer, catalyst::device::device_get_manufacturer},
    {"product", 0x0B, athdv_device_get_product, catalyst::device::device_get_product},
    {"descriptor", 0x0C, athdv_device_get_descriptor, catalyst::device::device_get_descriptor},
    {"revision", 0x0D, athdv_device_get_revision, catalyst::device::device_get_revision}
};

int main(int argc, char** argv) {
    bool verbose = take_verbose(&argc, &argv);
    if (argc != 1) { std::printf("Usage: test_athdvlib [--verbose]\n"); return 1; }
    std::printf("ATHDVLIB Tests\nTarget: %s/%s/%s\n", TARGET_VENDOR, TARGET_SYSTEM, TARGET_ARCHITECTURE);
    TestReport report(verbose);
    ATHDVLIB_DEVICES* snapshot = test_snapshot(&report);
    if (!snapshot) return report.finish();
    catalyst::NUINT count = athdv_devices_get_count(snapshot);
    report.check(count == catalyst::device::devices_get_count(snapshot), "C/C++ snapshot count");
    std::printf("Devices: %lu\n", (unsigned long) count);
    if (!count) report.skip("no devices discovered");
    for (catalyst::NUINT j = 0; j < count; ++j) {
        const ATHDVLIB_DEVICE* d = athdv_devices_get(snapshot, j);
        print_device(j, d);
        TestReport before = report;
        report.check(d && d == catalyst::device::devices_get(snapshot, j), "borrowed device / C++ parity");
        if (!d) { report.summary(before); continue; }
        ATHDVLIB_DEVICE_TYPE type = athdv_device_get_type(d);
        report.check(type <= ATHDVLIB_DEVICE_LOGICAL && type == catalyst::device::device_get_type(d), "device type");
        ATHDVLIB_IDENTIFIER id = {0, 0}, other = {0, 0};
        catalyst::RESULT r = athdv_device_get_identifier(d, &id), c = catalyst::device::device_get_identifier(d, &other);
        if (report.result("instance identifier", r, 0x06))
            report.check(same_identifier(id, other) && same_result(r, c), "identifier C/C++ parity");
        else report.check(!id.scheme && !id.value, "identifier failure outputs");
        catalyst::NUINT ids = 0, cppIds = 0;
        r = athdv_device_get_hwid_count(d, &ids); c = catalyst::device::device_get_hwid_count(d, &cppIds);
        if (report.result("HWID count", r, 0x07)) {
            report.check(ids == cppIds && same_result(r, c), "HWID count parity");
            for (catalyst::NUINT k = 0; k < ids; ++k) {
                ATHDVLIB_IDENTIFIER hw = {0, 0}, cppHw = {0, 0};
                r = athdv_device_get_hwid(d, k, &hw); c = catalyst::device::device_get_hwid(d, k, &cppHw);
                if (report.result("HWID", r, 0x08)) {
                    report.check(same_identifier(hw, cppHw) && same_result(r, c), "HWID parity");
                    if (valid_identifier(hw)) std::printf("      [%s] %s\n", (const char*) hw.scheme, (const char*) hw.value);
                } else report.check(!hw.scheme && !hw.value, "HWID failure outputs");
            }
        } else report.check(ids == 0, "HWID count failure output");
        for (catalyst::BYTE k = 0; k < sizeof(textCases) / sizeof(textCases[0]); ++k) {
            const TextCase& field = textCases[k];
            catalyst::UTF8 value = 0, cpp = 0;
            r = field.c(d, &value); c = field.cpp(d, &cpp);
            report.check(same_result(r, c), "metadata result parity");
            if (report.result(field.name, r, field.context))
                report.check(value && *value && value == cpp, "borrowed metadata");
            else report.check(!value && !cpp, "metadata failure outputs");
        }
        catalyst::BOOL supported = catalyst::FALSE, cppSupported = catalyst::FALSE;
        r = athdv_device_has_input(d, &supported); c = catalyst::device::device_has_input(d, &cppSupported);
        if (report.result("input capability", r, 0x0E))
            report.check(same_result(r, c) && supported == cppSupported, "input capability parity");
        else report.check(!supported, "input failure output");
        supported = cppSupported = catalyst::FALSE;
        r = athdv_device_has_output(d, &supported); c = catalyst::device::device_has_output(d, &cppSupported);
        if (report.result("output capability", r, 0x0F))
            report.check(same_result(r, c) && supported == cppSupported, "output capability parity");
        else report.check(!supported, "output failure output");
        // Instance lookup must identify this exact borrowed record.
        ATHDVLIB_IDENTIFIER lookupId = {0, 0};
        if (athdv_device_get_identifier(d, &lookupId).status == catalyst::STATUS_CODE_SUCCESS) {
            const ATHDVLIB_DEVICE* match = 0;
            catalyst::RESULT found = athdv_devices_find_identifier(snapshot, &lookupId, &match);
            const ATHDVLIB_DEVICE* cppMatch = 0;
            catalyst::RESULT cppFound = catalyst::device::devices_find_identifier(snapshot, &lookupId, &cppMatch);
            report.check(same_result(found, cppFound) && match == cppMatch, "identifier lookup parity");
            if (report.result("instance lookup", found, 0x12)) report.check(match == d, "instance lookup identity");
        }
        catalyst::BOOL display = athdv_device_is_display(d);
        report.check(display == catalyst::device::device_is_display(d), "display classification parity");
        if (display) {
            std::printf("  Display endpoint\n");
            const CATALYST_BYTE* bytes = 0, *cppBytes = 0;
            catalyst::NUINT size = 0, cppSize = 0;
            catalyst::RESULT er = athdv_display_get_edid(d, &bytes, &size);
            catalyst::RESULT ec = catalyst::device::display_get_edid(d, &cppBytes, &cppSize);
            report.check(same_result(er, ec) && bytes == cppBytes && size == cppSize, "EDID parity");
            if (er.status == catalyst::STATUS_CODE_ERROR_INVALID_STATE) report.skip("ambiguous EDID association");
            else if (report.result("EDID acquisition", er, 0x10)) {
                report.check(bytes && size, "borrowed EDID extent");
                std::printf("  EDID: %lu bytes\n", (unsigned long) size);
            }
            if (er.status != catalyst::STATUS_CODE_SUCCESS) report.check(!bytes && !size, "EDID failure outputs");
            catalyst::NUINT handle = 0, cppHandle = 0;
            catalyst::RESULT hr = athdv_display_get_native_handle(d, &handle);
            catalyst::RESULT hc = catalyst::device::display_get_native_handle(d, &cppHandle);
            report.check(same_result(hr, hc) && handle == cppHandle, "native handle parity");
            if (report.result("native display handle", hr, 0x11)) {
                const ATHDVLIB_DEVICE* match = 0;
                catalyst::RESULT found = athdv_devices_find_native_display(snapshot, handle, &match);
                const ATHDVLIB_DEVICE* cppMatch = 0;
                catalyst::RESULT cppFound = catalyst::device::devices_find_native_display(snapshot, handle, &cppMatch);
                report.check(same_result(found, cppFound) && match == cppMatch, "native display lookup parity");
                if (found.status == catalyst::STATUS_CODE_ERROR_INVALID_STATE) {
                    report.check(!match, "ambiguous native handle failure output");
                    report.skip("native display handle identifies multiple endpoints");
                } else if (report.result("native display lookup", found, 0x13))
                    report.check(match == d, "native display lookup identity");
                std::printf("  Native display handle: %p\n", (void*) handle);
            } else report.check(!handle, "native handle failure output");
        }
        report.summary(before);
    }
    // Obtain an identity from the native system, independently of Catalyst.
#if TARGET_PLATFORM_WIN32 || TARGET_PLATFORM_DARWIN
    catalyst::NUINT nativePrimary = 0;
#if TARGET_PLATFORM_DARWIN
    nativePrimary = (catalyst::NUINT) CGMainDisplayID();
#elif TARGET_PLATFORM_WIN32
    POINT origin = { 0, 0 };
    nativePrimary = (catalyst::NUINT) MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
#endif
    if (nativePrimary) {
        const ATHDVLIB_DEVICE* primary = 0;
        catalyst::RESULT pr = athdv_devices_find_native_display(snapshot, nativePrimary, &primary);
        if (pr.status == catalyst::STATUS_CODE_ERROR_INVALID_STATE) {
            report.check(!primary && pr.context == 0x13 && pr.operation == 0x02 && !pr.detail, "ambiguous native primary result");
            report.skip("native primary display maps to several endpoints");
        } else {
            report.check(pr.status == catalyst::STATUS_CODE_SUCCESS && pr.context == 0x13 &&
                pr.operation == 0x04 && !pr.detail && primary && athdv_device_is_display(primary), "independent native primary display lookup");
            if (primary) {
                catalyst::NUINT roundTrip = 0;
                catalyst::RESULT hr = athdv_display_get_native_handle(primary, &roundTrip);
                report.check(hr.status == catalyst::STATUS_CODE_SUCCESS && roundTrip == nativePrimary, "native primary handle identity");
                std::printf("Native primary display resolved successfully.\n");
            }
        }
    } else report.skip("no native primary display");
#else
    const ATHDVLIB_DEVICE* unsupported = 0;
    catalyst::RESULT nr = athdv_devices_find_native_display(snapshot, 0, &unsupported);
    report.check(nr.status == catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED && nr.context == 0x13 &&
        nr.operation == 0x05 && !nr.detail && !unsupported, "unsupported numeric native display lookup");
#endif
    ATHDVLIB_IDENTIFIER absent = { (CATALYST_ASCII) "test-absent-scheme", (CATALYST_UTF8) "absent" };
    const ATHDVLIB_DEVICE* missing = 0;
    catalyst::RESULT notFound = athdv_devices_find_identifier(snapshot, &absent, &missing);
    report.check(notFound.status == catalyst::STATUS_CODE_ERROR_NOT_FOUND && notFound.context == 0x12 &&
        notFound.operation == 0x03 && !notFound.detail && !missing, "missing instance lookup");
    report.result("free snapshot", catalyst::device::devices_free(snapshot), 0x02);
    snapshot = 0;
    catalyst::RESULT r = catalyst::device::devices_enumerate(&snapshot);
    if (report.result("C++ re-enumeration", r, 0x01)) report.result("C free", athdv_devices_free(snapshot), 0x02);
    else report.check(snapshot == 0, "re-enumeration failure output");
    return report.finish();
}
