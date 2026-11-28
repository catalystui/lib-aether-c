#ifndef ATHIDLIB_H
#define ATHIDLIB_H
#include "catalyst.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Export API */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(ATHIDLIB_EXPORTS)
        #define ATHIDLIB_API __declspec(dllexport)
    #elif defined(ATHIDLIB_USE_DLL)
        #define ATHIDLIB_API __declspec(dllimport)
    #else
        #define ATHIDLIB_API
    #endif
#else
    #define ATHIDLIB_API
#endif

/* -------------------------------------------------------------------------------------------------
 * C API :: SYSTEM IDENTIFICATION
 * ------------------------------------------------------------------------------------------------- */

/* All queries use caller-owned UTF-8 storage; capacity and required include
 * the terminating zero. Pass output = 0 and capacity = 0 to query the size.
 * Otherwise output must be writable for capacity units; required must be
 * writable and must not overlap output. Insufficient
 * capacity leaves output unchanged. Other failures set required to zero. */

/** Retrieve the operating system name. */
ATHIDLIB_API CATALYST_RESULT athid_system_get_name(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);

/** Retrieve the operating system version. */
ATHIDLIB_API CATALYST_RESULT athid_system_get_version(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);

/** Retrieve the operating system distribution name. */
ATHIDLIB_API CATALYST_RESULT athid_system_get_distribution(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);

/** Retrieve the operating system distribution version. */
ATHIDLIB_API CATALYST_RESULT athid_system_get_distribution_version(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);

/** Retrieve the native system architecture. */
ATHIDLIB_API CATALYST_RESULT athid_system_get_architecture(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);

/** Retrieve the system hostname. */
ATHIDLIB_API CATALYST_RESULT athid_system_get_hostname(CATALYST_UTF8W output, CATALYST_NUINT capacity, CATALYST_NUINT* required);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* ATHIDLIB_H */
