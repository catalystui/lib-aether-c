#ifndef DEVICE_IO_NATIVE_HPP
#define DEVICE_IO_NATIVE_HPP
#if TARGET_PLATFORM_DARWIN
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
/* Remove native Boolean macros before declaring Catalyst constants. */
#undef TRUE
#undef FALSE
#endif
#if TARGET_PLATFORM_LINUX
#include <linux/input.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <limits.h>
#endif

#include "device_io_test.hpp"

/* Independent test-only native reference. Match the exact instance and channel
 * before timing; never call Aether's private backend or synthesize hardware. */
struct NativeState {
#if TARGET_PLATFORM_DARWIN
    IOHIDDeviceRef device;
    IOHIDElementRef element;
#elif TARGET_PLATFORM_LINUX
    int fd;
    unsigned type, code;
#endif
    bool output;
    NativeState(bool isOutput) : output(isOutput) {
#if TARGET_PLATFORM_DARWIN
        device = 0; element = 0;
#elif TARGET_PLATFORM_LINUX
        fd = -1; type = code = 0;
#endif
    }
    ~NativeState() {
#if TARGET_PLATFORM_DARWIN
        if (element) CFRelease(element);
        if (device) { IOHIDDeviceClose(device, kIOHIDOptionsTypeNone); CFRelease(device); }
#elif TARGET_PLATFORM_LINUX
        if (fd >= 0) close(fd);
#endif
    }
    bool open(const CATDVLIB_DEVICE* d, const CATDVLIB_IDENTIFIER& channel) {
        CATDVLIB_IDENTIFIER id = {0, 0};
        if (catdv_device_get_identifier(d, &id).status != catalyst::STATUS_CODE_SUCCESS ||
            !valid_identifier(id) || !valid_identifier(channel)) return false;
#if TARGET_PLATFORM_DARWIN
        if (std::strcmp((const char*) id.scheme, "darwin-hid") ||
            std::strcmp((const char*) channel.scheme, "darwin-hid-cookie")) return false;
        // macOS is LP64; use the native unsigned long rather than C++11 syntax.
        unsigned long nativeRegistry = 0, nativeCookie = 0;
        if (std::sscanf((const char*) id.value, "%lx", &nativeRegistry) != 1 ||
            std::sscanf((const char*) channel.value, "%lx", &nativeCookie) != 1) return false;
        CFMutableDictionaryRef matching = IORegistryEntryIDMatching((uint64_t) nativeRegistry);
        if (!matching) return false;
        io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, matching);
        if (!service) return false;
        IOHIDDeviceRef native = IOHIDDeviceCreate(kCFAllocatorDefault, service);
        IOObjectRelease(service);
        if (!native) return false;
        if (IOHIDDeviceOpen(native, kIOHIDOptionsTypeNone) != kIOReturnSuccess) { CFRelease(native); return false; }
        device = native;
        CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, 0, kIOHIDOptionsTypeNone);
        if (!elements) return false;
        for (CFIndex j = 0; j < CFArrayGetCount(elements); ++j) {
            IOHIDElementRef e = (IOHIDElementRef) CFArrayGetValueAtIndex(elements, j);
            if ((unsigned long) IOHIDElementGetCookie(e) == nativeCookie) {
                element = e; CFRetain(element); break;
            }
        }
        CFRelease(elements);
        return element != 0;
#elif TARGET_PLATFORM_LINUX
        if (std::strcmp((const char*) id.scheme, "linux-sysfs") ||
            std::strcmp((const char*) channel.scheme, "linux-evdev") ||
            std::sscanf((const char*) channel.value, "%u:%u", &type, &code) != 2) return false;
        const char* name = std::strrchr((const char*) id.value, '/');
        if (!name || std::strncmp(name + 1, "event", 5)) return false;
        char path[PATH_MAX], sysfs[PATH_MAX], resolved[PATH_MAX];
        int n = snprintf(path, sizeof(path), "/dev/input/%s", name + 1);
        if (n < 0 || (size_t) n >= sizeof(path)) return false;
        n = snprintf(sysfs, sizeof(sysfs), "/sys/class/input/%s", name + 1);
        if (n < 0 || (size_t) n >= sizeof(sysfs) || !realpath(sysfs, resolved) ||
            std::strcmp(resolved, (const char*) id.value)) return false;
        fd = ::open(path, O_RDONLY | O_NONBLOCK);
        return fd >= 0;
#else
        (void) d; (void) channel; return false;
#endif
    }
