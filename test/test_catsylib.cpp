#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "catsylib.hpp"


typedef CATALYST_RESULT (*Query)(catalyst::UTF8W, catalyst::NUINT, catalyst::NUINT*);
typedef catalyst::RESULT (*QueryCpp)(catalyst::UTF8W, catalyst::NUINT, catalyst::NUINT*);

struct TestCase {
    const char* name;
    catalyst::CONTEXT_CODE context;
    Query query;
    QueryCpp cpp;
};

static const TestCase cases[] = {
    { "catsy_system_get_name", 0x01, catsy_system_get_name, catalyst::system_get_name },
    { "catsy_system_get_version", 0x02, catsy_system_get_version, catalyst::system_get_version },
    { "catsy_system_get_distribution", 0x03, catsy_system_get_distribution, catalyst::system_get_distribution },
    { "catsy_system_get_distribution_version", 0x04, catsy_system_get_distribution_version, catalyst::system_get_distribution_version },
    { "catsy_system_get_architecture", 0x05, catsy_system_get_architecture, catalyst::system_get_architecture },
    { "catsy_system_get_hostname", 0x06, catsy_system_get_hostname, catalyst::system_get_hostname }
};

#if TARGET_PLATFORM_WIN32
static const catalyst::OPERATION_CODE sizeOperation = 0x41;
static const catalyst::OPERATION_CODE copyOperation = 0x42;
static const catalyst::OPERATION_CODE overflowOperation = 0x43;
#else
static const catalyst::OPERATION_CODE sizeOperation = 0x01;
static const catalyst::OPERATION_CODE copyOperation = 0x02;
static const catalyst::OPERATION_CODE overflowOperation = 0x03;
#endif

static catalyst::BOOL matches(catalyst::RESULT result, catalyst::STATUS_CODE status, catalyst::CONTEXT_CODE context, catalyst::OPERATION_CODE operation) {
    return result.status == status && result.context == context &&
        result.operation == operation && result.detail == 0 ? catalyst::TRUE : catalyst::FALSE;
}

static int test_query(const TestCase& test, int* unavailable) {
    catalyst::NUINT required = 0;
    catalyst::RESULT result = test.query(0, 0, &required);
    if (!catalyst::status_code_is_success(result.status)) {
        catalyst::BOOL expected = result.status == catalyst::STATUS_CODE_ERROR_NOT_FOUND ||
            result.status == catalyst::STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED ||
            result.status == catalyst::STATUS_CODE_ERROR_ACCESS_DENIED;
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
    if (!matches(result, catalyst::STATUS_CODE_SUCCESS, test.context, sizeOperation) || required < 2) {
        std::printf("FAIL: %s size query contract\n", test.name);
        return 1;
    }
    if (required == CATALYST_NUINT_MAX) {
        std::printf("SKIP: %s cannot fit a guard byte\n", test.name);
        ++*unavailable;
        return 0;
    }
    catalyst::NUINT extent = (catalyst::NUINT) (required + (catalyst::NUINT) 1);
    catalyst::UTF8W output = (catalyst::UTF8W) std::malloc((size_t) extent);
    catalyst::UTF8W cppOutput = (catalyst::UTF8W) std::malloc((size_t) extent);
    if (output == 0 || cppOutput == 0) {
        std::free(output);
        std::free(cppOutput);
        std::printf("FAIL: %s fixture allocation\n", test.name);
        return 1;
    }
    int failed = 0;
    catalyst::NUINT actual = 0;
    std::memset(output, 0xA5, (size_t) extent);
    result = test.query(output, required, &actual);
    if (!matches(result, catalyst::STATUS_CODE_SUCCESS, test.context, copyOperation) ||
        actual != required || output[required - (catalyst::NUINT) 1] != 0 ||
        output[required] != 0xA5 || std::strlen((const char*) output) != (size_t) (required - (catalyst::NUINT) 1)) {
        std::printf("FAIL: %s exact capacity / terminator / guard\n", test.name);
        ++failed;
    } else {
        std::printf("PASS: %s = %s\n", test.name, (const char*) output);
    }

    catalyst::NUINT cppRequired = 0;
    std::memset(cppOutput, 0xA5, (size_t) extent);
    catalyst::RESULT cppResult = test.cpp(0, 0, &cppRequired);
    if (!matches(cppResult, catalyst::STATUS_CODE_SUCCESS, test.context, sizeOperation) || cppRequired != required) {
        std::printf("FAIL: %s C++ size query\n", test.name);
        ++failed;
    }
    cppResult = test.cpp(cppOutput, extent, &cppRequired);
    if (!matches(cppResult, catalyst::STATUS_CODE_SUCCESS, test.context, copyOperation) ||
        cppRequired != required || cppOutput[required - (catalyst::NUINT) 1] != 0 ||
        cppOutput[required] != 0xA5 || failed != 0 ||
        std::memcmp(output, cppOutput, (size_t) required) != 0) {
        std::printf("FAIL: %s C++ output / oversized buffer\n", test.name);
        ++failed;
    }

    // Zero capacity and a short writable extent are both valid invocations.
    for (catalyst::BYTE trial = 0; trial < 2; ++trial) {
        catalyst::NUINT capacity = trial == 0 ? (catalyst::NUINT) 0 : (catalyst::NUINT) (required - (catalyst::NUINT) 1);
        std::memset(output, 0xA5, (size_t) extent);
        actual = 0;
        result = test.query(output, capacity, &actual);
        catalyst::BOOL passed = matches(result, catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW, test.context, overflowOperation) &&
            actual == required ? catalyst::TRUE : catalyst::FALSE;
        for (catalyst::NUINT i = 0; i < extent; ++i) {
            if (output[i] != 0xA5) passed = catalyst::FALSE;
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
    for (catalyst::BYTE i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        failed += test_query(cases[i], &unavailable);
    }
    std::printf("\nFailures: %d; unavailable queries: %d\n", failed, unavailable);
    if (failed != 0) return 1;
    std::printf("All available CATSYLIB contracts verified.\n");
    return unavailable != 0 ? 2 : 0;
}
