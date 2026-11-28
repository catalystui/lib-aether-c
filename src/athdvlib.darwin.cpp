#include "athdvlib.native.hpp"
#include "athdvlib.darwin.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#undef TRUE
#undef FALSE

static void athdv_mac_text(IOHIDDeviceRef hid, CFStringRef key, ATHDVLIB_NATIVE_TEXT* text) {
    CFTypeRef value = IOHIDDeviceGetProperty(hid, key);
    if (!value) return;
    if (CFGetTypeID(value) != CFStringGetTypeID()) { text->status = catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT; return; }
    if (!CFStringGetCString((CFStringRef) value, (char*) text->value, sizeof(text->value), kCFStringEncodingUTF8)) {
        text->status = catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW; return;
    }
    catalyst::NUINT length;
    catalyst::RESULT r = catte_utf8_length(text->value, &length);
    text->status = catalyst::status_code_is_success(r.status) && length == 0 ? catalyst::STATUS_CODE_ERROR_NOT_FOUND : r.status;
}
static catalyst::STATUS_CODE athdv_mac_number(IOHIDDeviceRef hid, CFStringRef key, CATALYST_ULONG* number) {
    CFTypeRef value = IOHIDDeviceGetProperty(hid, key);
    if (!value) return catalyst::STATUS_CODE_ERROR_NOT_FOUND;
    SInt64 n;
    if (CFGetTypeID(value) != CFNumberGetTypeID() || !CFNumberGetValue((CFNumberRef) value, kCFNumberSInt64Type, &n) || n < 0)
        return catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT;
    *number = (CATALYST_ULONG) n; return catalyst::STATUS_CODE_SUCCESS;
}
static catalyst::STATUS_CODE athdv_mac_entry(ATHDVLIB_DEVICES* devices, IOHIDDeviceRef hid) {
    uint64_t native_id;
    io_service_t service = IOHIDDeviceGetService(hid);
    if (!service) return catalyst::STATUS_CODE_SUCCESS;
    kern_return_t err = IORegistryEntryGetRegistryEntryID(service, &native_id);
    if (err != KERN_SUCCESS) return athdv_mac_status(err);
    ATHDVLIB_DEVICE* d;
    catalyst::RESULT r = athdv_native_add(devices, &d);
    if (!catalyst::status_code_is_success(r.status)) return r.status;
    d->scheme = (CATALYST_ASCII) "darwin-hid"; d->type = ATHDVLIB_DEVICE_LOGICAL;
    athdv_mac_hex(d->identifier.value, (CATALYST_ULONG) native_id, 16);
    d->identifier.status = catalyst::STATUS_CODE_SUCCESS;
    athdv_mac_text(hid, CFSTR(kIOHIDManufacturerKey), &d->manufacturer);
    athdv_mac_text(hid, CFSTR(kIOHIDProductKey), &d->product);
    athdv_mac_text(hid, CFSTR(kIOHIDSerialNumberKey), &d->serial);
    d->descriptor = d->product;
    CATALYST_ULONG vendor, product, revision;
    catalyst::STATUS_CODE vs = athdv_mac_number(hid, CFSTR(kIOHIDVendorIDKey), &vendor);
    catalyst::STATUS_CODE ps = athdv_mac_number(hid, CFSTR(kIOHIDProductIDKey), &product);
    if (vs == catalyst::STATUS_CODE_SUCCESS && ps == catalyst::STATUS_CODE_SUCCESS) {
        if (vendor > 65535 || product > 65535) d->hwid_status = catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT;
        else {
            CFTypeRef transport = IOHIDDeviceGetProperty(hid, CFSTR(kIOHIDTransportKey));
            d->hwid_scheme = (CATALYST_ASCII) "darwin-hid";
            if (transport && CFGetTypeID(transport) == CFStringGetTypeID() &&
                CFStringCompare((CFStringRef) transport, CFSTR("USB"), 0) == kCFCompareEqualTo)
                d->hwid_scheme = (CATALYST_ASCII) "usb";
            athdv_mac_hex(d->hwids[0].value, vendor, 4);
            d->hwids[0].value[4] = ':';
            athdv_mac_hex(d->hwids[0].value + 5, product, 4);
            d->hwids[0].status = catalyst::STATUS_CODE_SUCCESS; d->hwid_count = 1;
        }
    } else if (vs != catalyst::STATUS_CODE_SUCCESS && vs != catalyst::STATUS_CODE_ERROR_NOT_FOUND) d->hwid_status = vs;
    else if (ps != catalyst::STATUS_CODE_SUCCESS && ps != catalyst::STATUS_CODE_ERROR_NOT_FOUND) d->hwid_status = ps;
    d->revision.status = athdv_mac_number(hid, CFSTR(kIOHIDVersionNumberKey), &revision);
    if (d->revision.status == catalyst::STATUS_CODE_SUCCESS) athdv_mac_hex(d->revision.value, revision, 16);
    CFArrayRef elements = IOHIDDeviceCopyMatchingElements(hid, 0, kIOHIDOptionsTypeNone);
    if (elements) {
        d->capability_status = catalyst::STATUS_CODE_SUCCESS;
        for (CFIndex j = 0; j < CFArrayGetCount(elements); ++j) {
            IOHIDElementRef element = (IOHIDElementRef) CFArrayGetValueAtIndex(elements, j);
            if (athdv_mac_direction(element, catalyst::TRUE)) d->input = catalyst::TRUE;
            if (athdv_mac_direction(element, catalyst::FALSE)) d->output = catalyst::TRUE;
            if (d->input && d->output) break;
        }
        CFRelease(elements);
    } else d->capability_status = catalyst::STATUS_CODE_ERROR_NOT_FOUND;
    return catalyst::STATUS_CODE_SUCCESS;
}
/* Use public CoreGraphics identities and public IOKit display properties.
 * A vendor/product match alone must not select one of several identical panels. */
