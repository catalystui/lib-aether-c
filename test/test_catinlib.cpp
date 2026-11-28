#include "device_io_test.hpp"
#include "catinlib.hpp"

static void test_input(TestReport* report, CATINLIB_INPUT* input) {
    NUINT count = 0, cppCount = 0;
    RESULT r = catin_device_get_channel_count(input, &count);
    RESULT c = catalyst::input::device_get_channel_count(input, &cppCount);
    report->check(same_result(r, c) && count == cppCount, "channel count parity");
    if (report->result("channel count", r, 0x04)) {
        if (!count) report->skip("no typed channels");
        for (NUINT j = 0; j < count; ++j) {
            CATINLIB_CHANNEL_TYPE type = CATINLIB_CHANNEL_UNKNOWN, cppType = CATINLIB_CHANNEL_UNKNOWN;
            r = catin_channel_get_type(input, j, &type); c = catalyst::input::channel_get_type(input, j, &cppType);
            if (!report->result("channel type", r, 0x05)) continue;
            report->check(type <= CATINLIB_CHANNEL_RELATIVE && type == cppType && same_result(r, c), "channel type parity");
            CATDVLIB_IDENTIFIER id = {0, 0}, cppId = {0, 0};
            r = catin_channel_get_identifier(input, j, &id); c = catalyst::input::channel_get_identifier(input, j, &cppId);
            if (report->result("channel identifier", r, 0x06))
                report->check(same_identifier(id, cppId) && same_result(r, c), "channel identifier parity");
            else report->check(!id.scheme && !id.value, "channel identifier failure outputs");
            if (type == CATINLIB_CHANNEL_DIGITAL) {
                BOOL state = FALSE;
                r = catalyst::input::digital_get_state(input, j, &state);
                if (report->result("digital state", r, 0x07))
                    report->check(state == TRUE || state == FALSE, "digital state representation");
                else report->check(!state, "digital state failure output");
                CATINLIB_DIGITAL_ACTION action = CATINLIB_DIGITAL_RELEASE;
                r = catin_digital_read(input, j, &action);
                if (report->result("digital occurrence", r, 0x08, true))
                    report->check(r.status == STATUS_CODE_SUCCESS_NOOP ? action == CATINLIB_DIGITAL_RELEASE :
                        action <= CATINLIB_DIGITAL_REPEAT, "digital occurrence/no-op representation");
            }
#if CATALYST_SUPPORTS_32BIT
            if (type == CATINLIB_CHANNEL_ABSOLUTE) {
                INT low = 0, high = 0, value = 0, cppLow = 0, cppHigh = 0;
                r = catin_absolute_get_range(input, j, &low, &high);
                c = catalyst::input::absolute_get_range(input, j, &cppLow, &cppHigh);
                if (report->result("absolute range", r, 0x09))
                    report->check(low <= high && low == cppLow && high == cppHigh && same_result(r, c), "absolute range parity");
                else report->check(!low && !high, "range failure outputs");
                r = catalyst::input::absolute_get_value(input, j, &value);
                if (!report->result("absolute value", r, 0x0A)) report->check(value == 0, "value failure output");
            } else if (type == CATINLIB_CHANNEL_RELATIVE) {
                INT offset = 0;
                r = catalyst::input::relative_read(input, j, &offset);
                if (report->result("relative occurrence", r, 0x0B, true) && r.status == STATUS_CODE_SUCCESS_NOOP)
                    report->check(offset == 0, "relative no-op output");
            }
#endif
        }
    } else report->check(!count, "channel count failure output");
    BYTE buffer[4097];
    std::memset(buffer, 0xA5, sizeof(buffer));
    NUINT received = 0;
    r = catin_device_read(input, buffer, 0, &received);
    if (r.status == STATUS_CODE_ERROR_BUFFER_OVERFLOW)
        report->check(r.context == 0x03 && r.operation == 0x01 && !r.detail && !received, "zero-capacity read");
    else report->result("zero-capacity read/no event", r, 0x03, true);
    bool untouched = true;
    for (size_t j = 0; j < sizeof(buffer); ++j) if (buffer[j] != 0xA5) untouched = false;
    report->check(untouched && !received, "zero-capacity buffer preservation");
    if (sizeof(buffer) <= CATALYST_NUINT_MAX) {
        for (BYTE j = 0; j < 8; ++j) {
            std::memset(buffer, 0xA5, sizeof(buffer)); received = 0;
            r = catalyst::input::device_read(input, buffer, sizeof(buffer) - 1, &received);
            if (report->result("native event polling", r, 0x03, true))
                report->check(r.status == STATUS_CODE_SUCCESS_NOOP ? received == 0 : received > 0 && received < sizeof(buffer), "read extent");
            else report->check(!received, "read failure extent");
            bool guard = received < sizeof(buffer);
            if (guard) for (size_t k = received; k < sizeof(buffer); ++k) if (buffer[k] != 0xA5) guard = false;
            report->check(guard, "read guard / no-op preservation");
        }
    } else report->skip("native buffer extent not representable");
}

int main() {
    std::printf("CATINLIB Tests\nTarget: %s/%s/%s\n", TARGET_VENDOR, TARGET_SYSTEM, TARGET_ARCHITECTURE);
    TestReport report;
    CATDVLIB_DEVICES* snapshot = test_snapshot(&report);
    if (!snapshot) return report.finish();
    NUINT opened = 0;
    for (NUINT j = 0; j < catdv_devices_get_count(snapshot); ++j) {
        const CATDVLIB_DEVICE* d = catdv_devices_get(snapshot, j);
        BOOL supported = FALSE;
        RESULT r = catdv_device_has_input(d, &supported);
        if (r.status != STATUS_CODE_SUCCESS || !supported) continue;
        print_device(j, d);
        CATINLIB_INPUT* input = 0;
        r = catin_device_open(d, &input);
        bool ok = report.result("open input", r, 0x01);
        report.check((r.status == STATUS_CODE_SUCCESS) == (input != 0), "input ownership");
        if (!ok || !input) continue;
        ++opened;
        test_input(&report, input);
        report.result("close input", catalyst::input::device_close(input), 0x02);
    }
    if (!opened) report.skip("no accessible input device");
    report.result("free snapshot", catdv_devices_free(snapshot), 0x02);
    return report.finish();
}
