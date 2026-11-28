#include "catdvlib.h"
#include "catdvlib.linux.hpp"
#include "cattelib.h"
#include <dirent.h>

/* Linux sysfs text attributes are read once into the snapshot. Long values
 * are reported, never silently truncated. */
struct CATDVLIB_TEXT {
    CATALYST_STATUS_CODE status;
    char text[1024];
};

struct CATDVLIB_DEVICE {
    CATDVLIB_DEVICE_TYPE type;
    char path[PATH_MAX];
    CATDVLIB_TEXT hwid, bus_hwid, serial, manufacturer, product, descriptor, revision;
    CATALYST_ASCII bus_scheme;
    CATALYST_STATUS_CODE capability_status;
    CATALYST_BOOL input, output;
};

struct CATDVLIB_DEVICES {
    CATDVLIB_DEVICE* data;
    CATALYST_NUINT count;
    CATALYST_NUINT capacity;
};

static void catdv_attribute(const char* root, const char* name, CATDVLIB_TEXT* field) {
    char path[PATH_MAX];
    field->text[0] = 0;
    if (!catdv_join(path, sizeof(path), root, name)) { field->status = catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW; return; }
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) { field->status = catdv_errno(errno); return; }
    catalyst::NUINT used = 0;
    field->status = catalyst::STATUS_CODE_SUCCESS;
    while (used < sizeof(field->text)) {
        ssize_t got = read(fd, field->text + used, sizeof(field->text) - used);
        if (got < 0) { field->status = catdv_errno(errno); break; }
        if (got == 0) break;
        used += (catalyst::NUINT) got;
    }
    if (close(fd) != 0 && field->status == catalyst::STATUS_CODE_SUCCESS) field->status = catdv_errno(errno);
    if (field->status != catalyst::STATUS_CODE_SUCCESS) return;
    if (used == sizeof(field->text)) { field->status = catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW; return; }
    while (used && (field->text[used - 1] == '\n' || field->text[used - 1] == '\r')) --used;
    field->text[used] = 0;
    if (used == 0) { field->status = catalyst::STATUS_CODE_ERROR_NOT_FOUND; return; }
    for (catalyst::NUINT j = 0; j < used; ++j) {
        if (field->text[j] == 0) { field->status = catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT; return; }
    }
    CATALYST_NUINT length;
    CATALYST_RESULT r = catte_utf8_length((CATALYST_UTF8) field->text, &length);
    field->status = r.status;
}

/* Decode only the hexadecimal digits used by sysfs identity and capability fields. */
static catalyst::BOOL catdv_hex_digit(char character, catalyst::BYTE* digit) {
    if (character >= '0' && character <= '9') *digit = (catalyst::BYTE) (character - '0');
    else if (character >= 'a' && character <= 'f') *digit = (catalyst::BYTE) (character - 'a' + 10);
    else if (character >= 'A' && character <= 'F') *digit = (catalyst::BYTE) (character - 'A' + 10);
    else return catalyst::FALSE;
    return catalyst::TRUE;
}

/* The Linux event-type mask covers EV_MAX == 0x1f. Reject malformed or
 * unrepresentable values before shifting into the native 32-bit field. */
static catalyst::BOOL catdv_event_mask(const char* text, catalyst::UINT* mask) {
    *mask = 0;
    if (!*text) return catalyst::FALSE;
    while (*text) {
        catalyst::BYTE digit;
        if (!catdv_hex_digit(*text++, &digit) || *mask > ((catalyst::UINT) CATALYST_UINT_MAX >> 4)) return catalyst::FALSE;
        *mask = (catalyst::UINT) ((*mask << 4) | digit);
    }
    return catalyst::TRUE;
}

/* Validate hexadecimal ID fields without assuming a native integer width. */
static const char* catdv_hex_id(const CATDVLIB_TEXT* field) {
    const char* text = field->text;
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) text += 2;
    for (catalyst::BYTE j = 0; j < 4; ++j) {
        catalyst::BYTE digit;
        if (!catdv_hex_digit(text[j], &digit)) return 0;
    }
    if (text[4] != 0) return 0;
    return text;
}