static bool athdv_mac_display_number(CFDictionaryRef info, CFStringRef key, uint32_t* value) {
    CFTypeRef field = CFDictionaryGetValue(info, key);
    SInt64 n = 0;
    if (!field || CFGetTypeID(field) != CFNumberGetTypeID() ||
        !CFNumberGetValue((CFNumberRef) field, kCFNumberSInt64Type, &n) || n < 0 || n > CATALYST_UINT_MAX) return false;
    *value = (uint32_t) n;
    return true;
}

static void athdv_mac_display_edid(ATHDVLIB_DEVICE* d, CGDirectDisplayID id, const CGDirectDisplayID* ids, uint32_t count) {
    uint32_t vendor = CGDisplayVendorNumber(id), product = CGDisplayModelNumber(id), serial = CGDisplaySerialNumber(id);
    if (!vendor) return;
    for (uint32_t j = 0; j < count; ++j) {
        if (ids[j] != id && CGDisplayVendorNumber(ids[j]) == vendor && CGDisplayModelNumber(ids[j]) == product &&
            (!serial || !CGDisplaySerialNumber(ids[j]) || CGDisplaySerialNumber(ids[j]) == serial)) {
            d->display.edid_status = CATALYST_STATUS_CODE_ERROR_INVALID_STATE; return;
        }
    }
    CFMutableDictionaryRef matching = IOServiceMatching("IODisplayConnect");
    if (!matching) { d->display.edid_status = CATALYST_STATUS_CODE_ERROR_ALLOCATION_FAILED; return; }
    io_iterator_t iterator = 0;
    kern_return_t result = IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &iterator);
    if (result != KERN_SUCCESS) { d->display.edid_status = athdv_mac_status(result); return; }
    CFDictionaryRef match = 0;
    io_service_t service;
    bool ambiguous = false;
    while ((service = IOIteratorNext(iterator)) != 0) {
        CFDictionaryRef info = IODisplayCreateInfoDictionary(service, kIODisplayOnlyPreferredName);
        IOObjectRelease(service);
        if (!info) continue;
        uint32_t v = 0, p = 0, s = 0;
        bool equal = athdv_mac_display_number(info, CFSTR("DisplayVendorID"), &v) &&
            athdv_mac_display_number(info, CFSTR("DisplayProductID"), &p) && v == vendor && p == product &&
            (!serial || (athdv_mac_display_number(info, CFSTR("DisplaySerialNumber"), &s) && s == serial));
        if (!equal) { CFRelease(info); continue; }
        if (match) { CFRelease(info); ambiguous = true; break; }
        match = info;
    }
    IOObjectRelease(iterator);
    if (ambiguous) d->display.edid_status = CATALYST_STATUS_CODE_ERROR_INVALID_STATE;
    else if (match) {
        CFTypeRef edid = CFDictionaryGetValue(match, CFSTR("IODisplayEDID"));
        if (edid) {
            if (CFGetTypeID(edid) != CFDataGetTypeID()) d->display.edid_status = CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT;
            else {
                CFDataRef data = (CFDataRef) edid;
                CFIndex length = CFDataGetLength(data);
                if (length < 0 || (CATALYST_ULONG) length > CATALYST_NUINT_MAX)
                    d->display.edid_status = CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW;
                else d->display.edid_status = athdv_display_copy_edid(&d->display, CFDataGetBytePtr(data), (catalyst::NUINT) length);
            }
        }
        CFTypeRef names = CFDictionaryGetValue(match, CFSTR("DisplayProductName"));
        if (names && CFGetTypeID(names) == CFDictionaryGetTypeID() && CFDictionaryGetCount((CFDictionaryRef) names) == 1) {
            const void* name = 0;
            CFDictionaryGetKeysAndValues((CFDictionaryRef) names, 0, &name);
            if (name && CFGetTypeID(name) == CFStringGetTypeID()) {
                if (CFStringGetCString((CFStringRef) name, (char*) d->product.value, sizeof(d->product.value), kCFStringEncodingUTF8)) {
                    catalyst::NUINT length = 0;
                    catalyst::RESULT r = catte_utf8_length(d->product.value, &length);
                    d->product.status = r.status == CATALYST_STATUS_CODE_SUCCESS && !length ? CATALYST_STATUS_CODE_ERROR_NOT_FOUND : r.status;
                    if (d->product.status == CATALYST_STATUS_CODE_SUCCESS) d->descriptor = d->product;
                } else d->product.status = CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW;
            }
        }
    }
    if (match) CFRelease(match);
}

