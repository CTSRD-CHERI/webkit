/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "WasmName.h"
#include "WasmNameSection.h"
#include <wtf/RefPtr.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/WTFString.h>

namespace JSC { namespace Wasm {

struct NameSection;

// Keep this class copyable when the world is stopped: do not allocate any memory while copying this.
// SamplingProfiler copies it while suspending threads.
struct IndexOrName {
    typedef size_t Index;

    IndexOrName() { m_indexName.index = emptyTag; }
    IndexOrName(Index, std::pair<const Name*, RefPtr<NameSection>>&&);
    bool isEmpty() const { return m_indexName.index == emptyTag; }
    bool isIndex() const { return m_indexName.index & indexTag; }
    bool isName() const { return !(isEmpty() || isName()); }
    NameSection* nameSection() const { return m_nameSection.get(); }

    friend String makeString(const IndexOrName&);

private:
    union {
        Index index;
        const Name* name;
    } m_indexName;
    RefPtr<NameSection> m_nameSection;

    static constexpr unsigned indexTag = 1;
    static constexpr unsigned emptyTag = 2;
    static constexpr unsigned indexShift = 2;
    static constexpr unsigned allTags = indexTag | emptyTag;
};

String makeString(const IndexOrName&);

} } // namespace JSC::Wasm
