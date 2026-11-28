#include "catdvlib.h"
#include "catio.linux.hpp"
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
    CATDVLIB_TEXT hwid, serial, manufacturer, product, descriptor, revision;
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
    int len = snprintf(path, sizeof(path), "%s/%s", root, name);
    if (len < 0 || (size_t) len >= sizeof(path)) { field->status = STATUS_CODE_ERROR_BUFFER_OVERFLOW; return; }
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) { field->status = catio_errno(errno); return; }
    size_t used = 0;
    field->status = STATUS_CODE_SUCCESS;
    while (used < sizeof(field->text)) {
        ssize_t got = read(fd, field->text + used, sizeof(field->text) - used);
        if (got < 0) { field->status = catio_errno(errno); break; }
        if (got == 0) break;
        used += (size_t) got;
    }
    if (close(fd) != 0 && field->status == STATUS_CODE_SUCCESS) field->status = catio_errno(errno);
    if (field->status != STATUS_CODE_SUCCESS) return;
    if (used == sizeof(field->text)) { field->status = STATUS_CODE_ERROR_BUFFER_OVERFLOW; return; }
    while (used && (field->text[used - 1] == '\n' || field->text[used - 1] == '\r')) --used;
    field->text[used] = 0;
    if (used == 0) { field->status = STATUS_CODE_ERROR_NOT_FOUND; return; }
    if (memchr(field->text, 0, used)) { field->status = STATUS_CODE_ERROR_MALFORMED_INPUT; return; }
    CATALYST_NUINT length;
    CATALYST_RESULT r = catte_utf8_length((CATALYST_UTF8) field->text, &length);
    field->status = r.status;
}

static void catdv_fill(CATDVLIB_DEVICE* device, CATALYST_BYTE kind) {
    device->type = kind == 2 ? CATDVLIB_DEVICE_LOGICAL : CATDVLIB_DEVICE_PHYSICAL;
    /* USB interface records are logical; PCI records identify hardware functions. */
    if (kind == 0 && strchr(strrchr(device->path, '/') + 1, ':')) device->type = CATDVLIB_DEVICE_LOGICAL;
    const char* root = device->path;
    if (kind == 2) {
        catdv_attribute(root, "device/modalias", &device->hwid);
        /* EVIOCGUNIQ is not necessarily a manufacturer serial. Do not expose it as one. */
        device->serial.status = STATUS_CODE_ERROR_NOT_FOUND;
        catdv_attribute(root, "device/name", &device->descriptor);
        device->manufacturer.status = STATUS_CODE_ERROR_NOT_FOUND;
        device->product.status = STATUS_CODE_ERROR_NOT_FOUND;
        device->revision.status = STATUS_CODE_ERROR_NOT_FOUND;
        CATDVLIB_TEXT ev;
        catdv_attribute(root, "device/capabilities/ev", &ev);
        device->capability_status = ev.status;
        if (ev.status == STATUS_CODE_SUCCESS) {
            char* end = 0;
            errno = 0;
            unsigned long mask = strtoul(ev.text, &end, 16);
            if (errno || !end || *end) device->capability_status = STATUS_CODE_ERROR_MALFORMED_INPUT;
            else {
                device->input = (mask & ((1UL << EV_KEY) | (1UL << EV_ABS) | (1UL << EV_REL) | (1UL << EV_SW) | (1UL << EV_MSC))) ? CATALYST_TRUE : CATALYST_FALSE;
                device->output = (mask & ((1UL << EV_LED) | (1UL << EV_SND) | (1UL << EV_FF))) ? CATALYST_TRUE : CATALYST_FALSE;
            }
        }
    } else {
        catdv_attribute(root, "modalias", &device->hwid);
        catdv_attribute(root, "serial", &device->serial);
        catdv_attribute(root, "manufacturer", &device->manufacturer);
        catdv_attribute(root, "product", &device->product);
        catdv_attribute(root, "product", &device->descriptor);
        catdv_attribute(root, kind == 0 ? "bcdDevice" : "revision", &device->revision);
        device->capability_status = STATUS_CODE_ERROR_NOT_IMPLEMENTED;
    }
}

