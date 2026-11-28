#include "device_io_test.hpp"
#include "catotlib.hpp"

/* Writes are opt-in and reapply the currently observed value only. */
static void test_output(TestReport* report, CATOTLIB_OUTPUT* output, bool write) {
    NUINT count = 0, cppCount = 0;
    RESULT r = catot_device_get_channel_count(output, &count);
    RESULT c = catalyst::output::device_get_channel_count(output, &cppCount);
    report->check(same_result(r, c) && count == cppCount, "channel count parity");
    if (report->result("channel count", r, 0x04)) {
        if (!count) report->skip("no typed channels");
        for (NUINT j = 0; j < count; ++j) {
            CATOTLIB_CHANNEL_TYPE type = CATOTLIB_CHANNEL_UNKNOWN, cppType = CATOTLIB_CHANNEL_UNKNOWN;
            r = catot_channel_get_type(output, j, &type); c = catalyst::output::channel_get_type(output, j, &cppType);
            if (!report->result("channel type", r, 0x05)) continue;
            report->check(type <= CATOTLIB_CHANNEL_ABSOLUTE && type == cppType && same_result(r, c), "channel type parity");
            CATDVLIB_IDENTIFIER id = {0, 0}, cppId = {0, 0};
            r = catot_channel_get_identifier(output, j, &id); c = catalyst::output::channel_get_identifier(output, j, &cppId);
            if (report->result("channel identifier", r, 0x06))
                report->check(same_identifier(id, cppId) && same_result(r, c), "channel identifier parity");
            else report->check(!id.scheme && !id.value, "channel identifier failure outputs");
            if (type == CATOTLIB_CHANNEL_DIGITAL) {
                BOOL state = FALSE;
                r = catot_digital_get_state(output, j, &state);
                if (report->result("digital state", r, 0x07)) {
                    report->check(state == TRUE || state == FALSE, "digital state representation");
                    if (write) {
                        for (unsigned long attempt = 0; attempt < 100000; ++attempt) {
                            r = catalyst::output::digital_set_state(output, j, state);
                            if (r.status != STATUS_CODE_SUCCESS_NOOP) break;
                        }
                        if (r.status == STATUS_CODE_SUCCESS_NOOP) { report->skip("digital write still pending; stop submitting"); break; }
                        report->result("reapply digital state", r, 0x08);
                    }
                } else report->check(!state, "state failure output");
            }
#if CATALYST_SUPPORTS_32BIT
            if (type == CATOTLIB_CHANNEL_ABSOLUTE) {
                INT low = 0, high = 0, cppLow = 0, cppHigh = 0, value = 0;
                r = catot_absolute_get_range(output, j, &low, &high); c = catalyst::output::absolute_get_range(output, j, &cppLow, &cppHigh);
                if (report->result("absolute range", r, 0x09))
                    report->check(low <= high && low == cppLow && high == cppHigh && same_result(r, c), "absolute range parity");
                else report->check(!low && !high, "range failure outputs");
                r = catot_absolute_get_value(output, j, &value);
                if (report->result("absolute value", r, 0x0A)) {
                    if (write) {
                        for (unsigned long attempt = 0; attempt < 100000; ++attempt) {
                            r = catalyst::output::absolute_set_value(output, j, value);
                            if (r.status != STATUS_CODE_SUCCESS_NOOP) break;
                        }
                        if (r.status == STATUS_CODE_SUCCESS_NOOP) { report->skip("absolute write still pending; stop submitting"); break; }
                        report->result("reapply absolute value", r, 0x0B);
                    }
                } else report->check(value == 0, "value failure output");
            }
#endif
        }
    } else report->check(!count, "channel count failure output");
    /* Zero bytes cannot be a complete native record; no hardware is changed. */
    BYTE dummy = 0;
    NUINT written = 0;
    r = catot_device_write(output, &dummy, 0, &written);
    if (r.status == STATUS_CODE_ERROR_INVALID_ARGUMENT)
        report->check(r.context == 0x03 && r.operation == 0x01 && r.detail == 2, "reject incomplete native record");
    else if (r.status == STATUS_CODE_ERROR_NOT_IMPLEMENTED) report->result("native output record", r, 0x03);
    else report->check(false, "reject incomplete native record");
    report->check(!written, "failed write extent");
}

int main(int argc, char** argv) {
    std::printf("CATOTLIB Tests\nTarget: %s/%s/%s\n", TARGET_VENDOR, TARGET_SYSTEM, TARGET_ARCHITECTURE);
    NUINT selected = 0;
    bool write = argc == 3 && std::strcmp(argv[1], "--write") == 0 && parse_index(argv[2], &selected);
    if (argc != 1 && !write) { std::printf("Usage: test_catotlib [--write DEVICE_INDEX]\n"); return 1; }
    if (!write) std::printf("Read-only verification; --write DEVICE_INDEX explicitly enables same-value writes.\n");
    TestReport report;
    CATDVLIB_DEVICES* snapshot = test_snapshot(&report);
    if (!snapshot) return report.finish();
    NUINT opened = 0, count = catdv_devices_get_count(snapshot);
    if (write && selected >= count) report.check(false, "selected device exists");
    for (NUINT j = 0; j < count; ++j) {
        if (write && j != selected) continue;
        const CATDVLIB_DEVICE* d = catdv_devices_get(snapshot, j);
        BOOL supported = FALSE;
        RESULT r = catdv_device_has_output(d, &supported);
        if (r.status != STATUS_CODE_SUCCESS || !supported) continue;
        print_device(j, d);
        CATOTLIB_OUTPUT* output = 0;
        r = catot_device_open(d, &output);
        bool ok = report.result("open output", r, 0x01);
        report.check((r.status == STATUS_CODE_SUCCESS) == (output != 0), "output ownership");
        if (!ok || !output) continue;
        ++opened;
        test_output(&report, output, write);
        r = catalyst::output::device_close(output);
        /* Darwin may need to finish the asynchronous request before close. */
        for (unsigned long attempt = 0; r.status == STATUS_CODE_ERROR_INVALID_STATE && attempt < 100000; ++attempt)
            r = catalyst::output::device_close(output);
        report.result("close output", r, 0x02);
    }
    if (!opened) report.skip("no accessible output device");
    if (!write) report.skip("hardware writes not requested");
    report.result("free snapshot", catdv_devices_free(snapshot), 0x02);
    return report.finish();
}
