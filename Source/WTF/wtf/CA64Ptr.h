/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#include <wtf/ContinuousArenaMalloc.h>

#if USE(CONTINUOUS_ARENA)

namespace WTF {

// The purpose of this class is to provide continuous arena pointer with
// behavior transparent regarding whether CHERI is enabled or not - the pointer
// should anyway be 64-bit and be dereferenceable.

template <typename T> class CA64Ptr {
    WTF_MAKE_FAST_ALLOCATED;
public:
    typedef uint64_t integer_t;

    CA64Ptr() =default;
    ALWAYS_INLINE CA64Ptr(T* ptr) {
        *this = ptr;
    }

    T* get() const {
#ifdef __CHERI_PURE_CAPABILITY__
        return WTF::ContinuousArenaMalloc::cast<T>(m_ptrAsInt64);
#else
        return reinterpret_cast<void *>(m_ptrAsInt64);
#endif
    }

    T* tryGet() const {
#ifdef __CHERI_PURE_CAPABILITY__
        if (WTF::ContinuousArenaMalloc::isWithin(m_ptrAsInt64)) {
            return WTF::ContinuousArenaMalloc::cast<T>(m_ptrAsInt64);
        } else {
            return nullptr;
        }
#else
        return get();
#endif
    }


    void clear() {
#ifdef __CHERI_PURE_CAPABILITY__
        m_ptrAsInt64 = WTF::ContinuousArenaMalloc::cast(static_cast<void *>(nullptr));
#else
        m_ptrAsInt64 = reinterpret_cast<uint64_t>(nullptr);
#endif
    }

    T& operator*() const { ASSERT(m_ptrAsInt64); return *get(); }
    ALWAYS_INLINE T* operator->() const { return get(); }

    operator T*() const { return get(); }

    bool operator!() const { return !m_ptrAsInt64; }

    explicit operator bool() const { return !!m_ptrAsInt64; }

    CA64Ptr& operator=(T*);

    void swap(CA64Ptr&);

private:
    uint64_t m_ptrAsInt64;
};

template<typename T> inline CA64Ptr<T>& CA64Ptr<T>::operator=(T* optr)
{
    m_ptrAsInt64 = (uint64_t) (uintptr_t) optr;
    return *this;
}

template<class T> inline void CA64Ptr<T>::swap(CA64Ptr& o)
{
    std::swap(m_ptrAsInt64, o.m_ptrAsInt64);
}

template<class T> inline void swap(CA64Ptr<T>& a, CA64Ptr<T>& b)
{
    a.swap(b);
}

} // namespace WTF

using WTF::CA64Ptr;

#endif
