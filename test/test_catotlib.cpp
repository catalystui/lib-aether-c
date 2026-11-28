#include "device_io_test.hpp"
#include "catotlib.hpp"
#if TARGET_PLATFORM_LINUX
#include <linux/input.h>
#endif

/* Writes are opt-in and reapply the currently observed value only. */
static void test_output(TestReport* report, CATOTLIB_OUTPUT* output, bool write) {
    catalyst::NUINT count = 0, cppCount = 0;
    catalyst::RESULT r = catot_device_get_channel_count(output, &count);
    catalyst::RESULT c = catalyst::output::device_get_channel_count(output, &cppCount);
    report->check(same_result(r, c) && count == cppCount, "channel count parity");
    if (report->result("channel count", r, 0x04)) {
        if (!count) report->skip("no channels");
        catalyst::NUINT rawOnly = 0;
        for (catalyst::NUINT j = 0; j < count; ++j) {
            CATOTLIB_CHANNEL_TYPE type = CATOTLIB_CHANNEL_UNKNOWN, cppType = CATOTLIB_CHANNEL_UNKNOWN;
            r = catot_channel_get_type(output, j, &type); c = catalyst::output::channel_get_type(output, j, &cppType);
            if (!report->result("channel type", r, 0x05)) continue;
            report->check(type <= CATOTLIB_CHANNEL_ABSOLUTE && type == cppType && same_result(r, c), "channel type parity");
            CATDVLIB_IDENTIFIER id = {0, 0}, cppId = {0, 0};
            r = catot_channel_get_identifier(output, j, &id); c = catalyst::output::channel_get_identifier(output, j, &cppId);
            if (report->result("channel identifier", r, 0x06))
                report->check(same_identifier(id, cppId) && same_result(r, c), "channel identifier parity");
            else report->check(!id.scheme && !id.value, "channel identifier failure outputs");
            if (type == CATOTLIB_CHANNEL_UNKNOWN) ++rawOnly;
            if (type == CATOTLIB_CHANNEL_DIGITAL) {
                catalyst::BOOL state = catalyst::FALSE;
                r = catot_digital_get_state(output, j, &state);
                if (report->result("digital state", r, 0x07)) {
                    report->check(state == catalyst::TRUE || state == catalyst::FALSE, "digital state representation");
                    if (write) {
                        std::time_t start = std::time(0);
                        for (;;) {
                            r = catalyst::output::digital_set_state(output, j, state);
                            if (r.status != catalyst::STATUS_CODE_SUCCESS_NOOP) break;
                            std::time_t now = std::time(0);
                            if (start == (std::time_t) -1 || now == (std::time_t) -1 || now < start || std::difftime(now, start) >= 5.0) break;
                        }
                        if (r.status == catalyst::STATUS_CODE_SUCCESS_NOOP) { report->check(false, "digital write completion not observed"); return; }
                        if (report->result("reapply digital state", r, 0x08)) {
                            catalyst::BOOL observed = catalyst::FALSE;
                            catalyst::RESULT readback = catot_digital_get_state(output, j, &observed);
                            if (report->result("digital write readback", readback, 0x07))
                                report->check(observed == state, "digital state after native completion");
#if TARGET_PLATFORM_LINUX
                            unsigned eventType, eventCode;
                            if (valid_identifier(id) && std::strcmp((const char*) id.scheme, "linux-evdev") == 0 &&
                                std::sscanf((const char*) id.value, "%u:%u", &eventType, &eventCode) == 2 &&
                                eventType == EV_LED && eventCode < LED_CNT) {
                                struct input_event event;
                                std::memset(&event, 0, sizeof(event));
                                event.type = EV_LED; event.code = (unsigned short) eventCode; event.value = state ? 1 : 0;
                                catalyst::NUINT written = 0;
                                r = catalyst::output::device_write(output, &event, sizeof(event), &written);
                                if (report->result("native LED record", r, 0x03, true))
                                    report->check(r.status == catalyst::STATUS_CODE_SUCCESS ? written == sizeof(event) : written == 0, "native write extent");
                            } else report->skip("no known native LED record");
#endif
                        }
                    }
                } else report->check(!state, "state failure output");
            }
#if CATALYST_SUPPORTS_32BIT
            if (type == CATOTLIB_CHANNEL_ABSOLUTE) {
                catalyst::INT low = 0, high = 0, cppLow = 0, cppHigh = 0, value = 0;
                r = catot_absolute_get_range(output, j, &low, &high); c = catalyst::output::absolute_get_range(output, j, &cppLow, &cppHigh);
                if (report->result("absolute range", r, 0x09))
                    report->check(low <= high && low == cppLow && high == cppHigh && same_result(r, c), "absolute range parity");
                else report->check(!low && !high, "range failure outputs");
                r = catot_absolute_get_value(output, j, &value);
                if (report->result("absolute value", r, 0x0A)) {
                    if (write) {
                        std::time_t start = std::time(0);
                        for (;;) {
                            r = catalyst::output::absolute_set_value(output, j, value);
                            if (r.status != catalyst::STATUS_CODE_SUCCESS_NOOP) break;
                            std::time_t now = std::time(0);
                            if (start == (std::time_t) -1 || now == (std::time_t) -1 || now < start || std::difftime(now, start) >= 5.0) break;
                        }
                        if (r.status == catalyst::STATUS_CODE_SUCCESS_NOOP) { report->check(false, "absolute write completion not observed"); return; }
                        if (report->result("reapply absolute value", r, 0x0B)) {
                            catalyst::INT observed = 0;
                            catalyst::RESULT readback = catot_absolute_get_value(output, j, &observed);
                            if (report->result("absolute write readback", readback, 0x0A))
                                report->check(observed == value, "absolute value after native completion");
                        }
                    }
                } else report->check(value == 0, "value failure output");
            }
#endif
        }
        std::printf("Channels: %lu total; %lu without scalar interpretation\n",
            (unsigned long) count, (unsigned long) rawOnly);
    } else report->check(!count, "channel count failure output");
    /* Zero bytes cannot be a complete native record; no hardware is changed. */
    catalyst::BYTE dummy = 0;
    catalyst::NUINT written = 0;
    r = catot_device_write(output, &dummy, 0, &written);
    if (r.status == catalyst::STATUS_CODE_ERROR_INVALID_ARGUMENT)
        report->check(r.context == 0x03 &&
#if TARGET_PLATFORM_DARWIN
            r.operation == 0x07 &&
#else
            r.operation == 0x01 &&
#endif
            r.detail == 2, "reject incomplete native record");
    else if (r.status == catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED) report->result("native output record", r, 0x03);
    else report->check(false, "reject incomplete native record");
    report->check(!written, "failed write extent");
}