static CATALYST_RESULT catdv_scan(CATDVLIB_DEVICES* snapshot, const char* root, CATALYST_BYTE kind) {
    DIR* dir = opendir(root);
    if (!dir) {
        if (errno == ENOENT) return RESULT(STATUS_CODE_SUCCESS, 0x80, 1, 0);
        return RESULT(catio_errno(errno), 0x80, 2, 0);
    }
    CATALYST_RESULT result = RESULT(STATUS_CODE_SUCCESS, 0x80, 3, 0);
    for (;;) {
        errno = 0;
        struct dirent* entry = readdir(dir);
        if (!entry) {
            if (errno) result = RESULT(catio_errno(errno), 0x80, 4, 0);
            break;
        }
        if (entry->d_name[0] == '.') continue;
        if (kind == 2 && (strncmp(entry->d_name, "event", 5) != 0 || !entry->d_name[5] ||
            strspn(entry->d_name + 5, "0123456789") != strlen(entry->d_name + 5))) continue;
        char path[PATH_MAX], resolved[PATH_MAX];
        int len = snprintf(path, sizeof(path), "%s/%s", root, entry->d_name);
        if (len < 0 || (size_t) len >= sizeof(path)) { result = RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x80, 5, 0); break; }
        if (!realpath(path, resolved)) {
            if (errno == ENOENT || errno == ENODEV) continue;
            result = RESULT(catio_errno(errno), 0x80, 6, 0); break;
        }
        CATALYST_BOOL duplicate = CATALYST_FALSE;
        for (CATALYST_NUINT j = 0; j < snapshot->count; ++j)
            if (strcmp(snapshot->data[j].path, resolved) == 0) { duplicate = CATALYST_TRUE; break; }
        if (duplicate) continue;
        if (snapshot->count == snapshot->capacity) {
            const CATALYST_NUINT maximum = ((CATALYST_NUINT) -1) / sizeof(CATDVLIB_DEVICE);
            if (snapshot->capacity == maximum) { result = RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x80, 7, 0); break; }
            CATALYST_NUINT next = snapshot->capacity == 0 ? 1 :
                (snapshot->capacity > maximum / 2 ? maximum : snapshot->capacity * 2);
            if (next > maximum || next == 0) { result = RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x80, 8, 0); break; }
            CATMMLIB_POINTERW memory = snapshot->data;
            CATALYST_RESULT r = catmm_realloc(&memory, next * sizeof(CATDVLIB_DEVICE));
            if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) { result = RESULT(r.status, 0x80, 9, r.detail); break; }
            snapshot->data = (CATDVLIB_DEVICE*) memory;
            snapshot->capacity = next;
        }
        CATDVLIB_DEVICE* device = &snapshot->data[snapshot->count];
        catmm_fill(device, 0, sizeof(*device));
        catmm_copy(device->path, resolved, strlen(resolved) + 1);
        catdv_fill(device, kind);
        ++snapshot->count;
    }
    if (closedir(dir) != 0 && CATALYST_STATUS_CODE_IS_SUCCESS(result.status))
        return RESULT(catio_errno(errno), 0x80, 10, 0);
    return result;
}

CATALYST_RESULT catdv_devices_enumerate(CATDVLIB_DEVICES** devices) {
    *devices = 0;
    CATMMLIB_POINTERW memory = 0;
    CATALYST_RESULT r = catmm_alloc(&memory, sizeof(CATDVLIB_DEVICES));
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return RESULT(r.status, 0x01, 0x01, r.detail);
    CATDVLIB_DEVICES* snapshot = (CATDVLIB_DEVICES*) memory;
    const char* roots[] = { "/sys/bus/usb/devices", "/sys/bus/pci/devices", "/sys/class/input" };
    if (access("/sys", F_OK) != 0) {
        CATALYST_STATUS_CODE status = catio_errno(errno);
        CATALYST_RESULT released = catmm_free(snapshot);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(released.status)) return RESULT(released.status, 0x01, 0x02, released.detail);
        return RESULT(status, 0x01, 0x03, 0);
    }
    for (CATALYST_BYTE k = 0; k < 3; ++k) {
        r = catdv_scan(snapshot, roots[k], k);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) {
            CATALYST_RESULT released = catdv_devices_free(snapshot);
            if (!CATALYST_STATUS_CODE_IS_SUCCESS(released.status)) return RESULT(released.status, 0x01, 0x04, released.detail);
            return RESULT(r.status, 0x01, 0x05, r.detail);
        }
    }
    *devices = snapshot;
    return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x06, 0);
}

