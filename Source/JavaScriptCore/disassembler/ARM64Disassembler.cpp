/*
 * Copyright (C) 2012-2018 Apple Inc. All rights reserved.
 * Copyright (C) 2020 Arm Ltd. All rights reserved.
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

#include "config.h"
#include "Disassembler.h"

#if USE(ARM64_DISASSEMBLER)

#include "A64DOpcode.h"
#include "MacroAssemblerCodeRef.h"

#if CPU(ARM64_CAPS)
#include <cheri/cheric.h>
#endif

namespace JSC {

bool tryToDisassemble(const MacroAssemblerCodePtr<DisassemblyPtrTag>& codePtr, size_t size, const char* prefix, PrintStream& out)
{
    A64DOpcode arm64Opcode;

    uint32_t* currentPC = codePtr.dataLocation<uint32_t*>();
    size_t byteCount = size;

    while (byteCount) {
        char pcString[20];
        snprintf(pcString, sizeof(pcString), "0x%lx", reinterpret_cast<unsigned long>(currentPC));

#if CPU(ARM64_CAPS)
        if (byteCount >= sizeof(void *) && isPointerAligned(currentPC)) {
            void* ptr = *reinterpret_cast<void **>(currentPC);

            if (__builtin_cheri_tag_get(ptr)) {
                unsigned long base = __builtin_cheri_base_get(ptr);
                unsigned long length = __builtin_cheri_length_get(ptr);
                unsigned long address = __builtin_cheri_address_get(ptr);

                unsigned long perms = __builtin_cheri_perms_get(ptr);
                bool isR = (perms & (CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP)) != 0;
                bool isW = (perms & (CHERI_PERM_STORE | CHERI_PERM_STORE_CAP)) != 0;
                bool isX = (perms & CHERI_PERM_EXECUTE) != 0;

                out.printf("%s%16s:    %-8.8s0x%016lx [0x%016lx ; 0x%016lx) %c%c%c\n",
                           prefix,
                           pcString,
                           "cap",
                           address,
                           base,
                           base + length,
                           isR ? 'R' : '-',
                           isW ? 'W' : '-',
                           isX ? 'X' : '-');

                currentPC += sizeof(void *) / sizeof(int32_t);
                byteCount -= sizeof(void *);

                continue;
            }
	}
#endif

        out.printf("%s%16s: %s\n", prefix, pcString, arm64Opcode.disassemble(currentPC));
        currentPC++;
        byteCount -= sizeof(uint32_t);
    }

    return true;
}

} // namespace JSC

#endif // USE(ARM64_DISASSEMBLER)