int main(int argc, char** argv) {
    std::printf("CATOTLIB Tests\nTarget: %s/%s/%s\n", TARGET_VENDOR, TARGET_SYSTEM, TARGET_ARCHITECTURE);
    bool write = argc == 4 && std::strcmp(argv[1], "--write") == 0;
    if (argc != 1 && !write) { std::printf("Usage: test_catotlib [--write SCHEME INSTANCE_ID]\n"); return 1; }
    if (!write) std::printf("Read-only verification; --write SCHEME INSTANCE_ID explicitly enables same-value writes.\n");
    TestReport report;
    CATDVLIB_DEVICES* snapshot = test_snapshot(&report);
    if (!snapshot) return report.finish();
    catalyst::NUINT opened = 0, count = catdv_devices_get_count(snapshot);
    bool found = false;
    for (catalyst::NUINT j = 0; j < count; ++j) {
        const CATDVLIB_DEVICE* d = catdv_devices_get(snapshot, j);
        if (write) {
            CATDVLIB_IDENTIFIER id = {0, 0};
            if (catdv_device_get_identifier(d, &id).status != catalyst::STATUS_CODE_SUCCESS ||
                !valid_identifier(id) || std::strcmp((const char*) id.scheme, argv[2]) ||
                std::strcmp((const char*) id.value, argv[3])) continue;
            found = true;
        }
        catalyst::BOOL supported = catalyst::FALSE;
        catalyst::RESULT r = catdv_device_has_output(d, &supported);
        if (r.status != catalyst::STATUS_CODE_SUCCESS || !supported) continue;
        print_device(j, d);
        CATOTLIB_OUTPUT* output = 0;
        r = catot_device_open(d, &output);
        bool ok = report.result("open output", r, 0x01);
        report.check((r.status == catalyst::STATUS_CODE_SUCCESS) == (output != 0), "output ownership");
        if (!ok || !output) continue;
        ++opened;
        test_output(&report, output, write);
        r = catalyst::output::device_close(output);
        /* Darwin may need to finish the asynchronous request before close. */
#if TARGET_PLATFORM_DARWIN
        for (unsigned long attempt = 0; r.status == catalyst::STATUS_CODE_ERROR_INVALID_STATE && attempt < 100000; ++attempt)
            r = catalyst::output::device_close(output);
#endif
        report.result("close output", r, 0x02);
    }
    if (write) report.check(found, "selected instance exists");
    if (!opened) report.skip("no accessible output device");
    if (!write) report.skip("hardware writes not requested");
    report.result("free snapshot", catdv_devices_free(snapshot), 0x02);
    return report.finish();
}
