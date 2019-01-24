#pragma once

#include <atomic>
#include <thread>

class MemoryProfiler {

public:
    static void recordJsStackGrow(size_t n) { jsStackBytes += n; }
    static void recordJsStackShrink(size_t n) { jsStackBytes -= n; }
    static void recordMmap(size_t n) { mmapBytes += n; }
    static void recordMunmap(size_t n) { mmapBytes -= n; }

    static void start() {
        if (!shouldRun.exchange(true))
            outputThread = std::thread(outputStats);
    }

    static void stop() {
        if (shouldRun.exchange(false))
            outputThread.join();
    }

protected:
    static void outputStats(void);

    static std::thread outputThread; // only accessed by main thread
    static std::atomic_bool shouldRun;
    static std::atomic_ulong jsStackBytes;
    static std::atomic_ulong mmapBytes;
};

// C API
extern "C" {
void MemoryProfiler_record_js_stack_grow(size_t n);
void MemoryProfiler_record_js_stack_shrink(size_t n);
void MemoryProfiler_record_mmap(size_t n);
void MemoryProfiler_record_munmap(size_t n);
}