#include "catsylib.internal.hpp"

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
