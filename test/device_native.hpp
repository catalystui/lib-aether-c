#ifndef DEVICE_NATIVE_HPP
#define DEVICE_NATIVE_HPP
#include "device_test.hpp"

#if TARGET_PLATFORM_DARWIN
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
/* Keep native Boolean macros out of the benchmark callers. */
#undef TRUE
#undef FALSE
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
