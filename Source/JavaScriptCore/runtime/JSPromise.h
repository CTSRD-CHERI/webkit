/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#include "JSInternalFieldObjectImpl.h"

namespace JSC {

class JSPromiseConstructor;
class JSPromise : public JSInternalFieldObjectImpl<2> {
public:
    using Base = JSInternalFieldObjectImpl<2>;

    JS_EXPORT_PRIVATE static JSPromise* create(VM&, Structure*);
    static Structure* createStructure(VM&, JSGlobalObject*, JSValue);

    DECLARE_EXPORT_INFO;

    enum class Status : unsigned {
        Pending = 0, // Making this as 0, so that, we can change the status from Pending to others without masking.
        Fulfilled = 1,
        Rejected = 2,
    };
    static constexpr uint32_t isHandledFlag = 4;
    static constexpr uint32_t isFirstResolvingFunctionCalledFlag = 8;
    static constexpr uint32_t stateMask = 0b11;

    enum class Field : unsigned {
        Flags = 0,
        ReactionsOrResult = 1,
    };
    static_assert(numberOfInternalFields == 2);

    JS_EXPORT_PRIVATE Status status(VM&) const;
    JS_EXPORT_PRIVATE JSValue result(VM&) const;
    JS_EXPORT_PRIVATE bool isHandled(VM&) const;

    JS_EXPORT_PRIVATE static JSPromise* resolvedPromise(JSGlobalObject*, JSValue);

    JS_EXPORT_PRIVATE void resolve(JSGlobalObject*, JSValue);
    JS_EXPORT_PRIVATE void reject(JSGlobalObject*, JSValue);
    JS_EXPORT_PRIVATE void reject(JSGlobalObject*, Exception*);

    struct DeferredData {
        WTF_FORBID_HEAP_ALLOCATION;
    public:
        JSPromise* promise { nullptr };
        JSFunction* resolve { nullptr };
        JSFunction* reject { nullptr };
    };
    static DeferredData createDeferredData(JSGlobalObject*, JSPromiseConstructor*);

    static void visitChildren(JSCell*, SlotVisitor&);

protected:
    JSPromise(VM&, Structure*);
    void finishCreation(VM&);

    uint32_t flags() const;
};

} // namespace JSC
