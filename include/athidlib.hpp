#ifndef ATHIDLIB_HPP
#define ATHIDLIB_HPP
#include "catalyst.hpp"
#include "athidlib.h"

namespace catalyst {

/* -------------------------------------------------------------------------------------------------
 * C++ API :: SYSTEM IDENTIFICATION
 * ------------------------------------------------------------------------------------------------- */

inline RESULT system_get_name(UTF8W output, NUINT capacity, NUINT* required) {
    return ::athid_system_get_name(output, capacity, required);
}

inline RESULT system_get_version(UTF8W output, NUINT capacity, NUINT* required) {
    return ::athid_system_get_version(output, capacity, required);
}

inline RESULT system_get_distribution(UTF8W output, NUINT capacity, NUINT* required) {
    return ::athid_system_get_distribution(output, capacity, required);
}

inline RESULT system_get_distribution_version(UTF8W output, NUINT capacity, NUINT* required) {
    return ::athid_system_get_distribution_version(output, capacity, required);
}

inline RESULT system_get_architecture(UTF8W output, NUINT capacity, NUINT* required) {
    return ::athid_system_get_architecture(output, capacity, required);
}

inline RESULT system_get_hostname(UTF8W output, NUINT capacity, NUINT* required) {
    return ::athid_system_get_hostname(output, capacity, required);
}

} /* namespace catalyst */
#endif /* ATHIDLIB_HPP */
