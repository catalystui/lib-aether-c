#ifndef CATSYLIB_HPP
#define CATSYLIB_HPP
#include "catalyst.hpp"
#include "catsylib.h"

namespace catalyst {

/** Retrieve the operating system name. */
inline RESULT system_get_name(UTF8W output, NUINT capacity, NUINT* required) {
    return ::catsy_system_get_name(output, capacity, required);
}

/** Retrieve the operating system version. */
inline RESULT system_get_version(UTF8W output, NUINT capacity, NUINT* required) {
    return ::catsy_system_get_version(output, capacity, required);
}

/** Retrieve the operating system distribution name. */
inline RESULT system_get_distribution(UTF8W output, NUINT capacity, NUINT* required) {
    return ::catsy_system_get_distribution(output, capacity, required);
}

/** Retrieve the operating system distribution version. */
inline RESULT system_get_distribution_version(UTF8W output, NUINT capacity, NUINT* required) {
    return ::catsy_system_get_distribution_version(output, capacity, required);
}

/** Retrieve the native system architecture. */
inline RESULT system_get_architecture(UTF8W output, NUINT capacity, NUINT* required) {
    return ::catsy_system_get_architecture(output, capacity, required);
}

/** Retrieve the system hostname. */
inline RESULT system_get_hostname(UTF8W output, NUINT capacity, NUINT* required) {
    return ::catsy_system_get_hostname(output, capacity, required);
}

} /* namespace catalyst */
#endif /* CATSYLIB_HPP */