/* Store one bus/model identifier in addition to the native modalias. */
static void catdv_bus_hwid(CATDVLIB_DEVICE* device, CATALYST_BYTE kind) {
    CATDVLIB_TEXT vendor, product, bus;
    const char* root = device->path;
    const char* vendor_key = kind == 0 ? "idVendor" : (kind == 1 ? "vendor" : "device/id/vendor");
    const char* product_key = kind == 0 ? "idProduct" : (kind == 1 ? "device" : "device/id/product");
    /* USB interface identity is distinct from the physical USB model. The
     * interface's native modalias remains available; do not invent a bus ID. */
    device->bus_hwid.status = catalyst::STATUS_CODE_ERROR_NOT_FOUND;
    if (kind == 0 && device->type == CATDVLIB_DEVICE_LOGICAL) return;
    catdv_attribute(root, vendor_key, &vendor);
    catdv_attribute(root, product_key, &product);
    if (vendor.status != catalyst::STATUS_CODE_SUCCESS) { device->bus_hwid.status = vendor.status; return; }
    if (product.status != catalyst::STATUS_CODE_SUCCESS) { device->bus_hwid.status = product.status; return; }
    const char* vid = catdv_hex_id(&vendor);
    const char* pid = catdv_hex_id(&product);
    if (!vid || !pid) { device->bus_hwid.status = catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT; return; }
    catalyst::NUINT used = 0;
    char* value = device->bus_hwid.text;
    const catalyst::NUINT capacity = sizeof(device->bus_hwid.text);
    if (kind == 2) {
        catdv_attribute(root, "device/id/bustype", &bus);
        if (bus.status != catalyst::STATUS_CODE_SUCCESS) { device->bus_hwid.status = bus.status; return; }
        const char* bid = catdv_hex_id(&bus);
        if (!bid) { device->bus_hwid.status = catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT; return; }
        device->bus_scheme = (CATALYST_ASCII) "linux-input";
        if (!catdv_append(value, capacity, &used, bid) ||
            !catdv_append(value, capacity, &used, ":")) {
            device->bus_hwid.status = catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW;
            return;
        }
    } else {
        device->bus_scheme = (CATALYST_ASCII) (kind == 0 ? "usb" : "pci");
    }
    if (!catdv_append(value, capacity, &used, vid) ||
        !catdv_append(value, capacity, &used, ":") ||
        !catdv_append(value, capacity, &used, pid)) {
        device->bus_hwid.status = catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW;
        return;
    }
    /* Hexadecimal fields have an explicit uppercase canonical spelling. */
    for (char* p = device->bus_hwid.text; *p; ++p)
        if (*p >= 'a' && *p <= 'f') *p = (char) (*p - 'a' + 'A');
    device->bus_hwid.status = catalyst::STATUS_CODE_SUCCESS;
}