CATALYST_RESULT catdv_devices_free(CATDVLIB_DEVICES* devices) {
    if (devices->data != 0) {
        CATALYST_RESULT r = catmm_free(devices->data);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return RESULT(r.status, 0x02, 0x01, r.detail);
        devices->data = 0;
        devices->count = 0;
        devices->capacity = 0;
    }
    CATALYST_RESULT r = catmm_free(devices);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(r.status)) return RESULT(r.status, 0x02, 0x02, r.detail);
    return RESULT(STATUS_CODE_SUCCESS, 0x02, 0x03, 0);
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
    return RESULT(STATUS_CODE_SUCCESS, 0x06, 0x01, 0);
}

CATALYST_RESULT catdv_device_get_hwid_count(const CATDVLIB_DEVICE* device, CATALYST_NUINT* count) {
    *count = 0;
    if (device->hwid.status != STATUS_CODE_SUCCESS && device->hwid.status != STATUS_CODE_ERROR_NOT_FOUND) return RESULT(device->hwid.status, 0x07, 0x01, 0);
    *count = device->hwid.status == STATUS_CODE_SUCCESS ? 1 : 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_hwid(const CATDVLIB_DEVICE* device, CATALYST_NUINT index, CATDVLIB_IDENTIFIER* hwid) {
    (void) index;
    hwid->scheme = (CATALYST_ASCII) "linux-modalias";
    hwid->value = (CATALYST_UTF8) device->hwid.text;
    return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x01, 0);
}

CATALYST_RESULT catdv_device_get_serial(const CATDVLIB_DEVICE* device, CATALYST_UTF8* serial) {
    *serial = 0;
    if (device->serial.status != STATUS_CODE_SUCCESS) return RESULT(device->serial.status, 0x09, 0x01, 0);
    *serial = (CATALYST_UTF8) device->serial.text;
    return RESULT(STATUS_CODE_SUCCESS, 0x09, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_manufacturer(const CATDVLIB_DEVICE* device, CATALYST_UTF8* manufacturer) {
    *manufacturer = 0;
    if (device->manufacturer.status != STATUS_CODE_SUCCESS) return RESULT(device->manufacturer.status, 0x0A, 0x01, 0);
    *manufacturer = (CATALYST_UTF8) device->manufacturer.text;
    return RESULT(STATUS_CODE_SUCCESS, 0x0A, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_product(const CATDVLIB_DEVICE* device, CATALYST_UTF8* product) {
    *product = 0;
    if (device->product.status != STATUS_CODE_SUCCESS) return RESULT(device->product.status, 0x0B, 0x01, 0);
    *product = (CATALYST_UTF8) device->product.text;
    return RESULT(STATUS_CODE_SUCCESS, 0x0B, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_descriptor(const CATDVLIB_DEVICE* device, CATALYST_UTF8* descriptor) {
    *descriptor = 0;
    if (device->descriptor.status != STATUS_CODE_SUCCESS) return RESULT(device->descriptor.status, 0x0C, 0x01, 0);
    *descriptor = (CATALYST_UTF8) device->descriptor.text;
    return RESULT(STATUS_CODE_SUCCESS, 0x0C, 0x02, 0);
}

CATALYST_RESULT catdv_device_get_revision(const CATDVLIB_DEVICE* device, CATALYST_UTF8* revision) {
    *revision = 0;
    if (device->revision.status != STATUS_CODE_SUCCESS) return RESULT(device->revision.status, 0x0D, 0x01, 0);
    *revision = (CATALYST_UTF8) device->revision.text;
    return RESULT(STATUS_CODE_SUCCESS, 0x0D, 0x02, 0);
}

CATALYST_RESULT catdv_device_has_input(const CATDVLIB_DEVICE* device, CATALYST_BOOL* supported) {
    *supported = CATALYST_FALSE;
    if (device->type != CATDVLIB_DEVICE_LOGICAL) return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0E, 0x01, 0);
    if (device->capability_status != STATUS_CODE_SUCCESS) return RESULT(device->capability_status, 0x0E, 0x02, 0);
    *supported = device->input;
    return RESULT(STATUS_CODE_SUCCESS, 0x0E, 0x03, 0);
}

CATALYST_RESULT catdv_device_has_output(const CATDVLIB_DEVICE* device, CATALYST_BOOL* supported) {
    *supported = CATALYST_FALSE;
    if (device->type != CATDVLIB_DEVICE_LOGICAL) return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x0F, 0x01, 0);
    if (device->capability_status != STATUS_CODE_SUCCESS) return RESULT(device->capability_status, 0x0F, 0x02, 0);
    *supported = device->output;
    return RESULT(STATUS_CODE_SUCCESS, 0x0F, 0x03, 0);
}