static catalyst::STATUS_CODE athdv_mac_displays(ATHDVLIB_DEVICES* snapshot) {
    uint32_t count = 0;
    if (CGGetOnlineDisplayList(0, 0, &count) != kCGErrorSuccess) return CATALYST_STATUS_CODE_ERROR_IO_ERROR;
    if (!count) return CATALYST_STATUS_CODE_SUCCESS;
    if ((CATALYST_ULONG) count > CATALYST_NUINT_MAX / sizeof(CGDirectDisplayID)) return CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW;
    CATMMLIB_POINTERW memory = 0;
    catalyst::RESULT r = catmm_alloc(&memory, (catalyst::NUINT) count * sizeof(CGDirectDisplayID));
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return r.status;
    CGDirectDisplayID* ids = (CGDirectDisplayID*) memory;
    uint32_t actual = 0;
    catalyst::STATUS_CODE status = CATALYST_STATUS_CODE_SUCCESS;
    if (CGGetOnlineDisplayList(count, ids, &actual) != kCGErrorSuccess || actual > count) status = CATALYST_STATUS_CODE_ERROR_IO_ERROR;
    for (uint32_t j = 0; j < actual && status == CATALYST_STATUS_CODE_SUCCESS; ++j) {
        CGDirectDisplayID id = ids[j];
        ATHDVLIB_DEVICE* d;
        r = athdv_native_add(snapshot, &d);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) { status = r.status; break; }
        athdv_display_init(&d->display);
        d->display.handle = (catalyst::NUINT) id;
        d->display.handle_status = CATALYST_STATUS_CODE_SUCCESS;
        d->type = ATHDVLIB_DEVICE_LOGICAL;
        d->scheme = (CATALYST_ASCII) "darwin-display";
        athdv_mac_hex(d->identifier.value, id, 8);
        d->identifier.status = CATALYST_STATUS_CODE_SUCCESS;
        d->capability_status = CATALYST_STATUS_CODE_SUCCESS;
        d->output = CATALYST_TRUE;
        uint32_t vendor = CGDisplayVendorNumber(id), product = CGDisplayModelNumber(id);
        if (vendor) {
            d->hwid_scheme = (CATALYST_ASCII) "darwin-display-model";
            athdv_mac_hex(d->hwids[0].value, vendor, 8);
            d->hwids[0].value[8] = ':';
            athdv_mac_hex(d->hwids[0].value + 9, product, 8);
            d->hwids[0].status = CATALYST_STATUS_CODE_SUCCESS;
            d->hwid_count = 1;
        }
        uint32_t serial = CGDisplaySerialNumber(id);
        if (serial) {
            athdv_mac_hex(d->serial.value, serial, 8);
            d->serial.status = CATALYST_STATUS_CODE_SUCCESS;
        }
        if (CGDisplayIsBuiltin(id)) {
            const char name[] = "Built-in display";
            catmm_copy(d->descriptor.value, name, sizeof(name));
            d->descriptor.status = CATALYST_STATUS_CODE_SUCCESS;
        }
        athdv_mac_display_edid(d, id, ids, actual);
    }
    r = catmm_free(memory);
    return CATALYST_STATUS_CODE_IS_SUCCESS(r.status) ? status : r.status;
}

