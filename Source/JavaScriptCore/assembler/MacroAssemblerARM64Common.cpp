/*
 * Copyright (C) 2013-2019 Apple Inc. All rights reserved.
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

#if ENABLE(ASSEMBLER) && CPU(ARM64) // Also includes ARM64_CAPS
#include "MacroAssembler.h"

#if OS(LINUX)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif

namespace JSC {

void MacroAssemblerARM64::collectCPUFeatures()
{
#if OS(LINUX)
    static std::once_flag onceKey;
    std::call_once(onceKey, [] {
        // A register for describing ARM64 CPU features are only accessible in kernel mode.
        // Thus, some kernel support is necessary to collect CPU features. In Linux, the
        // kernel passes CPU feature flags in AT_HWCAP auxiliary vector which is passed
        // when the process starts. While this may pose a bit conservative information
        // (for example, the Linux kernel may add a flag for a feature after the feature
        // is shipped and implemented in some CPUs. In that case, even if the CPU has
        // that feature, the kernel does not tell it to users.), it is a stable approach.
        // https://www.kernel.org/doc/Documentation/arm64/elf_hwcaps.txt
        uint64_t hwcaps = getauxval(AT_HWCAP);

#if !defined(HWCAP_JSCVT)
#define HWCAP_JSCVT (1 << 13)
#endif

        s_jscvtCheckState = (hwcaps & HWCAP_JSCVT) ? CPUIDCheckState::Set : CPUIDCheckState::Clear;
    });
#elif HAVE(FJCVTZS_INSTRUCTION)
    s_jscvtCheckState = CPUIDCheckState::Set;
#else
    s_jscvtCheckState = CPUIDCheckState::Clear;
#endif
}

MacroAssemblerARM64::CPUIDCheckState MacroAssemblerARM64::s_jscvtCheckState = CPUIDCheckState::NotChecked;

} // namespace JSC

#endif // ENABLE(ASSEMBLER) && CPU(ARM64)

