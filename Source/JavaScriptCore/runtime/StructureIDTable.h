/*
 * Copyright (C) 2013-2019 Apple Inc. All rights reserved.
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

#include "HeapPtr.h"
#include "UnusedPointer.h"
#include <wtf/UniqueArray.h>
#include <wtf/Vector.h>
#include <wtf/WeakRandom.h>

#if defined(__CHERI_PURE_CAPABILITY__) && !ENABLE(JSHEAP_CHERI_OFFSET_REFS)
#define ENCODE_STRUCTURE_BITS 0
#else
#define ENCODE_STRUCTURE_BITS 1
#endif

namespace JSC {

class Structure;

#if USE(JSVALUE64)
typedef uint32_t StructureID;

inline StructureID nukedStructureIDBit()
{
    return 0x80000000u;
}

inline StructureID nuke(StructureID id)
{
    return id | nukedStructureIDBit();
}

inline bool isNuked(StructureID id)
{
    return !!(id & nukedStructureIDBit());
}

inline StructureID decontaminate(StructureID id)
{
    return id & ~nukedStructureIDBit();
}
#else // not USE(JSVALUE64)
typedef Structure* StructureID;

inline StructureID nukedStructureIDBit()
{
    return bitwise_cast<StructureID>(static_cast<uintptr_t>(1));
}

inline StructureID nuke(StructureID id)
{
    return bitwise_cast<StructureID>(bitwise_cast<uintptr_t>(id) | bitwise_cast<uintptr_t>(nukedStructureIDBit()));
}

inline bool isNuked(StructureID id)
{
    return !!(bitwise_cast<uintptr_t>(id) & bitwise_cast<uintptr_t>(nukedStructureIDBit()));
}

inline StructureID decontaminate(StructureID id)
{
    return bitwise_cast<StructureID>(bitwise_cast<uintptr_t>(id) & ~bitwise_cast<uintptr_t>(nukedStructureIDBit()));
}
#endif // not USE(JSVALUE64)

#if USE(JSVALUE64)

#if ENCODE_STRUCTURE_BITS
using EncodedStructureBits = size_t;
#endif

class StructureIDTable {
    friend class LLIntOffsetsExtractor;
public:
    StructureIDTable();

    void** base() { return reinterpret_cast<void**>(&m_table); }

    bool isValid(StructureID);
    Structure* get(StructureID);
    void deallocateID(Structure*, StructureID);
    StructureID allocateID(Structure*);

    void flushOldTables();
    
    size_t size() const { return m_size; }

private:
    void resize(size_t newCapacity);
    void makeFreeListFromRange(uint32_t first, uint32_t last);

    union StructureOrOffset {
        WTF_MAKE_FAST_ALLOCATED;
    public:
#if ENCODE_STRUCTURE_BITS
        EncodedStructureBits encodedStructureBits;
#else
        Structure* structure;
#endif
        StructureID offset;
    };

    StructureOrOffset* table() const { return m_table.get(); }
#if ENCODE_STRUCTURE_BITS
    static Structure* decode(EncodedStructureBits, StructureID);
    static EncodedStructureBits encode(Structure*, StructureID);
#endif

    static constexpr size_t s_initialSize = 512;

    Vector<UniqueArray<StructureOrOffset>> m_oldTables;

    uint32_t m_firstFreeOffset { 0 };
    uint32_t m_lastFreeOffset { 0 };
    UniqueArray<StructureOrOffset> m_table;

    size_t m_size { 0 };
    size_t m_capacity;

    WeakRandom m_weakRandom;

    static constexpr StructureID s_unusedID = 0;

public:
    // 1. StructureID is encoded as:
    //
    //    ----------------------------------------------------------------
    //    | 1 Nuke Bit | 24 StructureIDTable index bits | 7 entropy bits |
    //    ----------------------------------------------------------------
    //
    //    The entropy bits are chosen at random and assigned when a StructureID
    //    is allocated.
    //
    // 2. For each StructureID, the StructureIDTable stores encodedStructureBits
    //    which are encoded from the structure pointer as such:
    //
    //    ----------------------------------------------------------------
    //    | 7 entropy bits |                   57 structure pointer bits |
    //    ----------------------------------------------------------------
    //
    //    The entropy bits here are the same 7 bits used in the encoding of the
    //    StructureID for this structure entry in the StructureIDTable.

    static constexpr uint32_t s_numberOfNukeBits = 1;
    static constexpr uint32_t s_numberOfEntropyBits = 7;
#if ENCODE_STRUCTURE_BITS
    static constexpr uint32_t s_entropyBitsShiftForStructurePointer = (sizeof(EncodedStructureBits) * 8) - s_numberOfEntropyBits;
#endif

    static constexpr uint32_t s_maximumNumberOfStructures = 1 << (32 - s_numberOfEntropyBits - s_numberOfNukeBits);
};

#if ENCODE_STRUCTURE_BITS
ALWAYS_INLINE Structure* StructureIDTable::decode(EncodedStructureBits bits, StructureID structureID)
{
    return bitwise_cast< HeapPtr<Structure> >(bits ^ (static_cast<EncodedStructureBits>(structureID) << s_entropyBitsShiftForStructurePointer));
}

ALWAYS_INLINE EncodedStructureBits StructureIDTable::encode(Structure* structure, StructureID structureID)
{
    return bitwise_cast<EncodedStructureBits>(HeapPtr<Structure>(structure)) ^ (static_cast<EncodedStructureBits>(structureID) << s_entropyBitsShiftForStructurePointer);
}
#endif

inline Structure* StructureIDTable::get(StructureID structureID)
{
    ASSERT_WITH_SECURITY_IMPLICATION(structureID);
    ASSERT_WITH_SECURITY_IMPLICATION(!isNuked(structureID));
    uint32_t structureIndex = structureID >> s_numberOfEntropyBits;
    RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(structureIndex < m_capacity);
    const StructureOrOffset& structureOrOffset = table()[structureIndex];
#if ENCODE_STRUCTURE_BITS
    return decode(structureOrOffset.encodedStructureBits, structureID);
#else
    return structureOrOffset.structure;
#endif
}

inline bool StructureIDTable::isValid(StructureID structureID)
{
    if (!structureID)
        return false;
    uint32_t structureIndex = structureID >> s_numberOfEntropyBits;
    if (structureIndex >= m_capacity)
        return false;
#if CPU(ADDRESS64) && ENCODE_STRUCTURE_BITS
    HeapPtr<Structure> structure = decode(table()[structureIndex].encodedStructureBits, structureID);
    if (bitwise_cast<EncodedStructureBits>(structure) >> s_entropyBitsShiftForStructurePointer)
        return false;
#endif
    return true;
}

#else // not USE(JSVALUE64)

class StructureIDTable {
    friend class LLIntOffsetsExtractor;
public:
    StructureIDTable() = default;

    Structure* get(StructureID structureID) { return structureID; }
    void deallocateID(Structure*, StructureID) { }
    StructureID allocateID(Structure* structure)
    {
        ASSERT(!isNuked(structure));
        return structure;
    };

    void flushOldTables() { }
};

#endif // not USE(JSVALUE64)

} // namespace JSC