CATALYST_RESULT athdv_devices_enumerate(ATHDVLIB_DEVICES** devices) {
    *devices = 0;
    IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_ALLOCATION_FAILED, 0x01, 0x01, 0);
    IOHIDManagerSetDeviceMatching(manager, 0);
    CFSetRef set = IOHIDManagerCopyDevices(manager);
    CATMMLIB_POINTERW memory = 0;
    catalyst::RESULT r = catmm_alloc(&memory, sizeof(ATHDVLIB_DEVICES));
    if (!catalyst::status_code_is_success(r.status)) { if (set) CFRelease(set); CFRelease(manager); return catalyst::RESULT(r.status, 0x01, 0x02, r.detail); }
    ATHDVLIB_DEVICES* snapshot = (ATHDVLIB_DEVICES*) memory;
    catalyst::STATUS_CODE status = catalyst::STATUS_CODE_SUCCESS;
    if (set) {
        CFIndex count = CFSetGetCount(set);
        if (count < 0 || (CATALYST_ULONG) count > CATALYST_NUINT_MAX / sizeof(CFTypeRef)) status = catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW;
        else if (count) {
            CATMMLIB_POINTERW values = 0;
            r = catmm_alloc(&values, (catalyst::NUINT) count * sizeof(CFTypeRef));
            if (!catalyst::status_code_is_success(r.status)) status = r.status;
            else {
                CFSetGetValues(set, (const void**) values);
                for (CFIndex j = 0; j < count && status == catalyst::STATUS_CODE_SUCCESS; ++j)
                    status = athdv_mac_entry(snapshot, ((IOHIDDeviceRef*) values)[j]);
                r = catmm_free(values);
                if (!catalyst::status_code_is_success(r.status)) status = r.status;
            }
        }
        CFRelease(set);
    }
    CFRelease(manager);
    if (status == CATALYST_STATUS_CODE_SUCCESS) status = athdv_mac_displays(snapshot);
    if (status != catalyst::STATUS_CODE_SUCCESS) {
        r = athdv_devices_free(snapshot);
        if (!catalyst::status_code_is_success(r.status)) return catalyst::RESULT(r.status, 0x01, 0x03, r.detail);
        return catalyst::RESULT(status, 0x01, 0x05, 0);
    }
    *devices = snapshot;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x01, 0x06, 0);
}