static void catdv_fill(CATDVLIB_DEVICE* device, CATALYST_BYTE kind) {
    device->type = kind == 2 ? CATDVLIB_DEVICE_LOGICAL : CATDVLIB_DEVICE_PHYSICAL;
    /* USB interface records are logical; PCI records identify hardware functions. */
    if (kind == 0) {
        const char* name = catdv_basename(device->path);
        while (*name && *name != ':') ++name;
        if (*name == ':') device->type = CATDVLIB_DEVICE_LOGICAL;
    }
    const char* root = device->path;
    if (kind == 2) {
        catdv_attribute(root, "device/modalias", &device->hwid);
        /* EVIOCGUNIQ is not necessarily a manufacturer serial. Do not expose it as one. */
        device->serial.status = catalyst::STATUS_CODE_ERROR_NOT_FOUND;
        catdv_attribute(root, "device/name", &device->descriptor);
        device->manufacturer.status = catalyst::STATUS_CODE_ERROR_NOT_FOUND;
        device->product.status = catalyst::STATUS_CODE_ERROR_NOT_FOUND;
        device->revision.status = catalyst::STATUS_CODE_ERROR_NOT_FOUND;
        CATDVLIB_TEXT ev;
        catdv_attribute(root, "device/capabilities/ev", &ev);
        device->capability_status = ev.status;
        if (ev.status == catalyst::STATUS_CODE_SUCCESS) {
            catalyst::UINT mask;
            if (!catdv_event_mask(ev.text, &mask)) device->capability_status = catalyst::STATUS_CODE_ERROR_MALFORMED_INPUT;
            else {
                device->input = (mask & (((catalyst::UINT) 1 << EV_KEY) | ((catalyst::UINT) 1 << EV_ABS) | ((catalyst::UINT) 1 << EV_REL) | ((catalyst::UINT) 1 << EV_SW) | ((catalyst::UINT) 1 << EV_MSC))) ? CATALYST_TRUE : CATALYST_FALSE;
                device->output = (mask & (((catalyst::UINT) 1 << EV_LED) | ((catalyst::UINT) 1 << EV_SND) | ((catalyst::UINT) 1 << EV_FF))) ? CATALYST_TRUE : CATALYST_FALSE;
            }
        }
    } else {
        catdv_attribute(root, "modalias", &device->hwid);
        catdv_attribute(root, "serial", &device->serial);
        catdv_attribute(root, "manufacturer", &device->manufacturer);
        catdv_attribute(root, "product", &device->product);
        catdv_attribute(root, "product", &device->descriptor);
        catdv_attribute(root, kind == 0 ? "bcdDevice" : "revision", &device->revision);
        device->capability_status = catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED;
    }
    catdv_bus_hwid(device, kind);
}

static CATALYST_RESULT catdv_scan(CATDVLIB_DEVICES* snapshot, const char* root, CATALYST_BYTE kind) {
    DIR* dir = opendir(root);
    if (!dir) {
        if (errno == ENOENT) return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x80, 1, 0);
        return catalyst::RESULT(catdv_errno(errno), 0x80, 2, 0);
    }
    CATALYST_RESULT result = catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x80, 3, 0);
    for (;;) {
        errno = 0;
        struct dirent* entry = readdir(dir);
        if (!entry) {
            if (errno) result = catalyst::RESULT(catdv_errno(errno), 0x80, 4, 0);
            break;
        }
        if (entry->d_name[0] == '.') continue;
        if (kind == 2 && !catdv_event_name(entry->d_name)) continue;
        char path[PATH_MAX], resolved[PATH_MAX];
        if (!catdv_join(path, sizeof(path), root, entry->d_name)) { result = catalyst::RESULT(catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x80, 5, 0); break; }
        catalyst::STATUS_CODE resolved_status = catdv_resolve(path, resolved, sizeof(resolved));
        if (resolved_status != catalyst::STATUS_CODE_SUCCESS) {
            if (resolved_status == catalyst::STATUS_CODE_ERROR_NOT_FOUND) continue;
            result = catalyst::RESULT(resolved_status, 0x80, 6, 0); break;
        }
        CATALYST_NUINT length;
        CATALYST_RESULT valid = catte_utf8_length((CATALYST_UTF8) resolved, &length);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(valid.status)) {
            result = catalyst::RESULT(valid.status, 0x80, 11, valid.detail);
            break;
        }
        CATALYST_BOOL duplicate = CATALYST_FALSE;
        for (CATALYST_NUINT j = 0; j < snapshot->count; ++j)
            if (catdv_native_equal(snapshot->data[j].path, resolved)) { duplicate = CATALYST_TRUE; break; }
        if (duplicate) continue;
        if (snapshot->count == snapshot->capacity) {
            const CATALYST_NUINT maximum = ((CATALYST_NUINT) -1) / sizeof(CATDVLIB_DEVICE);
            if (snapshot->capacity == maximum) { result = catalyst::RESULT(catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x80, 7, 0); break; }
            CATALYST_NUINT next = snapshot->capacity == 0 ? 1 :
                (snapshot->capacity > maximum / 2 ? maximum : snapshot->capacity * 2);
            if (next > maximum || next == 0) { result = catalyst::RESULT(catalyst::STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x80, 8, 0); break; }
            CATMMLIB_POINTERW memory = snapshot->data;
            CATALYST_RESULT r = catmm_realloc(&memory, next * sizeof(CATDVLIB_DEVICE));
            if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) { result = catalyst::RESULT(r.status, 0x80, 9, r.detail); break; }
            snapshot->data = (CATDVLIB_DEVICE*) memory;
            snapshot->capacity = next;
        }
        CATDVLIB_DEVICE* device = &snapshot->data[snapshot->count];
        catmm_fill(device, 0, sizeof(*device));
        catmm_copy(device->path, resolved, catdv_native_extent(resolved) + 1);
        catdv_fill(device, kind);
        ++snapshot->count;
    }
    if (closedir(dir) != 0 && CATALYST_STATUS_CODE_IS_SUCCESS(result.status))
        return catalyst::RESULT(catdv_errno(errno), 0x80, 10, 0);
    return result;
}

