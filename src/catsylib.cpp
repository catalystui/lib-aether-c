#include "catsylib.internal.hpp"
#include "catsylib.h"
#include "catalyst.hpp"
#include "catmmlib.h"
#include "cattelib.h"


/* -------------------------------------------------------------------------------------------------
 * Internal native text handling
 * ------------------------------------------------------------------------------------------------- */

CATALYST_RESULT catsy_prepare_output(CATSYLIB_FIELD field, catalyst::NUINT length,
    catalyst::UTF8W output, catalyst::NUINT capacity, catalyst::NUINT* required) {
    catalyst::CONTEXT_CODE context = (catalyst::CONTEXT_CODE) field;
#if TARGET_PLATFORM_WIN32
    const catalyst::OPERATION_CODE sizeOperation = 0x41;
    const catalyst::OPERATION_CODE copyOperation = 0x42;
    const catalyst::OPERATION_CODE capacityOperation = 0x43;
#else
    const catalyst::OPERATION_CODE sizeOperation = 0x01;
    const catalyst::OPERATION_CODE copyOperation = 0x02;
    const catalyst::OPERATION_CODE capacityOperation = 0x03;
#endif
    *required = length;
    if (output == 0) return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, context, sizeOperation, 0);
    if (capacity < length) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, capacityOperation, 0);
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, context, copyOperation, 0);
}

CATALYST_RESULT catsy_copy_utf8(CATSYLIB_FIELD field, catalyst::UTF8 value,
    catalyst::UTF8W output, catalyst::NUINT capacity, catalyst::NUINT* required) {
    catalyst::CONTEXT_CODE context = (catalyst::CONTEXT_CODE) field;
#if TARGET_PLATFORM_WIN32
    const catalyst::OPERATION_CODE emptyOperation = 0x44;
    const catalyst::OPERATION_CODE extentOperation = 0x45;
    const catalyst::OPERATION_CODE validationOperation = 0x49;
#else
    const catalyst::OPERATION_CODE emptyOperation = 0x06;
    const catalyst::OPERATION_CODE extentOperation = 0x07;
    const catalyst::OPERATION_CODE validationOperation = 0x0A;
#endif
    if (*value == 0) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_FOUND, context, emptyOperation, 0);
    catalyst::NUINT length = 0;
    while (value[length] != 0) {
        catalyst::NUINT consumed = 1;
        if (value[length] >= (catalyst::BYTE) 0x80) {
            catalyst::CODEPOINT codepoint;
            catalyst::RESULT decoded = catte_utf8_decode(value + length, &codepoint, &consumed);
            if (!catalyst::status_code_is_success(decoded.status)) {
                return catalyst::RESULT(decoded.status, context, validationOperation, decoded.detail);
            }
        }
        if (consumed >= CATALYST_NUINT_MAX - length) {
            return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, extentOperation, 0);
        }
        length += consumed;
    }
    catalyst::RESULT result = catsy_prepare_output(field, length + (catalyst::NUINT) 1, output, capacity, required);
    if (output != 0 && catalyst::status_code_is_success(result.status)) catmm_copy(output, value, *required);
    return result;
}

CATALYST_RESULT catsy_release(CATMMLIB_POINTERW memory, CATALYST_RESULT result,
    catalyst::OPERATION_CODE operation, catalyst::NUINT* required) {
    if (memory == 0) return result;
    catalyst::RESULT released = catmm_free(memory);
    if (!catalyst::status_code_is_success(released.status)) {
        catalyst::BOOL shortBuffer = result.status == catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW &&
            (result.operation == 0x03 || result.operation == 0x43);
        if (catalyst::status_code_is_success(result.status) || shortBuffer ||
            (released.status & CATALYST_STATUS_CODE_LEVEL_MASK) > (result.status & CATALYST_STATUS_CODE_LEVEL_MASK)) {
            *required = 0;
            return catalyst::RESULT(released.status, result.context, operation, released.detail);
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
