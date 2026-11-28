#ifndef CATSYLIB_INTERNAL_HPP
#define CATSYLIB_INTERNAL_HPP
#include "catalyst.h"
#include "catmmlib.h"

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

/* Readable terminated native byte string, separate from destination; validate UTF-8. */
CATALYST_RESULT catsy_copy_utf8(CATSYLIB_FIELD field, CATALYST_UTF8 value,
    CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);

/* Return the size/capacity outcome for an already validated nonempty value.
 * Length includes the terminator. A successful non-null query still needs a copy. */
CATALYST_RESULT catsy_prepare_output(CATSYLIB_FIELD field, CATALYST_NUINT length,
    CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);

/* Release temporary Core storage; retain an earlier fatal or more severe failure.
 * A cleanup failure supersedes success or insufficient destination capacity. */
CATALYST_RESULT catsy_release(CATMMLIB_POINTERW memory, CATALYST_RESULT result,
    CATALYST_OPERATION_CODE operation, CATALYST_NUINT* required);

#endif /* CATSYLIB_INTERNAL_HPP */
