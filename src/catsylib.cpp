#include "catsylib.internal.hpp"
#include "catsylib.h"
#include "catalyst.hpp"
#include "catmmlib.h"
#include "cattelib.h"

using namespace catalyst;

/* -------------------------------------------------------------------------------------------------
 * Internal native text handling
 * ------------------------------------------------------------------------------------------------- */

CATALYST_RESULT catsy_prepare_output(CATSYLIB_FIELD field, NUINT length,
    UTF8W output, NUINT capacity, NUINT* required) {
    CONTEXT_CODE context = (CONTEXT_CODE) field;
#if TARGET_PLATFORM_WIN32
    const OPERATION_CODE sizeOperation = 0x41;
    const OPERATION_CODE copyOperation = 0x42;
    const OPERATION_CODE capacityOperation = 0x43;
#else
    const OPERATION_CODE sizeOperation = 0x01;
    const OPERATION_CODE copyOperation = 0x02;
    const OPERATION_CODE capacityOperation = 0x03;
#endif
    *required = length;
    if (output == 0) return RESULT(STATUS_CODE_SUCCESS, context, sizeOperation, 0);
    if (capacity < length) return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, capacityOperation, 0);
    return RESULT(STATUS_CODE_SUCCESS, context, copyOperation, 0);
}

CATALYST_RESULT catsy_copy_utf8(CATSYLIB_FIELD field, UTF8 value,
    UTF8W output, NUINT capacity, NUINT* required) {
    CONTEXT_CODE context = (CONTEXT_CODE) field;
#if TARGET_PLATFORM_WIN32
    const OPERATION_CODE emptyOperation = 0x44;
    const OPERATION_CODE extentOperation = 0x45;
    const OPERATION_CODE validationOperation = 0x49;
#else
    const OPERATION_CODE emptyOperation = 0x06;
    const OPERATION_CODE extentOperation = 0x07;
    const OPERATION_CODE validationOperation = 0x0A;
#endif
    if (*value == 0) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, context, emptyOperation, 0);
    NUINT length = 0;
    while (value[length] != 0) {
        NUINT consumed = 1;
        if (value[length] >= (BYTE) 0x80) {
            CODEPOINT codepoint;
            RESULT decoded = catte_utf8_decode(value + length, &codepoint, &consumed);
            if (!status_code_is_success(decoded.status)) {
                return RESULT(decoded.status, context, validationOperation, decoded.detail);
            }
        }
        if (consumed >= CATALYST_NUINT_MAX - length) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, extentOperation, 0);
        }
        length += consumed;
    }
    RESULT result = catsy_prepare_output(field, length + (NUINT) 1, output, capacity, required);
    if (output != 0 && status_code_is_success(result.status)) catmm_copy(output, value, *required);
    return result;
}

CATALYST_RESULT catsy_release(CATMMLIB_POINTERW memory, CATALYST_RESULT result,
    OPERATION_CODE operation, NUINT* required) {
    if (memory == 0) return result;
    RESULT released = catmm_free(memory);
    if (!status_code_is_success(released.status)) {
        BOOL shortBuffer = result.status == STATUS_CODE_ERROR_BUFFER_OVERFLOW &&
            (result.operation == 0x03 || result.operation == 0x43);
        if (status_code_is_success(result.status) || shortBuffer ||
            (released.status & CATALYST_STATUS_CODE_LEVEL_MASK) > (result.status & CATALYST_STATUS_CODE_LEVEL_MASK)) {
            *required = 0;
            return RESULT(released.status, result.context, operation, released.detail);
        }
    }
    return result;
}

/* -------------------------------------------------------------------------------------------------
 * C API :: SYSTEM IDENTIFICATION
 * ------------------------------------------------------------------------------------------------- */

static CATALYST_RESULT catsy_system_get(CATSYLIB_FIELD field, CATALYST_UTF8W output,
    CATALYST_NUINT capacity, CATALYST_NUINT* required) {
    *required = 0;
    return catsy_platform_get(field, output, capacity, required);
}

CATALYST_RESULT catsy_system_get_name(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required) {
    return catsy_system_get(CATSYLIB_FIELD_NAME, output, capacity, required);
}

CATALYST_RESULT catsy_system_get_version(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required) {
    return catsy_system_get(CATSYLIB_FIELD_VERSION, output, capacity, required);
}

CATALYST_RESULT catsy_system_get_distribution(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required) {
    return catsy_system_get(CATSYLIB_FIELD_DISTRIBUTION, output, capacity, required);
}

CATALYST_RESULT catsy_system_get_distribution_version(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required) {
    return catsy_system_get(CATSYLIB_FIELD_DISTRIBUTION_VERSION, output, capacity, required);
}

CATALYST_RESULT catsy_system_get_architecture(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required) {
    return catsy_system_get(CATSYLIB_FIELD_ARCHITECTURE, output, capacity, required);
}

CATALYST_RESULT catsy_system_get_hostname(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required) {
    return catsy_system_get(CATSYLIB_FIELD_HOSTNAME, output, capacity, required);
}
