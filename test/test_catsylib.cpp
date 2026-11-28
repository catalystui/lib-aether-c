#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "catsylib.hpp"

using namespace catalyst;

typedef CATALYST_RESULT (*Query)(UTF8W, NUINT, NUINT*);
typedef RESULT (*QueryCpp)(UTF8W, NUINT, NUINT*);

struct TestCase {
    const char* name;
    CONTEXT_CODE context;
    Query query;
    QueryCpp cpp;
};

static const TestCase cases[] = {
    { "catsy_system_get_name", 0x01, catsy_system_get_name, system_get_name },
    { "catsy_system_get_version", 0x02, catsy_system_get_version, system_get_version },
    { "catsy_system_get_distribution", 0x03, catsy_system_get_distribution, system_get_distribution },
    { "catsy_system_get_distribution_version", 0x04, catsy_system_get_distribution_version, system_get_distribution_version },
    { "catsy_system_get_architecture", 0x05, catsy_system_get_architecture, system_get_architecture },
    { "catsy_system_get_hostname", 0x06, catsy_system_get_hostname, system_get_hostname }
};

#if TARGET_PLATFORM_WIN32
static const OPERATION_CODE sizeOperation = 0x41;
static const OPERATION_CODE copyOperation = 0x42;
static const OPERATION_CODE overflowOperation = 0x43;
#else
static const OPERATION_CODE sizeOperation = 0x01;
static const OPERATION_CODE copyOperation = 0x02;
static const OPERATION_CODE overflowOperation = 0x03;
#endif

static BOOL matches(RESULT result, STATUS_CODE status, CONTEXT_CODE context, OPERATION_CODE operation) {
    return result.status == status && result.context == context &&
        result.operation == operation && result.detail == 0 ? TRUE : FALSE;
}

static int test_query(const TestCase& test, int* unavailable) {
    NUINT required = 0;
    RESULT result = test.query(0, 0, &required);
    if (!status_code_is_success(result.status)) {
        BOOL expected = result.status == STATUS_CODE_ERROR_NOT_FOUND ||
            result.status == STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED ||
            result.status == STATUS_CODE_ERROR_ACCESS_DENIED;
        if (expected && result.context == test.context && result.operation != 0 &&
            result.detail == 0 && required == 0) {
            std::printf("SKIP: %s unavailable (status %02X, operation %02X)\n",
                test.name, (unsigned int) result.status, (unsigned int) result.operation);
            ++*unavailable;
            return 0;
        }
        std::printf("FAIL: %s size query (%02X %02X %02X %02X)\n", test.name,
            (unsigned int) result.status, (unsigned int) result.context,
            (unsigned int) result.operation, (unsigned int) result.detail);
        return 1;
    }
    if (!matches(result, STATUS_CODE_SUCCESS, test.context, sizeOperation) || required < 2) {
        std::printf("FAIL: %s size query contract\n", test.name);
        return 1;
    }
    if (required == CATALYST_NUINT_MAX) {
        std::printf("SKIP: %s cannot fit a guard byte\n", test.name);
        ++*unavailable;
        return 0;
    }
    NUINT extent = (NUINT) (required + (NUINT) 1);
    UTF8W output = (UTF8W) std::malloc((size_t) extent);
    UTF8W cppOutput = (UTF8W) std::malloc((size_t) extent);
    if (output == 0 || cppOutput == 0) {
        std::free(output);
        std::free(cppOutput);
        std::printf("FAIL: %s fixture allocation\n", test.name);
        return 1;
    }
    int failed = 0;
    NUINT actual = 0;
    std::memset(output, 0xA5, (size_t) extent);
    result = test.query(output, required, &actual);
    if (!matches(result, STATUS_CODE_SUCCESS, test.context, copyOperation) ||
        actual != required || output[required - (NUINT) 1] != 0 ||
        output[required] != 0xA5 || std::strlen((const char*) output) != (size_t) (required - (NUINT) 1)) {
        std::printf("FAIL: %s exact capacity / terminator / guard\n", test.name);
        ++failed;
    } else {
        std::printf("PASS: %s = %s\n", test.name, (const char*) output);
    }

    NUINT cppRequired = 0;
    std::memset(cppOutput, 0xA5, (size_t) extent);
    RESULT cppResult = test.cpp(0, 0, &cppRequired);
    if (!matches(cppResult, STATUS_CODE_SUCCESS, test.context, sizeOperation) || cppRequired != required) {
        std::printf("FAIL: %s C++ size query\n", test.name);
        ++failed;
    }
    cppResult = test.cpp(cppOutput, extent, &cppRequired);
    if (!matches(cppResult, STATUS_CODE_SUCCESS, test.context, copyOperation) ||
        cppRequired != required || cppOutput[required - (NUINT) 1] != 0 ||
        cppOutput[required] != 0xA5 || failed != 0 ||
        std::memcmp(output, cppOutput, (size_t) required) != 0) {
        std::printf("FAIL: %s C++ output / oversized buffer\n", test.name);
        ++failed;
    }

    // Zero capacity and a short writable extent are both valid invocations.
    for (BYTE trial = 0; trial < 2; ++trial) {
        NUINT capacity = trial == 0 ? (NUINT) 0 : (NUINT) (required - (NUINT) 1);
        std::memset(output, 0xA5, (size_t) extent);
        actual = 0;
        result = test.query(output, capacity, &actual);
        BOOL passed = matches(result, STATUS_CODE_ERROR_BUFFER_OVERFLOW, test.context, overflowOperation) &&
            actual == required ? TRUE : FALSE;
        for (NUINT i = 0; i < extent; ++i) {
            if (output[i] != 0xA5) passed = FALSE;
        }
        if (!passed) {
            std::printf("FAIL: %s short-buffer preservation (%lu units)\n", test.name, (unsigned long) capacity);
            ++failed;
        } else {
            std::printf("PASS: %s short-buffer preservation (%lu units)\n", test.name, (unsigned long) capacity);
        }
    }
    std::free(output);
    std::free(cppOutput);
    return failed;
}

int main() {
    std::printf("CATSYLIB Tests\n");
    std::printf("Target: %s/%s/%s\n", TARGET_VENDOR, TARGET_SYSTEM, TARGET_ARCHITECTURE);
    int failed = 0;
    int unavailable = 0;
    for (BYTE i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        failed += test_query(cases[i], &unavailable);
    }
    std::printf("\nFailures: %d; unavailable queries: %d\n", failed, unavailable);
    if (failed != 0) return 1;
    std::printf("All available CATSYLIB contracts verified.\n");
    return unavailable != 0 ? 2 : 0;
}
