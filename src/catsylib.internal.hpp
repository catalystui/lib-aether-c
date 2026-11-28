#ifndef CATSYLIB_INTERNAL_HPP
#define CATSYLIB_INTERNAL_HPP
#include "catsylib.h"
#include "catalyst.hpp"

enum CATSYLIB_FIELD {
    CATSYLIB_FIELD_NAME = 0x01,
    CATSYLIB_FIELD_VERSION = 0x02,
    CATSYLIB_FIELD_DISTRIBUTION = 0x03,
    CATSYLIB_FIELD_DISTRIBUTION_VERSION = 0x04,
    CATSYLIB_FIELD_ARCHITECTURE = 0x05,
    CATSYLIB_FIELD_HOSTNAME = 0x06
};

/* Public output preconditions hold; required initially contains zero. */
CATALYST_RESULT catsy_platform_get(CATSYLIB_FIELD field, CATALYST_UTF8W output,
    CATALYST_NUINT capacity, CATALYST_NUINT* required);

#endif /* CATSYLIB_INTERNAL_HPP */
