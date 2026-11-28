#include "catdvlib.native.hpp"
#include "catio.darwin.hpp"

static void catdv_mac_text(IOHIDDeviceRef hid, CFStringRef key, CATDVLIB_NATIVE_TEXT* text) {
    CFTypeRef value = IOHIDDeviceGetProperty(hid, key);
    if (!value) return;
    if (CFGetTypeID(value) != CFStringGetTypeID()) { text->status = STATUS_CODE_ERROR_MALFORMED_INPUT; return; }
    if (!CFStringGetCString((CFStringRef) value, (char*) text->value, sizeof(text->value), kCFStringEncodingUTF8)) {
        text->status = STATUS_CODE_ERROR_BUFFER_OVERFLOW; return;
    }
    NUINT length;
    RESULT r = catte_utf8_length(text->value, &length);
    text->status = status_code_is_success(r.status) && length == 0 ? STATUS_CODE_ERROR_NOT_FOUND : r.status;
}
static STATUS_CODE catdv_mac_number(IOHIDDeviceRef hid, CFStringRef key, CATALYST_ULONG* number) {
    CFTypeRef value = IOHIDDeviceGetProperty(hid, key);
    if (!value) return STATUS_CODE_ERROR_NOT_FOUND;
    SInt64 n;
    if (CFGetTypeID(value) != CFNumberGetTypeID() || !CFNumberGetValue((CFNumberRef) value, kCFNumberSInt64Type, &n) || n < 0)
        return STATUS_CODE_ERROR_MALFORMED_INPUT;
    *number = (CATALYST_ULONG) n; return STATUS_CODE_SUCCESS;
}
static STATUS_CODE catdv_mac_entry(CATDVLIB_DEVICES* devices, IOHIDDeviceRef hid) {
    uint64_t native_id;
    io_service_t service = IOHIDDeviceGetService(hid);
    if (!service) return STATUS_CODE_SUCCESS;
    kern_return_t err = IORegistryEntryGetRegistryEntryID(service, &native_id);
    if (err != KERN_SUCCESS) return catio_mac_status(err);
    CATDVLIB_DEVICE* d;
    RESULT r = catdv_native_add(devices, &d);
    if (!status_code_is_success(r.status)) return r.status;
    d->scheme = (CATALYST_ASCII) "darwin-hid"; d->type = CATDVLIB_DEVICE_LOGICAL;
    catio_mac_hex(d->identifier.value, (CATALYST_ULONG) native_id, 16);
    d->identifier.status = STATUS_CODE_SUCCESS;
    catdv_mac_text(hid, CFSTR(kIOHIDManufacturerKey), &d->manufacturer);
    catdv_mac_text(hid, CFSTR(kIOHIDProductKey), &d->product);
    catdv_mac_text(hid, CFSTR(kIOHIDSerialNumberKey), &d->serial);
    d->descriptor = d->product;
    CATALYST_ULONG vendor, product, revision;
    STATUS_CODE vs = catdv_mac_number(hid, CFSTR(kIOHIDVendorIDKey), &vendor);
    STATUS_CODE ps = catdv_mac_number(hid, CFSTR(kIOHIDProductIDKey), &product);
    if (vs == STATUS_CODE_SUCCESS && ps == STATUS_CODE_SUCCESS) {
        if (vendor > 65535 || product > 65535) d->hwid_status = STATUS_CODE_ERROR_MALFORMED_INPUT;
        else {
            CFTypeRef transport = IOHIDDeviceGetProperty(hid, CFSTR(kIOHIDTransportKey));
            d->hwid_scheme = (CATALYST_ASCII) "darwin-hid";
            if (transport && CFGetTypeID(transport) == CFStringGetTypeID() &&
                CFStringCompare((CFStringRef) transport, CFSTR("USB"), 0) == kCFCompareEqualTo)
                d->hwid_scheme = (CATALYST_ASCII) "usb";
            catio_mac_hex(d->hwids[0].value, vendor, 4);
            d->hwids[0].value[4] = ':';
            catio_mac_hex(d->hwids[0].value + 5, product, 4);
            d->hwids[0].status = STATUS_CODE_SUCCESS; d->hwid_count = 1;
        }
    } else if (vs != STATUS_CODE_SUCCESS && vs != STATUS_CODE_ERROR_NOT_FOUND) d->hwid_status = vs;
    else if (ps != STATUS_CODE_SUCCESS && ps != STATUS_CODE_ERROR_NOT_FOUND) d->hwid_status = ps;
    d->revision.status = catdv_mac_number(hid, CFSTR(kIOHIDVersionNumberKey), &revision);
    if (d->revision.status == STATUS_CODE_SUCCESS) catio_mac_hex(d->revision.value, revision, 16);
    CFArrayRef elements = IOHIDDeviceCopyMatchingElements(hid, 0, kIOHIDOptionsTypeNone);
    if (elements) {
        d->capability_status = STATUS_CODE_SUCCESS;
        for (CFIndex j = 0; j < CFArrayGetCount(elements); ++j) {
            IOHIDElementRef element = (IOHIDElementRef) CFArrayGetValueAtIndex(elements, j);
            if (catio_mac_direction(element, TRUE)) d->input = TRUE;
            if (catio_mac_direction(element, FALSE)) d->output = TRUE;
        }
        CFRelease(elements);
    } else d->capability_status = STATUS_CODE_ERROR_NOT_FOUND;
    return STATUS_CODE_SUCCESS;
}
CATALYST_RESULT catdv_devices_enumerate(CATDVLIB_DEVICES** devices) {
    *devices = 0;
    IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager) return RESULT(STATUS_CODE_ERROR_ALLOCATION_FAILED, 0x01, 0x01, 0);
    IOHIDManagerSetDeviceMatching(manager, 0);
    CFSetRef set = IOHIDManagerCopyDevices(manager);
    CATMMLIB_POINTERW memory = 0;
    RESULT r = catmm_alloc(&memory, sizeof(CATDVLIB_DEVICES));
    if (!status_code_is_success(r.status)) { if (set) CFRelease(set); CFRelease(manager); return RESULT(r.status, 0x01, 0x02, r.detail); }
    CATDVLIB_DEVICES* snapshot = (CATDVLIB_DEVICES*) memory;
    STATUS_CODE status = STATUS_CODE_SUCCESS;
    if (set) {
        CFIndex count = CFSetGetCount(set);
        if (count < 0 || (CATALYST_ULONG) count > CATALYST_NUINT_MAX / sizeof(CFTypeRef)) status = STATUS_CODE_ERROR_BUFFER_OVERFLOW;
        else if (count) {
            CATMMLIB_POINTERW values = 0;
            r = catmm_alloc(&values, (NUINT) count * sizeof(CFTypeRef));
            if (!status_code_is_success(r.status)) status = r.status;
            else {
                CFSetGetValues(set, (const void**) values);
                for (CFIndex j = 0; j < count && status == STATUS_CODE_SUCCESS; ++j)
                    status = catdv_mac_entry(snapshot, ((IOHIDDeviceRef*) values)[j]);
                r = catmm_free(values);
                if (!status_code_is_success(r.status)) status = r.status;
            }
        }
        CFRelease(set);
    }
    CFRelease(manager);
    if (status != STATUS_CODE_SUCCESS) {
        r = catdv_devices_free(snapshot);
        if (!status_code_is_success(r.status)) return RESULT(r.status, 0x01, 0x03, r.detail);
        return RESULT(status, 0x01, 0x05, 0);
    }
    *devices = snapshot;
    return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x06, 0);
}