CATALYST_RESULT catdv_devices_enumerate(CATDVLIB_DEVICES** devices) {
    *devices = 0;
    CATMMLIB_POINTERW memory = 0;
    CATALYST_RESULT r = catmm_alloc(&memory, sizeof(CATDVLIB_DEVICES));
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x01, 0x01, r.detail);
    CATDVLIB_DEVICES* snapshot = (CATDVLIB_DEVICES*) memory;
    const char* roots[] = { "/sys/bus/usb/devices", "/sys/bus/pci/devices", "/sys/class/input" };
    if (access("/sys/devices", F_OK) != 0 || access("/proc/self/fd", X_OK) != 0) {
        CATALYST_STATUS_CODE status = catdv_errno(errno);
        CATALYST_RESULT released = catmm_free(snapshot);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(released.status)) return catalyst::RESULT(released.status, 0x01, 0x02, released.detail);
        return catalyst::RESULT(status, 0x01, 0x03, 0);
    }
    for (CATALYST_BYTE k = 0; k < 3; ++k) {
        r = catdv_scan(snapshot, roots[k], k);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) {
            CATALYST_RESULT released = catdv_devices_free(snapshot);
            if (!CATALYST_STATUS_CODE_IS_SUCCESS(released.status)) return catalyst::RESULT(released.status, 0x01, 0x04, released.detail);
            return catalyst::RESULT(r.status, 0x01, 0x05, r.detail);
        }
    }
    *devices = snapshot;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x01, 0x06, 0);
}

CATALYST_RESULT catdv_devices_free(CATDVLIB_DEVICES* devices) {
    if (devices->data != 0) {
        CATALYST_RESULT r = catmm_free(devices->data);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x02, 0x01, r.detail);
        devices->data = 0;
        devices->count = 0;
        devices->capacity = 0;
    }
    CATALYST_RESULT r = catmm_free(devices);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return catalyst::RESULT(r.status, 0x02, 0x02, r.detail);
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x02, 0x03, 0);
}

CATALYST_NUINT catdv_devices_get_count(const CATDVLIB_DEVICES* devices) {
    return devices->count;
}

const CATDVLIB_DEVICE* catdv_devices_get(const CATDVLIB_DEVICES* devices, CATALYST_NUINT index) {
    return &devices->data[index];
}

CATDVLIB_DEVICE_TYPE catdv_device_get_type(const CATDVLIB_DEVICE* device) {
    return device->type;
}

CATALYST_RESULT catdv_device_get_identifier(const CATDVLIB_DEVICE* device, CATDVLIB_IDENTIFIER* identifier) {
    identifier->scheme = (CATALYST_ASCII) "linux-sysfs";
    identifier->value = (CATALYST_UTF8) device->path;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x06, 0x01, 0);
}

