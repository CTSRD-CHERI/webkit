/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#pragma once

#include "FreeList.h"
#include "MarkedBlock.h"

namespace JSC {

template<typename Func>
ALWAYS_INLINE HeapCell* FreeList::allocate(const Func& slowPath)
{
    // TODO confirm this applies bounds to objects and butterflies
    unsigned remaining = m_remaining;
    if (remaining) {
        unsigned cellSize = m_cellSize;
        remaining -= cellSize;
        m_remaining = remaining;
        HeapCell *ret = bitwise_cast<HeapCell*>(m_payloadEnd - remaining - cellSize);
#ifdef __CHERI_PURE_CAPABILITY__
        ret = cheri_setboundsexact(ret, cellSize);
#endif
        return ret;
    }
    
    FreeCell* result = head();
    if (UNLIKELY(!result)) {
        HeapCell *ret = slowPath();
#ifdef __CHERI_PURE_CAPABILITY__
        ret = cheri_setboundsexact(ret, m_cellSize);
#endif
        return slowPath();
    }
    
    m_scrambledHead = result->scrambledNext;
    HeapCell *ret = bitwise_cast<HeapCell*>(result);
#ifdef __CHERI_PURE_CAPABILITY__
        ret = cheri_setboundsexact(ret, m_cellSize);
#endif
    return ret;
}

template<typename Func>
void FreeList::forEach(const Func& func) const
{
    if (m_remaining) {
        for (unsigned remaining = m_remaining; remaining; remaining -= m_cellSize)
            func(bitwise_cast<HeapCell*>(m_payloadEnd - remaining));
    } else {
        for (FreeCell* cell = head(); cell;) {
            // We can use this to overwrite free objects before destroying the free list. So, we need
            // to get next before proceeding further.
            FreeCell* next = cell->next(m_secret);
            func(bitwise_cast<HeapCell*>(cell));
            cell = next;
        }
    }
}

} // namespace JSC

