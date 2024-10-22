/*
 * Copyright (C) 2012-2019 Apple Inc. All rights reserved.
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

#include "LLIntCommon.h"
#include <wtf/Assertions.h>
#include <wtf/Gigacage.h>
#include <runtime/StructureIDTable.h>

#if ENCODE_STRUCTURE_BITS
#define OFFLINE_ASM_ENCODE_STRUCTURE_BITS 1
#else
#define OFFLINE_ASM_ENCODE_STRUCTURE_BITS 0
#endif

#ifdef __CHERI_PURE_CAPABILITY__
#ifndef __SIZEOF_INTCAP__
#error __SIZEOF_INTCAP__ is undefined
#elif __SIZEOF_INTCAP__ == 16
#define OFFLINE_ASM_CHERI_128_PURECAP 1
#define OFFLINE_ASM_CHERI_256_PURECAP 0
#elif __SIZEOF_INTCAP__ == 32
#define OFFLINE_ASM_CHERI_128_PURECAP 0
#define OFFLINE_ASM_CHERI_256_PURECAP 1
#else
#error __SIZEOF_INTCAP__ has an unsupported value
#endif
#else
#define OFFLINE_ASM_CHERI_128_PURECAP 0
#define OFFLINE_ASM_CHERI_256_PURECAP 0
#endif


#if ENABLE(C_LOOP)
#if !OS(WINDOWS)
#define OFFLINE_ASM_C_LOOP 1
#define OFFLINE_ASM_C_LOOP_WIN 0
#else
#define OFFLINE_ASM_C_LOOP 0
#define OFFLINE_ASM_C_LOOP_WIN 1
#endif
#define OFFLINE_ASM_X86 0
#define OFFLINE_ASM_X86_WIN 0
#define OFFLINE_ASM_ARMv7 0
#define OFFLINE_ASM_ARM64 0
#define OFFLINE_ASM_ARM64_CAPS 0
#define OFFLINE_ASM_ARM64E 0
#define OFFLINE_ASM_X86_64 0
#define OFFLINE_ASM_X86_64_WIN 0
#define OFFLINE_ASM_ARMv7k 0
#define OFFLINE_ASM_ARMv7s 0
#define OFFLINE_ASM_MIPS 0

#else // ENABLE(C_LOOP)

#define OFFLINE_ASM_C_LOOP 0
#define OFFLINE_ASM_C_LOOP_WIN 0

#if CPU(X86) && !COMPILER(MSVC)
#define OFFLINE_ASM_X86 1
#else
#define OFFLINE_ASM_X86 0
#endif

#if CPU(X86) && COMPILER(MSVC)
#define OFFLINE_ASM_X86_WIN 1
#else
#define OFFLINE_ASM_X86_WIN 0
#endif

#ifdef __ARM_ARCH_7K__
#define OFFLINE_ASM_ARMv7k 1
#else
#define OFFLINE_ASM_ARMv7k 0
#endif

#ifdef __ARM_ARCH_7S__
#define OFFLINE_ASM_ARMv7s 1
#else
#define OFFLINE_ASM_ARMv7s 0
#endif

#if CPU(ARM_THUMB2)
#define OFFLINE_ASM_ARMv7 1
#else
#define OFFLINE_ASM_ARMv7 0
#endif

#if CPU(X86_64) && !COMPILER(MSVC)
#define OFFLINE_ASM_X86_64 1
#else
#define OFFLINE_ASM_X86_64 0
#endif

#if CPU(X86_64) && COMPILER(MSVC)
#define OFFLINE_ASM_X86_64_WIN 1
#else
#define OFFLINE_ASM_X86_64_WIN 0
#endif

#if CPU(MIPS)
#define OFFLINE_ASM_MIPS 1
#else
#define OFFLINE_ASM_MIPS 0
#endif

#if CPU(ARM64)
#define OFFLINE_ASM_ARM64 1
#else
#define OFFLINE_ASM_ARM64 0
#endif

#if CPU(ARM64E)
#define OFFLINE_ASM_ARM64E 1
#undef OFFLINE_ASM_ARM64
#define OFFLINE_ASM_ARM64 0 // Pretend that ARM64 and ARM64E are mutually exclusive to please the offlineasm.
#else
#define OFFLINE_ASM_ARM64E 0
#endif

#if CPU(ARM64_CAPS)
#define OFFLINE_ASM_ARM64_CAPS 1
#undef OFFLINE_ASM_ARM64
#define OFFLINE_ASM_ARM64 0 // Pretend that ARM64 and ARM64_CAPS are mutually exclusive to please the offlineasm.
#else
#define OFFLINE_ASM_ARM64_CAPS 0
#endif

#if CPU(MIPS)
#ifdef WTF_MIPS_PIC
#define S(x) #x
#define SX(x) S(x)
#define OFFLINE_ASM_CPLOAD(reg) \
    ".set noreorder\n" \
    ".cpload " SX(reg) "\n" \
    ".set reorder\n"
#else
#define OFFLINE_ASM_CPLOAD(reg)
#endif
#endif

#endif // ENABLE(C_LOOP)

#if USE(JSVALUE64)
#define OFFLINE_ASM_JSVALUE64 1
#else
#define OFFLINE_ASM_JSVALUE64 0
#endif

#if CPU(ADDRESS64)
#define OFFLINE_ASM_ADDRESS64 1
#else
#define OFFLINE_ASM_ADDRESS64 0
#endif

#if !ASSERT_DISABLED
#define OFFLINE_ASM_ASSERT_ENABLED 1
#else
#define OFFLINE_ASM_ASSERT_ENABLED 0
#endif

#if LLINT_TRACING
#define OFFLINE_ASM_TRACING 1
#else
#define OFFLINE_ASM_TRACING 0
#endif

#define OFFLINE_ASM_GIGACAGE_ENABLED GIGACAGE_ENABLED
