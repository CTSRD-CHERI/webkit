/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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

#include "CellSize.h"
#include "VMInspector.h"
#include <wtf/Assertions.h>

namespace JSC {

#define AUDIT_CONDITION(x) (x), #x
#define AUDIT_VERIFY(action, verifier, cond, ...) do { \
        if (action == VerifierAction::ReleaseAssert) \
            RELEASE_ASSERT(cond, __VA_ARGS__); \
        else if (!verifier(AUDIT_CONDITION(cond), __VA_ARGS__)) \
            return false; \
    } while (false)

template<VMInspector::VerifierAction action, VMInspector::VerifyFunctor verifier>
bool VMInspector::verifyCellSize(VM& vm, JSCell* cell, size_t allocatorCellSize)
{
    Structure* structure = cell->structure(vm);
    const ClassInfo* classInfo = structure->classInfo();
    JSType cellType = cell->type();
    AUDIT_VERIFY(action, verifier, cellType == structure->m_blob.type(), cell, cellType, structure->m_blob.type());

    size_t size = cellSize(vm, cell);
    AUDIT_VERIFY(action, verifier, size <= allocatorCellSize, cell, cellType, size, allocatorCellSize, classInfo->staticClassSize);
    if (isDynamicallySizedType(cellType))
        AUDIT_VERIFY(action, verifier, size >= classInfo->staticClassSize, cell, cellType, size, classInfo->staticClassSize);

    return true;
}

template<VMInspector::VerifierAction action, VMInspector::VerifyFunctor verifier>
bool VMInspector::verifyCell(VM& vm, JSCell* cell)
{
    size_t allocatorCellSize = 0;
    if (cell->isLargeAllocation()) {
        LargeAllocation& largeAllocation = cell->largeAllocation();
        AUDIT_VERIFY(action, verifier, &largeAllocation.vm() == &vm, cell, cell->type(), &largeAllocation.vm(), &vm);

        bool isValidLargeAllocation = false;
        for (auto* i : vm.heap.objectSpace().largeAllocations()) {
            if (i == &largeAllocation) {
                isValidLargeAllocation = true;
                break;
            }
        }
        AUDIT_VERIFY(action, verifier, isValidLargeAllocation, cell, cell->type());

        allocatorCellSize = largeAllocation.cellSize();
    } else {
        MarkedBlock& block = cell->markedBlock();
        MarkedBlock::Handle& blockHandle = block.handle();
        AUDIT_VERIFY(action, verifier, &block.vm() == &vm, cell, cell->type(), &block.vm(), &vm);

        uintptr_t blockStartAddress = reinterpret_cast<uintptr_t>(blockHandle.start());
        AUDIT_VERIFY(action, verifier, blockHandle.contains(cell), cell, cell->type(), blockStartAddress, blockHandle.end());

        uintptr_t cellAddress = reinterpret_cast<uintptr_t>(cell);
        ptrdiff_t cellOffset = cellAddress - static_cast<ptraddr_t>(blockStartAddress);
        allocatorCellSize = block.cellSize();
        bool cellIsProperlyAligned = !(cellOffset % allocatorCellSize);
        AUDIT_VERIFY(action, verifier, cellIsProperlyAligned, cell, cell->type(), allocatorCellSize);
    }

    auto cellType = cell->type();
    if (cell->type() != JSImmutableButterflyType)
        AUDIT_VERIFY(action, verifier, !Gigacage::contains(cell), cell, cellType);

    if (!verifyCellSize<action, verifier>(vm, cell, allocatorCellSize))
        return false;

    if (Gigacage::isEnabled(Gigacage::JSValue) && cell->isObject()) {
        JSObject* object = asObject(cell);
        const Butterfly* butterfly = object->butterfly();
        AUDIT_VERIFY(action, verifier, !butterfly || Gigacage::isCaged(Gigacage::JSValue, butterfly), cell, cell->type(), butterfly);
    }

    return true;
}

#undef AUDIT_VERIFY
#undef AUDIT_CONDITION

} // namespace JSC
