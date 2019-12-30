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

namespace WTF {

template <typename T> class PlainPtr {
    WTF_MAKE_FAST_ALLOCATED;
public:
    typedef uintptr_t integer_t;

    PlainPtr() =default;
    ALWAYS_INLINE PlainPtr(T* ptr) {
        *this = ptr;
    }

    T* get() const { return m_ptr; }
    T* tryGet() const { return get(); }

    void clear() { m_ptr = nullptr; }

    T& operator*() const { ASSERT(m_ptr); return *get(); }
    ALWAYS_INLINE T* operator->() const { return get(); }

    operator T*() const { return get(); }

    bool operator!() const { return !m_ptr; }

    explicit operator bool() const { return !!m_ptr; }

    PlainPtr& operator=(T*);

    void swap(PlainPtr&);

private:
    T *m_ptr;
};

template<typename T> inline PlainPtr<T>& PlainPtr<T>::operator=(T* optr)
{
    m_ptr = optr;
    return *this;
}

template<class T> inline void PlainPtr<T>::swap(PlainPtr& o)
{
    std::swap(m_ptr, o.m_ptr);
}

template<class T> inline void swap(PlainPtr<T>& a, PlainPtr<T>& b)
{
    a.swap(b);
}

} // namespace WTF

using WTF::PlainPtr;
