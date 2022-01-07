/*
 *  Copyright (C) 2019-2020 Arm Ltd. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#pragma once

#include <new>
#include <stdlib.h>
#include <wtf/StdLibExtras.h>
#include <wtf/ThreadingPrimitives.h>

#if USE(CONTINUOUS_ARENA)
#include <malloc_np.h>

namespace WTF {

class ContinuousArenaMalloc {
public:
    static void initialize();
    static void initializePerThread();

    static void* malloc(size_t size) {
        void *ret = tryMalloc(size);

        if (!ret) {
            CRASH();
        }

        return ret;
    }

    static void* realloc(void* p, size_t size) {
        void *ret = internalReallocate(p, size);

        if (!ret) {
            CRASH();
        }

        return ret;
    }

    static void free(void* p) {
        return internalFree(p);
    }

    static void alignedFree(void *p) {
        return internalFree(p);
    }

    static void* tryMalloc(size_t size) {
        return internalAllocateAligned(sizeof(void *), size);
    }

    static void* tryAlignedMalloc(size_t alignment, size_t size) {
        return internalAllocateAligned(alignment, size);
    }

    static void* tryRealloc(void* p, size_t size) {
        return internalReallocate(p, size);
    }

    static bool isWithin(size_t non_cap_ptr) {
#ifdef __CHERI_PURE_CAPABILITY__
        return (__builtin_cheri_address_get(s_Start) <= non_cap_ptr
                && __builtin_cheri_address_get(s_End) > non_cap_ptr);
#else
        return ((size_t) s_Start <= non_cap_ptr && (size_t) s_End > non_cap_ptr);
#endif
    }

    template<class T> ALWAYS_INLINE static T *cast(size_t non_cap_ptr) {
#ifdef __CHERI_PURE_CAPABILITY__
        static_assert(sizeof(T *) > sizeof(non_cap_ptr));
        return (T *) capabilityFromPointer(non_cap_ptr);
#else
        static_assert(sizeof(T *) == sizeof(non_cap_ptr));
        return (T *) non_cap_ptr;
#endif
    }

    template<class T> ALWAYS_INLINE static size_t cast(T *ptr) {
        size_t ret;

        ret = (size_t) (uintptr_t) ptr;

        ASSERT(ptr == cast<T>(ret));

        return ret;
    }

    ALWAYS_INLINE static size_t cast(nullptr_t) {
        return 0;
    }


private:
#ifdef __CHERI_PURE_CAPABILITY__
    ALWAYS_INLINE static void* capabilityFromPointer(size_t non_cap_ptr)
    {
      ASSERT(s_Initialized);

      char *ddc_cap;
      asm("mrs %0, ddc" : "=C"(ddc_cap));
      ASSERT(ddc_cap == s_Start);

      ASSERT(non_cap_ptr == 0 || __builtin_cheri_address_get(ddc_cap) <= non_cap_ptr);
      ASSERT(non_cap_ptr == 0 || __builtin_cheri_address_get(s_End) > non_cap_ptr);

      return __builtin_cheri_offset_increment(ddc_cap,
                   non_cap_ptr - __builtin_cheri_address_get(ddc_cap));
    }
#endif

    static void* internalAllocateAligned(size_t alignment, size_t size);
    static void* internalReallocate(void *p, size_t size);
    static void internalFree(void* ptr);

    // True iff [addr, addr+size) is a subset of or equal to [s_Start, s_End).
    static bool isValidRange(void *addr, size_t size);
    // True iff [addr, addr+size) is a subset of or equal to [s_Start, s_Current).
    static bool isAllocatedRange(void *addr, size_t size);
    // True iff [addr, addr+size) is a subset of or equal to [s_Current, s_End).
    static bool isAvailableRange(void *addr, size_t size);

    static void* extentAlloc(extent_hooks_t *extent_hooks,
                             void *new_addr,
                             size_t size,
                             size_t alignment,
                             bool *zero,
                             bool *commit,
                             unsigned arena_ind);
    static void extentDestroy(extent_hooks_t *extent_hooks,
                              void *addr,
                              size_t size,
                              bool committed,
                              unsigned arena_ind);
    static bool extentPurgeCommon(extent_hooks_t *extent_hooks,
                                  void *addr,
                                  size_t size,
                                  size_t offset,
                                  size_t length,
                                  unsigned arena_ind);
    static bool extentPurgeLazy(extent_hooks_t *extent_hooks,
                                void *addr,
                                size_t size,
                                size_t offset,
                                size_t length,
                                unsigned arena_ind);
    static bool extentPurgeForced(extent_hooks_t *extent_hooks,
                                  void *addr,
                                  size_t size,
                                  size_t offset,
                                  size_t length,
                                  unsigned arena_ind);

    static bool s_Initialized;
    static unsigned int s_arenaIndex;
    static extent_hooks_t s_extentHooks;
    static char *s_Start;
    static char *s_End;
    static char *s_Current;
    static Mutex *s_Mutex;

    static constexpr size_t k_LgOneGigabyte = 30;
    static constexpr size_t k_LgAreaSize = (sizeof(size_t) == 4 ? 1 : 6) + k_LgOneGigabyte;
    static constexpr size_t k_AreaSize = (size_t) 1 << k_LgAreaSize;
};

} // namespace WTF

#endif
