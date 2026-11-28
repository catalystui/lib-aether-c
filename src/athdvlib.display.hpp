#ifndef ATHDVLIB_DISPLAY_HPP
#define ATHDVLIB_DISPLAY_HPP
#include "athdvlib.h"
#include "catalyst.hpp"
#include "catmmlib.h"

/* Optional display metadata. Owned bytes are present only on display entries. */
struct ATHDVLIB_DISPLAY {
    CATALYST_BOOL present;
    CATALYST_STATUS_CODE edid_status, handle_status;
    CATALYST_NUINT handle, size;
    CATALYST_BYTE* bytes;
};

static inline void athdv_display_init(ATHDVLIB_DISPLAY* display) {
    display->present = CATALYST_TRUE;
    display->edid_status = CATALYST_STATUS_CODE_ERROR_NOT_FOUND;
    display->handle_status = CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED;
}

/* Acquire an immutable copy of the system-provided bytes without parsing them. */
static inline CATALYST_STATUS_CODE athdv_display_copy_edid(ATHDVLIB_DISPLAY* display, const CATALYST_BYTE* bytes, CATALYST_NUINT size) {
    if (!size) return CATALYST_STATUS_CODE_ERROR_NOT_FOUND;
    CATMMLIB_POINTERW memory = 0;
    catalyst::RESULT r = catmm_alloc(&memory, size);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return r.status;
    catmm_copy(memory, bytes, size);
    display->bytes = (CATALYST_BYTE*) memory;
    display->size = size;
    return CATALYST_STATUS_CODE_SUCCESS;
}

static inline CATALYST_RESULT athdv_display_release(ATHDVLIB_DISPLAY* display) {
    if (display->bytes) {
        catalyst::RESULT r = catmm_free(display->bytes);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x91, 0x01, r.detail);
        display->bytes = 0;
        display->size = 0;
    }
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x91, 0x02, 0);
}
#endif
