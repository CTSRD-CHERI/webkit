/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 * Copyright (C) 2017-2018 Apple Inc. All rights reserved.
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

#include "config.h"

#include <wtf/CA64Ptr.h>
#include <wtf/PlainPtr.h>

namespace JSC {

template<typename T>
#if defined(__CHERI_PURE_CAPABILITY__) && ENABLE(JSHEAP_CHERI_OFFSET_REFS)
    using HeapPtr = CA64Ptr<T>;
#else /* __CHERI_PURE_CAPABILITY__ && !ENABLE(JSHEAP_CHERI_OFFSET_REFS) */
    using HeapPtr = PlainPtr<T>;
#endif /* __CHERI_PURE_CAPABILITY__ && !ENABLE(JSHEAP_CHERI_OFFSET_REFS) */

template<typename T>
struct HeapPtrTraits {
    template<typename U> using RebindTraits = HeapPtrTraits<U>;

    using StorageType = HeapPtr<T>;

    template<typename U>
    static ALWAYS_INLINE StorageType exchange(StorageType& ptr, U&& newValue) {
        StorageType newValueAsStorageType = std::move(newValue);
        return std::exchange(ptr, newValueAsStorageType);
    }

    static ALWAYS_INLINE void swap(StorageType& a, StorageType& b) { std::swap(a, b); }
    static ALWAYS_INLINE T* unwrap(const StorageType& ptr) { return ptr; }
};

} // namespace JSC
