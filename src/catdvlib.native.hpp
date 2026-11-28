#ifndef CATDVLIB_NATIVE_HPP
#define CATDVLIB_NATIVE_HPP
#include "catdvlib.h"
#include "catalyst.hpp"
#include "catmmlib.h"
#include "catdvlib.display.hpp"


/* Snapshot storage for native Windows and Darwin enumeration. All strings
 * are immutable after publication. Overflow is recorded rather than truncated. */
struct CATDVLIB_NATIVE_TEXT {
    catalyst::STATUS_CODE status;
    CATALYST_UTF8_CODE_UNIT value[1024];
};
struct CATDVLIB_DEVICE {
    CATDVLIB_DEVICE_TYPE type;
    CATALYST_ASCII scheme;
    CATDVLIB_NATIVE_TEXT identifier, serial, manufacturer, product, descriptor, revision;
    CATALYST_ASCII hwid_scheme;
    CATDVLIB_NATIVE_TEXT hwids[16];
    catalyst::NUINT hwid_count;
    catalyst::STATUS_CODE hwid_status, capability_status;
    CATALYST_BOOL input, output;
    CATDVLIB_DISPLAY display;
};
struct CATDVLIB_DEVICES {
    CATDVLIB_DEVICE* data;
    catalyst::NUINT count, capacity;
};

/* Start a private snapshot entry. Caller publishes only after enumeration. */
static inline CATALYST_RESULT catdv_native_add(CATDVLIB_DEVICES* devices, CATDVLIB_DEVICE** entry) {
    *entry = 0;
    if (devices->count == devices->capacity) {
        catalyst::NUINT maximum = CATALYST_NUINT_MAX / sizeof(CATDVLIB_DEVICE);
        if (devices->capacity == maximum) return catalyst::RESULT(CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x90, 1, 0);
        catalyst::NUINT next = devices->capacity == 0 ? 1 :
            (devices->capacity > maximum / 2 ? maximum : devices->capacity * 2);
        CATMMLIB_POINTERW memory = devices->data;
        catalyst::RESULT r = catmm_realloc(&memory, next * sizeof(CATDVLIB_DEVICE));
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x90, 2, r.detail);
        devices->data = (CATDVLIB_DEVICE*) memory;
        devices->capacity = next;
    }
    CATDVLIB_DEVICE* d = &devices->data[devices->count++];
    catmm_fill(d, 0, sizeof(*d));
    d->identifier.status = d->serial.status = d->manufacturer.status = d->product.status =
        d->descriptor.status = d->revision.status = CATALYST_STATUS_CODE_ERROR_NOT_FOUND;
    d->capability_status = CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED;
    d->hwid_status = CATALYST_STATUS_CODE_SUCCESS;
    *entry = d;
    return catalyst::RESULT(CATALYST_STATUS_CODE_SUCCESS, 0x90, 3, 0);
}
#endif
