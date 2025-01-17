/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
 * Copyright (C) 2019 Arm Ltd. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "IsoSubspace.h"
#include "MarkedBlock.h"
#include "MarkedSpace.h"
#include <wtf/Packed.h>

namespace JSC {

#if defined(__CHERI_PURE_CAPABILITY__) && !ENABLE(JSHEAP_CHERI_OFFSET_REFS)
/* CHERI pointers can't be packed */
template<typename T>
using PackedCellPtr = T*;
#else
template<typename T>
class PackedCellPtr : public PackedAlignedPtr<T, MarkedBlock::atomSize, HeapPtr<T>> {
public:
    using Base = PackedAlignedPtr<T, MarkedBlock::atomSize, HeapPtr<T>>;
    PackedCellPtr(T* pointer)
        : Base(pointer)
    {
        static_assert((sizeof(T) <= MarkedSpace::largeCutoff && std::is_final<T>::value) || isAllocatedFromIsoSubspace<T>::value, "LargeAllocation does not have 16byte alignment");
        ASSERT(!(bitwise_cast<uintptr_t>(pointer) & (16 - 1)));
    }
};
#endif

} // namespace JSC
