/*
 * Copyright (C) 2015-2018 Apple Inc. All rights reserved.
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

#pragma once

#if ENABLE(JIT)

#include "CallEdge.h"
#include "CallVariant.h"
#include "GCAwareJITStubRoutine.h"
#include <wtf/Noncopyable.h>
#include <wtf/UniqueArray.h>
#include <wtf/Vector.h>

namespace JSC {

class CallLinkInfo;

class PolymorphicCallNode
#ifdef __CHERI_PURE_CAPABILITY__
    : public BasicRawSentinelNode<PolymorphicCallNode> {
#else
    : public PackedRawSentinelNode<PolymorphicCallNode> {
#endif
    WTF_MAKE_NONCOPYABLE(PolymorphicCallNode);
public:
    PolymorphicCallNode(CallLinkInfo* info)
        : m_callLinkInfo(info)
    {
    }
    
    ~PolymorphicCallNode();
    
    void unlink(VM&);

    bool hasCallLinkInfo(CallLinkInfo* info) { return m_callLinkInfo.get() == info; }
    void clearCallLinkInfo();
    
private:
#ifdef __CHERI_PURE_CAPABILITY__
    PlainPtr<CallLinkInfo> m_callLinkInfo;
#else
    PackedPtr<CallLinkInfo> m_callLinkInfo;
#endif
};

class PolymorphicCallCase {
public:
    PolymorphicCallCase()
        : m_codeBlock(nullptr)
    {
    }
    
    PolymorphicCallCase(CallVariant variant, CodeBlock* codeBlock)
        : m_variant(variant)
        , m_codeBlock(codeBlock)
    {
    }
    
    CallVariant variant() const { return m_variant; }
    CodeBlock* codeBlock() const { return m_codeBlock; }
    
    void dump(PrintStream&) const;
    
private:
    CallVariant m_variant;
    CodeBlock* m_codeBlock;
};

class PolymorphicCallStubRoutine : public GCAwareJITStubRoutine {
public:
    PolymorphicCallStubRoutine(
        const MacroAssemblerCodeRef<JITStubRoutinePtrTag>&, VM&, const JSCell* owner,
        CallFrame* callerFrame, CallLinkInfo&, const Vector<PolymorphicCallCase>&,
        UniqueArray<uint32_t>&& fastCounts);
    
    virtual ~PolymorphicCallStubRoutine();
    
    CallVariantList variants() const;
    bool hasEdges() const;
    CallEdgeList edges() const;

    void clearCallNodesFor(CallLinkInfo*);
    
    template<typename Functor>
    void forEachDependentCell(const Functor& functor)
    {
        for (auto& variant : m_variants)
            functor(variant.get());
    }

    bool visitWeak(VM&) override;

protected:
    void markRequiredObjectsInternal(SlotVisitor&) override;

private:
    Vector<WriteBarrier<JSCell>, 2> m_variants;
    UniqueArray<uint32_t> m_fastCounts;
    Bag<PolymorphicCallNode> m_callNodes;
};

} // namespace JSC

#endif // ENABLE(JIT)
