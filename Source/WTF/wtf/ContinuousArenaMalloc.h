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

#if __has_feature(capabilities)
#include <cheri/cheric.h>
#endif

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

    static void* rederive(void* p) {
#if defined(__CHERI_PURE_CAPABILITY__)
        if (cheri_gettag(p) && cheri_is_address_inbounds(p, cheri_getaddress(p)) &&
            isWithin(cheri_getaddress(p)))
        {
            return cheri_setaddress(s_Start, cheri_getaddress(p));
        }
#endif
        return p;
    }

#if defined(__CHERI_PURE_CAPABILITY__) && ENABLE(JSHEAP_CHERI_OFFSET_REFS)
    template<class T> ALWAYS_INLINE static T *cast(size_t non_cap_ptr) {
        ASSERT(s_Initialized);

        if (non_cap_ptr == 0) {
            return nullptr;
        }

        char *ddc_cap;
        asm("mrs %0, ddc" : "=C"(ddc_cap));
        ASSERT(ddc_cap == s_Start);

        ASSERT(cheri_getaddress(s_Start) <= non_cap_ptr);
        ASSERT(cheri_getaddress(s_End) > non_cap_ptr);

        return reinterpret_cast<T *>(cheri_setaddress(s_Start, non_cap_ptr));
    }
#endif

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
    static void* internalAllocateAligned(size_t alignment, size_t size);
    static void* internalReallocate(void *p, size_t size);
    static void internalFree(void* ptr);

    static bool isValidRange(void *addr, size_t size);
    static void* extentAlloc(extent_hooks_t *extent_hooks,
                             void *new_addr,
                             size_t size,
                             size_t alignment,
                             bool *zero,
                             bool *commit,
                             unsigned arena_ind);
    static bool extentDalloc(extent_hooks_t *extent_hooks,
                             void *addr,
                             size_t size,
                             bool committed,
                             unsigned arena_ind);
    static void extentDestroy(extent_hooks_t *extent_hooks,
                              void *addr,
                              size_t size,
                              bool committed,
                              unsigned arena_ind);
    static bool extentCommit(extent_hooks_t *extent_hooks,
                             void *addr,
                             size_t size,
                             size_t offset,
                             size_t length,
                             unsigned arena_ind);
    static bool extentDecommit(extent_hooks_t *extent_hooks,
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
    static bool extentSplit(extent_hooks_t *extent_hooks,
                            void *addr,
                            size_t size,
                            size_t size_a,
                            size_t size_b,
                            bool committed,
                            unsigned arena_ind);
    static bool extentMerge(extent_hooks_t *extent_hooks,
                            void *addr_a,
                            size_t size_a,
                            void *addr_b,
                            size_t size_b,
                            bool committed,
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