CATALYST_RESULT catdv_device_get_hwid_count(const CATDVLIB_DEVICE* device, CATALYST_NUINT* count) {
    *count = 0;
    if (device->hwid.status != catalyst::STATUS_CODE_SUCCESS && device->hwid.status != catalyst::STATUS_CODE_ERROR_NOT_FOUND) return catalyst::RESULT(device->hwid.status, 0x07, 0x01, 0);
    if (device->bus_hwid.status != catalyst::STATUS_CODE_SUCCESS && device->bus_hwid.status != catalyst::STATUS_CODE_ERROR_NOT_FOUND) return catalyst::RESULT(device->bus_hwid.status, 0x07, 0x03, 0);
    if (device->bus_hwid.status == catalyst::STATUS_CODE_SUCCESS) ++*count;
    if (device->hwid.status == catalyst::STATUS_CODE_SUCCESS) ++*count;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x07, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_hwid(const CATDVLIB_DEVICE* device, CATALYST_NUINT index, CATDVLIB_IDENTIFIER* hwid) {
    if (device->bus_hwid.status == catalyst::STATUS_CODE_SUCCESS && index == 0) {
        hwid->scheme = device->bus_scheme;
        hwid->value = (CATALYST_UTF8) device->bus_hwid.text;
        return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x08, 0x02, 0);
    }
    hwid->scheme = (CATALYST_ASCII) "linux-modalias";
    hwid->value = (CATALYST_UTF8) device->hwid.text;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x08, 0x01, 0);
}

CATALYST_RESULT catdv_device_get_serial(const CATDVLIB_DEVICE* device, CATALYST_UTF8* serial) {
    *serial = 0;
    if (device->serial.status != catalyst::STATUS_CODE_SUCCESS) return catalyst::RESULT(device->serial.status, 0x09, 0x01, 0);
    *serial = (CATALYST_UTF8) device->serial.text;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x09, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_manufacturer(const CATDVLIB_DEVICE* device, CATALYST_UTF8* manufacturer) {
    *manufacturer = 0;
    if (device->manufacturer.status != catalyst::STATUS_CODE_SUCCESS) return catalyst::RESULT(device->manufacturer.status, 0x0A, 0x01, 0);
    *manufacturer = (CATALYST_UTF8) device->manufacturer.text;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x0A, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_product(const CATDVLIB_DEVICE* device, CATALYST_UTF8* product) {
    *product = 0;
    if (device->product.status != catalyst::STATUS_CODE_SUCCESS) return catalyst::RESULT(device->product.status, 0x0B, 0x01, 0);
    *product = (CATALYST_UTF8) device->product.text;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x0B, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_descriptor(const CATDVLIB_DEVICE* device, CATALYST_UTF8* descriptor) {
    *descriptor = 0;
    if (device->descriptor.status != catalyst::STATUS_CODE_SUCCESS) return catalyst::RESULT(device->descriptor.status, 0x0C, 0x01, 0);
    *descriptor = (CATALYST_UTF8) device->descriptor.text;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x0C, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_revision(const CATDVLIB_DEVICE* device, CATALYST_UTF8* revision) {
    *revision = 0;
    if (device->revision.status != catalyst::STATUS_CODE_SUCCESS) return catalyst::RESULT(device->revision.status, 0x0D, 0x01, 0);
    *revision = (CATALYST_UTF8) device->revision.text;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x0D, 0x02, 0);
}

CATALYST_RESULT catdv_device_has_input(const CATDVLIB_DEVICE* device, CATALYST_BOOL* supported) {
    *supported = CATALYST_FALSE;
    if (device->type != CATDVLIB_DEVICE_LOGICAL) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0E, 0x01, 0);
    if (device->capability_status != catalyst::STATUS_CODE_SUCCESS) return catalyst::RESULT(device->capability_status, 0x0E, 0x02, 0);
    *supported = device->input;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x0E, 0x03, 0);
}

CATALYST_RESULT catdv_device_has_output(const CATDVLIB_DEVICE* device, CATALYST_BOOL* supported) {
    *supported = CATALYST_FALSE;
    if (device->type != CATDVLIB_DEVICE_LOGICAL) return catalyst::RESULT(catalyst::STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0F, 0x01, 0);
    if (device->capability_status != catalyst::STATUS_CODE_SUCCESS) return catalyst::RESULT(device->capability_status, 0x0F, 0x02, 0);
    *supported = device->output;
    return catalyst::RESULT(catalyst::STATUS_CODE_SUCCESS, 0x0F, 0x03, 0);
}