#if CATALYST_SUPPORTS_32BIT
    bool read(bool range, bool digital, catalyst::INT* a, catalyst::INT* b) {
        *a = *b = 0;
#if TARGET_PLATFORM_DARWIN
        if (range) {
            CFIndex low = IOHIDElementGetLogicalMin(element), high = IOHIDElementGetLogicalMax(element);
            if (low < CATALYST_INT_MIN || high > CATALYST_INT_MAX || low > high) return false;
            *a = (catalyst::INT) low; *b = (catalyst::INT) high; return true;
        }
        IOHIDValueRef value = 0;
        if (IOHIDDeviceGetValue(device, element, &value) != kIOReturnSuccess || !value) return false;
        CFIndex n = IOHIDValueGetIntegerValue(value);
        if (n < CATALYST_INT_MIN || n > CATALYST_INT_MAX) return false;
        *a = digital ? (n ? 1 : 0) : (catalyst::INT) n;
        return true;
#elif TARGET_PLATFORM_LINUX
        if (digital) {
            const unsigned bitsPerWord = sizeof(unsigned long) * CHAR_BIT;
            unsigned long bits[(KEY_CNT + sizeof(unsigned long) * CHAR_BIT - 1) / (sizeof(unsigned long) * CHAR_BIT)] = {0};
            if ((!output && (type != EV_KEY || code >= KEY_CNT)) || (output && (type != EV_LED || code >= LED_CNT))) return false;
            if (ioctl(fd, output ? EVIOCGLED(sizeof(bits)) : EVIOCGKEY(sizeof(bits)), bits) < 0) return false;
            *a = (bits[code / bitsPerWord] & (1UL << (code % bitsPerWord))) ? 1 : 0; return true;
        }
        if (type != EV_ABS || code >= ABS_CNT) return false;
        struct input_absinfo info;
        if (ioctl(fd, EVIOCGABS(code), &info) < 0) return false;
        *a = range ? info.minimum : info.value;
        if (range) *b = info.maximum;
        return true;
#else
        (void) range; (void) digital; return false;
#endif
    }
#endif
};

#if CATALYST_SUPPORTS_32BIT
static inline void benchmark_native_state(const CATDVLIB_DEVICE* device, const CATDVLIB_IDENTIFIER& channel,
    bool output, bool range, bool digital, catalyst::INT expectedA, catalyst::INT expectedB, double catalystNs) {
    NativeState native(output);
    catalyst::INT a = 0, b = 0;
    if (!native.open(device, channel) || !native.read(range, digital, &a, &b) ||
        a != expectedA || b != expectedB) {
        std::printf("  Direct native reference unavailable or value changed during preflight.\n"); return;
    }
    unsigned long calls = 1;
    double samples[3] = {0, 0, 0};
    for (catalyst::BYTE sample = 0; sample < 3; ++sample) {
        for (;;) {
            std::clock_t start = std::clock();
            if (start == (std::clock_t) -1) return;
            for (unsigned long j = 0; j < calls; ++j) {
                if (!native.read(range, digital, &a, &b)) { std::printf("  Native reference interrupted.\n"); return; }
                device_io_sink = (catalyst::BYTE) (device_io_sink ^ (catalyst::BYTE) a ^ (catalyst::BYTE) b);
            }
            std::clock_t end = std::clock();
            if (end == (std::clock_t) -1 || end < start) return;
            samples[sample] = (double) (end - start) / CLOCKS_PER_SEC;
            if (sample || samples[sample] >= 0.025 || calls >= 16777216UL) break;
            calls *= 2;
        }
    }
    if (samples[0] > samples[1]) { double tmp = samples[0]; samples[0] = samples[1]; samples[1] = tmp; }
    if (samples[1] > samples[2]) { double tmp = samples[1]; samples[1] = samples[2]; samples[2] = tmp; }
    double elapsed = samples[0] > samples[1] ? samples[0] : samples[1];
    if (elapsed < 0.001 || elapsed * CLOCKS_PER_SEC < 20) { std::printf("  Direct native timing too short.\n"); return; }
    double ns = elapsed * 1e9 / calls;
    std::printf("  Direct native C API: %.1f ns/call; %.0f calls/s (%lu calls/sample)", ns, calls / elapsed, calls);
    if (catalystNs > 0) std::printf("; Catalyst C %+.2f%% vs native (includes Result and validation)", (catalystNs / ns - 1) * 100);
    std::printf("\n");
}
#endif

#if TARGET_PLATFORM_DARWIN
/* Discovery reference is deliberately narrower than a Catalyst snapshot:
 * it counts native HID devices but does not copy or validate metadata. */
static inline void benchmark_native_discovery() {
    unsigned long calls = 1;
    double elapsed = 0;
    for (;;) {
        std::clock_t start = std::clock();
        if (start == (std::clock_t) -1) return;
        for (unsigned long j = 0; j < calls; ++j) {
            IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
            if (!manager) return;
            IOHIDManagerSetDeviceMatching(manager, 0);
            CFSetRef devices = IOHIDManagerCopyDevices(manager);
            if (devices) { device_io_sink ^= (catalyst::BYTE) CFSetGetCount(devices); CFRelease(devices); }
            CFRelease(manager);
        }
        std::clock_t end = std::clock();
        if (end == (std::clock_t) -1 || end < start) return;
        elapsed = (double) (end - start) / CLOCKS_PER_SEC;
        if (elapsed >= 0.025 || calls >= 64) break;
        calls *= 2;
    }
    if (elapsed >= 0.001 && elapsed * CLOCKS_PER_SEC >= 20)
        std::printf("  Direct IOKit discovery/count only: %.1f ns/call; %.0f calls/s (not equivalent to full snapshot; no overhead ratio)\n",
            elapsed * 1e9 / calls, calls / elapsed);
}
#endif
#endif
