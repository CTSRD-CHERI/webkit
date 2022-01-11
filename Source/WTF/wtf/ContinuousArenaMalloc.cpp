/*
 * Copyright (C) 2019-2020 Arm Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include <wtf/ContinuousArenaMalloc.h>

#include <sys/mman.h>
#include <cheriintrin.h>

#if USE(CONTINUOUS_ARENA)

namespace WTF {

bool ContinuousArenaMalloc::s_Initialized;
unsigned int ContinuousArenaMalloc::s_arenaIndex;
extent_hooks_t ContinuousArenaMalloc::s_extentHooks;
char *ContinuousArenaMalloc::s_Start;
char *ContinuousArenaMalloc::s_End;
char *ContinuousArenaMalloc::s_Current;
Mutex *ContinuousArenaMalloc::s_Mutex;

void ContinuousArenaMalloc::initialize(void) {
    ASSERT(!s_Initialized);
    ASSERT(s_Mutex == NULL);

    s_Mutex = new Mutex();

    void *area_start = mmap(NULL, k_AreaSize,
                            PROT_READ | PROT_WRITE,
                            MAP_ANON | MAP_PRIVATE | MAP_ALIGNED(k_LgAreaSize),
                            -1, 0);

    ASSERT(area_start != MAP_FAILED);

    void *area_start_remapped = mmap(area_start, k_AreaSize,
                                     PROT_NONE,
                                     MAP_GUARD | MAP_FIXED,
                                     -1, 0);

    ASSERT(area_start == area_start_remapped);

    LOG_CHERI("initialize() - reserved %zu bytes starting from %p\n",
              k_AreaSize, area_start);

    s_Start = (char *) area_start;
    s_End = s_Start + k_AreaSize;
    s_Current = s_Start;

    int mallctlRet;

    /* the extent functions might be invoked
     * before the initialization is finished,
     * during the arenas.create operation */
    s_extentHooks.alloc = extentAlloc;
    s_extentHooks.dalloc = NULL;  // Opt out.
    s_extentHooks.destroy = extentDestroy;
    s_extentHooks.commit = NULL;  // Opt out.
    s_extentHooks.decommit = NULL;  // Opt out.
    s_extentHooks.purge_lazy = extentPurgeLazy;
    s_extentHooks.purge_forced = extentPurgeForced;
    s_extentHooks.split = NULL;  // Opt out.
    s_extentHooks.merge = NULL;  // Opt out.

    extent_hooks_t *newHooksPtr = &s_extentHooks;
    size_t indexSize = sizeof(s_arenaIndex);

    /* arenas.create invokes extentAlloc, therefore
     * s_Start, s_End, s_Current and s_Mutex should
     * already be initialized at this point */

    mallctlRet = mallctl("arenas.create",
                         &s_arenaIndex,
                         &indexSize,
                         &newHooksPtr,
                         sizeof(extent_hooks_t *));

    ASSERT(mallctlRet == 0);

    s_Initialized = true;
}

void ContinuousArenaMalloc::initializePerThread()
{
    ASSERT(s_Initialized);
#ifdef __CHERI_PURE_CAPABILITY__
    asm volatile("msr ddc, %0" :: "C"(s_Start));
#endif
}

void *ContinuousArenaMalloc::internalAllocateAligned(size_t alignment,
                                            size_t size)
{
    ASSERT((alignment & (alignment - 1)) == 0);
    ASSERT(s_Initialized);

    return mallocx(size, MALLOCX_ALIGN(alignment) | MALLOCX_TCACHE_NONE | MALLOCX_ARENA(s_arenaIndex));
}

void* ContinuousArenaMalloc::internalReallocate(void* ptr, size_t size)
{
    ASSERT(s_Initialized);
    return rallocx(ptr, size, MALLOCX_TCACHE_NONE | MALLOCX_ARENA(s_arenaIndex));
}

void ContinuousArenaMalloc::internalFree(void* ptr)
{
    ASSERT(s_Initialized);
    dallocx(ptr, MALLOCX_TCACHE_NONE);
}

bool ContinuousArenaMalloc::isValidRange(void *addr, size_t size)
{
    ASSERT(s_Mutex->tryLock() == false);

    ASSERT(s_Start != NULL);
    ASSERT(s_Current >= s_Start);
    ASSERT(s_End >= s_Current);

    char* start = reinterpret_cast<char*>(addr);
    char* end = start + size;
    return (start >= s_Start) && (start <= s_End) && (end >= s_Start) && (end <= s_End);
}

bool ContinuousArenaMalloc::isAllocatedRange(void *addr, size_t size)
{
    char* end = reinterpret_cast<char*>(addr) + size;
    return isValidRange(addr, size) && (end <= s_End);
}

bool ContinuousArenaMalloc::isAvailableRange(void *addr, size_t size)
{
    char* start = reinterpret_cast<char*>(addr);
    return isValidRange(addr, size) && (start >= s_Current);
}

void* ContinuousArenaMalloc::extentAlloc(extent_hooks_t *extent_hooks,
                                         void *new_addr,
                                         size_t size,
                                         size_t alignment,
                                         bool *zero,
                                         bool *commit,
                                         unsigned arena_ind)
{
    MutexLocker locker(s_Mutex);

    LOG_CHERI("alloc(%p, %p, %zu, %zu, %c, %c, %u) = ",
              extent_hooks,
              new_addr,
              size,
              alignment,
              *zero ? 'T' : 'F',
              *commit ? 'T' : 'F',
              arena_ind);

    void *ret;

    if (new_addr != NULL || size == 0) {
        ret = NULL;
    } else {
        // The masks have all bits set except for zero or more low-order bits,
        // such that `& mask` aligns down to a multiple of a power of two.
        ASSERT(hasOneBitSet(alignment));
        size_t align_mask = -alignment;
        size_t repr_mask = cheri_representable_alignment_mask(size);
        size_t repr_size = cheri_representable_length(size);

        ASSERT(hasZeroOrOneBitsSet(~repr_mask + 1));
        ASSERT(repr_size >= size);

        // We need to align up, not down, so we don't hand out memory that's
        // already allocated.
        size_t mask = align_mask & repr_mask;
        size_t start_addr = (cheri_address_get(s_Current) + ~mask) & mask;
        void *start = cheri_address_set(s_Current, start_addr);

        if (isAvailableRange(start, repr_size)) {
            ret = mmap(start,
                       repr_size,
                       PROT_READ | PROT_WRITE,
                       MAP_ANON | MAP_PRIVATE | MAP_FIXED,
                       -1, 0);

            if (ret == MAP_FAILED) {
                ret = NULL;
            } else {
#ifdef __CHERI_PURE_CAPABILITY__
                // We checked representability, so this should be exact.
                ASSERT(cheri_address_get(ret) == cheri_address_get(start));
                ASSERT(cheri_length_get(ret) == repr_size);
#endif

                *zero = true;
                *commit = true;

                s_Current = reinterpret_cast<char*>(start) + cheri_length_get(ret);
            }
        } else {
            ret = NULL;
        }
    }

    LOG_CHERI("%p\n", ret);

    return ret;
}

void ContinuousArenaMalloc::extentDestroy(extent_hooks_t *extent_hooks,
                                          void *addr,
                                          size_t size,
                                          bool committed,
                                          unsigned arena_ind)
{
    MutexLocker locker(s_Mutex);

    LOG_CHERI("destroy(%p, %p, %zu, %c, %u)\n",
              extent_hooks,
              addr,
              size,
              committed ? 'T' : 'F',
              arena_ind);

    ASSERT(isAllocatedRange(addr, size));

    void *ret = mmap(addr, size, PROT_NONE, MAP_GUARD | MAP_FIXED, -1, 0);

    ASSERT(ret == addr);
}

bool ContinuousArenaMalloc::extentPurgeCommon(extent_hooks_t *extent_hooks,
                                              void *addr,
                                              size_t size,
                                              size_t offset,
                                              size_t length,
                                              unsigned arena_ind)
{
    UNUSED_PARAM(extent_hooks);
    UNUSED_PARAM(arena_ind);

    ASSERT(s_Mutex->tryLock() == false);

    ASSERT(isAllocatedRange(addr, size));

    char *start = reinterpret_cast<char *>(addr) + offset;
    ASSERT(start >= reinterpret_cast<char *>(addr));
    ASSERT((start + length) <= (reinterpret_cast<char *>(addr) + size));
    ASSERT(isAllocatedRange(start, length));

    void *ret = mmap(start,
                     length,
                     PROT_READ | PROT_WRITE,
                     MAP_ANON | MAP_PRIVATE | MAP_FIXED,
                     -1, 0);

    ASSERT(ret == start);

    return false;
}

bool ContinuousArenaMalloc::extentPurgeLazy(extent_hooks_t *extent_hooks,
                                            void *addr,
                                            size_t size,
                                            size_t offset,
                                            size_t length,
                                            unsigned arena_ind)
{
    MutexLocker locker(s_Mutex);

    LOG_CHERI("purge_lazy(%p, %p, %zu, %zu, %zu, %u)\n",
              extent_hooks,
              addr,
              size,
              offset,
              length,
              arena_ind);

    // For simplicity, just force all purges.
    return extentPurgeCommon(extent_hooks, addr, size, offset, length, arena_ind);
}

bool ContinuousArenaMalloc::extentPurgeForced(extent_hooks_t *extent_hooks,
                                              void *addr,
                                              size_t size,
                                              size_t offset,
                                              size_t length,
                                              unsigned arena_ind)
{
    MutexLocker locker(s_Mutex);

    LOG_CHERI("purge_forced(%p, %p, %zu, %zu, %zu, %u)\n",
              extent_hooks,
              addr,
              size,
              offset,
              length,
              arena_ind);

    return extentPurgeCommon(extent_hooks, addr, size, offset, length, arena_ind);
}

} // namespace WTF

#endif // USE(CONTINUOUS_ARENA)
