/*
 * Copyright (C) 2013-2019 Apple Inc. All rights reserved.
 * Copyright (C) 2020-2022 Arm Ltd. All rights reserved.
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

#if ENABLE(ASSEMBLER) && CPU(ARM64_CAPS)  // Morello purecap only.
#include "MacroAssembler.h"

#include "ProbeContext.h"
#include <wtf/InlineASM.h>

namespace JSC {

#if ENABLE(MASM_PROBE)

extern "C" void ctiMasmProbeTrampoline();

using namespace ARM64Registers;

#if COMPILER(GCC_COMPATIBLE)

// The following are offsets for Probe::State fields accessed
// by the ctiMasmProbeTrampoline stub.
#if CPU(ADDRESS64)
// The size of a capability.
#define PTR_SIZE 16
#else
#define PTR_SIZE 4
#endif

#define PROBE_PROBE_FUNCTION_OFFSET (0 * PTR_SIZE)
#define PROBE_ARG_OFFSET (1 * PTR_SIZE)
#define PROBE_INIT_STACK_FUNCTION_OFFSET (2 * PTR_SIZE)
#define PROBE_INIT_STACK_ARG_OFFSET (3 * PTR_SIZE)

#define PROBE_FIRST_GPREG_OFFSET (4 * PTR_SIZE)

#define GPREG_SIZE 16

#define PROBE_CPU_C0_OFFSET (PROBE_FIRST_GPREG_OFFSET + (0 * GPREG_SIZE))
#define PROBE_CPU_C1_OFFSET (PROBE_FIRST_GPREG_OFFSET + (1 * GPREG_SIZE))
#define PROBE_CPU_C2_OFFSET (PROBE_FIRST_GPREG_OFFSET + (2 * GPREG_SIZE))
#define PROBE_CPU_C3_OFFSET (PROBE_FIRST_GPREG_OFFSET + (3 * GPREG_SIZE))
#define PROBE_CPU_C4_OFFSET (PROBE_FIRST_GPREG_OFFSET + (4 * GPREG_SIZE))
#define PROBE_CPU_C5_OFFSET (PROBE_FIRST_GPREG_OFFSET + (5 * GPREG_SIZE))
#define PROBE_CPU_C6_OFFSET (PROBE_FIRST_GPREG_OFFSET + (6 * GPREG_SIZE))
#define PROBE_CPU_C7_OFFSET (PROBE_FIRST_GPREG_OFFSET + (7 * GPREG_SIZE))
#define PROBE_CPU_C8_OFFSET (PROBE_FIRST_GPREG_OFFSET + (8 * GPREG_SIZE))
#define PROBE_CPU_C9_OFFSET (PROBE_FIRST_GPREG_OFFSET + (9 * GPREG_SIZE))
#define PROBE_CPU_C10_OFFSET (PROBE_FIRST_GPREG_OFFSET + (10 * GPREG_SIZE))
#define PROBE_CPU_C11_OFFSET (PROBE_FIRST_GPREG_OFFSET + (11 * GPREG_SIZE))
#define PROBE_CPU_C12_OFFSET (PROBE_FIRST_GPREG_OFFSET + (12 * GPREG_SIZE))
#define PROBE_CPU_C13_OFFSET (PROBE_FIRST_GPREG_OFFSET + (13 * GPREG_SIZE))
#define PROBE_CPU_C14_OFFSET (PROBE_FIRST_GPREG_OFFSET + (14 * GPREG_SIZE))
#define PROBE_CPU_C15_OFFSET (PROBE_FIRST_GPREG_OFFSET + (15 * GPREG_SIZE))
#define PROBE_CPU_C16_OFFSET (PROBE_FIRST_GPREG_OFFSET + (16 * GPREG_SIZE))
#define PROBE_CPU_C17_OFFSET (PROBE_FIRST_GPREG_OFFSET + (17 * GPREG_SIZE))
#define PROBE_CPU_C18_OFFSET (PROBE_FIRST_GPREG_OFFSET + (18 * GPREG_SIZE))
#define PROBE_CPU_C19_OFFSET (PROBE_FIRST_GPREG_OFFSET + (19 * GPREG_SIZE))
#define PROBE_CPU_C20_OFFSET (PROBE_FIRST_GPREG_OFFSET + (20 * GPREG_SIZE))
#define PROBE_CPU_C21_OFFSET (PROBE_FIRST_GPREG_OFFSET + (21 * GPREG_SIZE))
#define PROBE_CPU_C22_OFFSET (PROBE_FIRST_GPREG_OFFSET + (22 * GPREG_SIZE))
#define PROBE_CPU_C23_OFFSET (PROBE_FIRST_GPREG_OFFSET + (23 * GPREG_SIZE))
#define PROBE_CPU_C24_OFFSET (PROBE_FIRST_GPREG_OFFSET + (24 * GPREG_SIZE))
#define PROBE_CPU_C25_OFFSET (PROBE_FIRST_GPREG_OFFSET + (25 * GPREG_SIZE))
#define PROBE_CPU_C26_OFFSET (PROBE_FIRST_GPREG_OFFSET + (26 * GPREG_SIZE))
#define PROBE_CPU_C27_OFFSET (PROBE_FIRST_GPREG_OFFSET + (27 * GPREG_SIZE))
#define PROBE_CPU_C28_OFFSET (PROBE_FIRST_GPREG_OFFSET + (28 * GPREG_SIZE))
#define PROBE_CPU_CFP_OFFSET (PROBE_FIRST_GPREG_OFFSET + (29 * GPREG_SIZE))
#define PROBE_CPU_CLR_OFFSET (PROBE_FIRST_GPREG_OFFSET + (30 * GPREG_SIZE))
#define PROBE_CPU_CSP_OFFSET (PROBE_FIRST_GPREG_OFFSET + (31 * GPREG_SIZE))

#define PROBE_CPU_PCC_OFFSET (PROBE_FIRST_GPREG_OFFSET + (32 * GPREG_SIZE))
// TODO: These don't actually require 16 bytes.
#define PROBE_CPU_NZCV_OFFSET (PROBE_FIRST_GPREG_OFFSET + (33 * GPREG_SIZE))
#define PROBE_CPU_FPSR_OFFSET (PROBE_FIRST_GPREG_OFFSET + (34 * GPREG_SIZE))

#define PROBE_FIRST_FPREG_OFFSET (PROBE_FIRST_GPREG_OFFSET + (35 * GPREG_SIZE))

#define FPREG_SIZE 8
#define PROBE_CPU_Q0_OFFSET (PROBE_FIRST_FPREG_OFFSET + (0 * FPREG_SIZE))
#define PROBE_CPU_Q1_OFFSET (PROBE_FIRST_FPREG_OFFSET + (1 * FPREG_SIZE))
#define PROBE_CPU_Q2_OFFSET (PROBE_FIRST_FPREG_OFFSET + (2 * FPREG_SIZE))
#define PROBE_CPU_Q3_OFFSET (PROBE_FIRST_FPREG_OFFSET + (3 * FPREG_SIZE))
#define PROBE_CPU_Q4_OFFSET (PROBE_FIRST_FPREG_OFFSET + (4 * FPREG_SIZE))
#define PROBE_CPU_Q5_OFFSET (PROBE_FIRST_FPREG_OFFSET + (5 * FPREG_SIZE))
#define PROBE_CPU_Q6_OFFSET (PROBE_FIRST_FPREG_OFFSET + (6 * FPREG_SIZE))
#define PROBE_CPU_Q7_OFFSET (PROBE_FIRST_FPREG_OFFSET + (7 * FPREG_SIZE))
#define PROBE_CPU_Q8_OFFSET (PROBE_FIRST_FPREG_OFFSET + (8 * FPREG_SIZE))
#define PROBE_CPU_Q9_OFFSET (PROBE_FIRST_FPREG_OFFSET + (9 * FPREG_SIZE))
#define PROBE_CPU_Q10_OFFSET (PROBE_FIRST_FPREG_OFFSET + (10 * FPREG_SIZE))
#define PROBE_CPU_Q11_OFFSET (PROBE_FIRST_FPREG_OFFSET + (11 * FPREG_SIZE))
#define PROBE_CPU_Q12_OFFSET (PROBE_FIRST_FPREG_OFFSET + (12 * FPREG_SIZE))
#define PROBE_CPU_Q13_OFFSET (PROBE_FIRST_FPREG_OFFSET + (13 * FPREG_SIZE))
#define PROBE_CPU_Q14_OFFSET (PROBE_FIRST_FPREG_OFFSET + (14 * FPREG_SIZE))
#define PROBE_CPU_Q15_OFFSET (PROBE_FIRST_FPREG_OFFSET + (15 * FPREG_SIZE))
#define PROBE_CPU_Q16_OFFSET (PROBE_FIRST_FPREG_OFFSET + (16 * FPREG_SIZE))
#define PROBE_CPU_Q17_OFFSET (PROBE_FIRST_FPREG_OFFSET + (17 * FPREG_SIZE))
#define PROBE_CPU_Q18_OFFSET (PROBE_FIRST_FPREG_OFFSET + (18 * FPREG_SIZE))
#define PROBE_CPU_Q19_OFFSET (PROBE_FIRST_FPREG_OFFSET + (19 * FPREG_SIZE))
#define PROBE_CPU_Q20_OFFSET (PROBE_FIRST_FPREG_OFFSET + (20 * FPREG_SIZE))
#define PROBE_CPU_Q21_OFFSET (PROBE_FIRST_FPREG_OFFSET + (21 * FPREG_SIZE))
#define PROBE_CPU_Q22_OFFSET (PROBE_FIRST_FPREG_OFFSET + (22 * FPREG_SIZE))
#define PROBE_CPU_Q23_OFFSET (PROBE_FIRST_FPREG_OFFSET + (23 * FPREG_SIZE))
#define PROBE_CPU_Q24_OFFSET (PROBE_FIRST_FPREG_OFFSET + (24 * FPREG_SIZE))
#define PROBE_CPU_Q25_OFFSET (PROBE_FIRST_FPREG_OFFSET + (25 * FPREG_SIZE))
#define PROBE_CPU_Q26_OFFSET (PROBE_FIRST_FPREG_OFFSET + (26 * FPREG_SIZE))
#define PROBE_CPU_Q27_OFFSET (PROBE_FIRST_FPREG_OFFSET + (27 * FPREG_SIZE))
#define PROBE_CPU_Q28_OFFSET (PROBE_FIRST_FPREG_OFFSET + (28 * FPREG_SIZE))
#define PROBE_CPU_Q29_OFFSET (PROBE_FIRST_FPREG_OFFSET + (29 * FPREG_SIZE))
#define PROBE_CPU_Q30_OFFSET (PROBE_FIRST_FPREG_OFFSET + (30 * FPREG_SIZE))
#define PROBE_CPU_Q31_OFFSET (PROBE_FIRST_FPREG_OFFSET + (31 * FPREG_SIZE))
#define PROBE_SIZE (PROBE_FIRST_FPREG_OFFSET + (32 * FPREG_SIZE))

#define SAVED_PROBE_RETURN_PCC_OFFSET       (PROBE_SIZE + (0 * GPREG_SIZE))
#define PROBE_SIZE_PLUS_EXTRAS              (PROBE_SIZE + (3 * GPREG_SIZE))

// These ASSERTs remind you that if you change the layout of Probe::State,
// you need to change ctiMasmProbeTrampoline offsets above to match.
#define PROBE_OFFSETOF(x) offsetof(struct Probe::State, x)
static_assert(PROBE_OFFSETOF(probeFunction) == PROBE_PROBE_FUNCTION_OFFSET, "Probe::State::probeFunction's offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(arg) == PROBE_ARG_OFFSET, "Probe::State::arg's offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(initializeStackFunction) == PROBE_INIT_STACK_FUNCTION_OFFSET, "Probe::State::initializeStackFunction's offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(initializeStackArg) == PROBE_INIT_STACK_ARG_OFFSET, "Probe::State::initializeStackArg's offset matches ctiMasmProbeTrampoline");

static_assert(!(PROBE_CPU_C0_OFFSET & 0xf), "Probe::State::cpu.gprs[c0]'s offset should be 16 byte aligned");

static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c0]) == PROBE_CPU_C0_OFFSET, "Probe::State::cpu.gprs[c0]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c1]) == PROBE_CPU_C1_OFFSET, "Probe::State::cpu.gprs[c1]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c2]) == PROBE_CPU_C2_OFFSET, "Probe::State::cpu.gprs[c2]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c3]) == PROBE_CPU_C3_OFFSET, "Probe::State::cpu.gprs[c3]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c4]) == PROBE_CPU_C4_OFFSET, "Probe::State::cpu.gprs[c4]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c5]) == PROBE_CPU_C5_OFFSET, "Probe::State::cpu.gprs[c5]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c6]) == PROBE_CPU_C6_OFFSET, "Probe::State::cpu.gprs[c6]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c7]) == PROBE_CPU_C7_OFFSET, "Probe::State::cpu.gprs[c7]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c8]) == PROBE_CPU_C8_OFFSET, "Probe::State::cpu.gprs[c8]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c9]) == PROBE_CPU_C9_OFFSET, "Probe::State::cpu.gprs[c9]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c10]) == PROBE_CPU_C10_OFFSET, "Probe::State::cpu.gprs[c10]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c11]) == PROBE_CPU_C11_OFFSET, "Probe::State::cpu.gprs[c11]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c12]) == PROBE_CPU_C12_OFFSET, "Probe::State::cpu.gprs[c12]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c13]) == PROBE_CPU_C13_OFFSET, "Probe::State::cpu.gprs[c13]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c14]) == PROBE_CPU_C14_OFFSET, "Probe::State::cpu.gprs[c14]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c15]) == PROBE_CPU_C15_OFFSET, "Probe::State::cpu.gprs[c15]'s offset matches ctiMasmProbeTrampoline");

static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c16]) == PROBE_CPU_C16_OFFSET, "Probe::State::cpu.gprs[c16]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c17]) == PROBE_CPU_C17_OFFSET, "Probe::State::cpu.gprs[c17]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c18]) == PROBE_CPU_C18_OFFSET, "Probe::State::cpu.gprs[c18]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c19]) == PROBE_CPU_C19_OFFSET, "Probe::State::cpu.gprs[c19]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c20]) == PROBE_CPU_C20_OFFSET, "Probe::State::cpu.gprs[c20]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c21]) == PROBE_CPU_C21_OFFSET, "Probe::State::cpu.gprs[c21]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c22]) == PROBE_CPU_C22_OFFSET, "Probe::State::cpu.gprs[c22]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c23]) == PROBE_CPU_C23_OFFSET, "Probe::State::cpu.gprs[c23]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c24]) == PROBE_CPU_C24_OFFSET, "Probe::State::cpu.gprs[c24]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c25]) == PROBE_CPU_C25_OFFSET, "Probe::State::cpu.gprs[c25]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c26]) == PROBE_CPU_C26_OFFSET, "Probe::State::cpu.gprs[c26]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c27]) == PROBE_CPU_C27_OFFSET, "Probe::State::cpu.gprs[c27]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::c28]) == PROBE_CPU_C28_OFFSET, "Probe::State::cpu.gprs[c28]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::cfp]) == PROBE_CPU_CFP_OFFSET, "Probe::State::cpu.gprs[cfp]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::clr]) == PROBE_CPU_CLR_OFFSET, "Probe::State::cpu.gprs[clr]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.gprs[ARM64Registers::csp]) == PROBE_CPU_CSP_OFFSET, "Probe::State::cpu.gprs[csp]'s offset matches ctiMasmProbeTrampoline");

static_assert(PROBE_OFFSETOF(cpu.sprs[ARM64Registers::pcc]) == PROBE_CPU_PCC_OFFSET, "Probe::State::cpu.sprs[pcc]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.sprs[ARM64Registers::nzcv]) == PROBE_CPU_NZCV_OFFSET, "Probe::State::cpu.sprs[nzcv]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.sprs[ARM64Registers::fpsr]) == PROBE_CPU_FPSR_OFFSET, "Probe::State::cpu.sprs[fpsr]'s offset matches ctiMasmProbeTrampoline");

static_assert(!(PROBE_CPU_Q0_OFFSET & 0x7), "Probe::State::cpu.fprs[q0]'s offset should be 8 byte aligned");

static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q0]) == PROBE_CPU_Q0_OFFSET, "Probe::State::cpu.fprs[q0]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q1]) == PROBE_CPU_Q1_OFFSET, "Probe::State::cpu.fprs[q1]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q2]) == PROBE_CPU_Q2_OFFSET, "Probe::State::cpu.fprs[q2]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q3]) == PROBE_CPU_Q3_OFFSET, "Probe::State::cpu.fprs[q3]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q4]) == PROBE_CPU_Q4_OFFSET, "Probe::State::cpu.fprs[q4]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q5]) == PROBE_CPU_Q5_OFFSET, "Probe::State::cpu.fprs[q5]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q6]) == PROBE_CPU_Q6_OFFSET, "Probe::State::cpu.fprs[q6]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q7]) == PROBE_CPU_Q7_OFFSET, "Probe::State::cpu.fprs[q7]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q8]) == PROBE_CPU_Q8_OFFSET, "Probe::State::cpu.fprs[q8]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q9]) == PROBE_CPU_Q9_OFFSET, "Probe::State::cpu.fprs[q9]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q10]) == PROBE_CPU_Q10_OFFSET, "Probe::State::cpu.fprs[q10]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q11]) == PROBE_CPU_Q11_OFFSET, "Probe::State::cpu.fprs[q11]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q12]) == PROBE_CPU_Q12_OFFSET, "Probe::State::cpu.fprs[q12]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q13]) == PROBE_CPU_Q13_OFFSET, "Probe::State::cpu.fprs[q13]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q14]) == PROBE_CPU_Q14_OFFSET, "Probe::State::cpu.fprs[q14]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q15]) == PROBE_CPU_Q15_OFFSET, "Probe::State::cpu.fprs[q15]'s offset matches ctiMasmProbeTrampoline");

static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q16]) == PROBE_CPU_Q16_OFFSET, "Probe::State::cpu.fprs[q16]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q17]) == PROBE_CPU_Q17_OFFSET, "Probe::State::cpu.fprs[q17]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q18]) == PROBE_CPU_Q18_OFFSET, "Probe::State::cpu.fprs[q18]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q19]) == PROBE_CPU_Q19_OFFSET, "Probe::State::cpu.fprs[q19]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q20]) == PROBE_CPU_Q20_OFFSET, "Probe::State::cpu.fprs[q20]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q21]) == PROBE_CPU_Q21_OFFSET, "Probe::State::cpu.fprs[q21]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q22]) == PROBE_CPU_Q22_OFFSET, "Probe::State::cpu.fprs[q22]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q23]) == PROBE_CPU_Q23_OFFSET, "Probe::State::cpu.fprs[q23]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q24]) == PROBE_CPU_Q24_OFFSET, "Probe::State::cpu.fprs[q24]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q25]) == PROBE_CPU_Q25_OFFSET, "Probe::State::cpu.fprs[q25]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q26]) == PROBE_CPU_Q26_OFFSET, "Probe::State::cpu.fprs[q26]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q27]) == PROBE_CPU_Q27_OFFSET, "Probe::State::cpu.fprs[q27]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q28]) == PROBE_CPU_Q28_OFFSET, "Probe::State::cpu.fprs[q28]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q29]) == PROBE_CPU_Q29_OFFSET, "Probe::State::cpu.fprs[q29]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q30]) == PROBE_CPU_Q30_OFFSET, "Probe::State::cpu.fprs[q30]'s offset matches ctiMasmProbeTrampoline");
static_assert(PROBE_OFFSETOF(cpu.fprs[ARM64Registers::q31]) == PROBE_CPU_Q31_OFFSET, "Probe::State::cpu.fprs[q31]'s offset matches ctiMasmProbeTrampoline");

static_assert(sizeof(Probe::State) == PROBE_SIZE, "Probe::State's size matches ctiMasmProbeTrampoline");

// Conditions for using ldp and stp.
static_assert(PROBE_CPU_PCC_OFFSET == PROBE_CPU_CSP_OFFSET + GPREG_SIZE, "PROBE_CPU_CSP_OFFSET and PROBE_CPU_PCC_OFFSET must be adjacent");
static_assert(!(PROBE_SIZE_PLUS_EXTRAS & 0xf), "PROBE_SIZE_PLUS_EXTRAS should be 16 byte aligned"); // the Probe::State copying code relies on this.

#undef PROBE_OFFSETOF

#define FPR_OFFSET(fpr) (PROBE_CPU_##fpr##_OFFSET - PROBE_CPU_Q0_OFFSET)

struct IncomingProbeRecord {
    UCPURegister c24;
    UCPURegister c25;
    UCPURegister c26;
    UCPURegister c27;
    UCPURegister c28;
    UCPURegister c30; // clr
};

#define IN_C24_OFFSET (0 * GPREG_SIZE)
#define IN_C25_OFFSET (1 * GPREG_SIZE)
#define IN_C26_OFFSET (2 * GPREG_SIZE)
#define IN_C27_OFFSET (3 * GPREG_SIZE)
#define IN_C28_OFFSET (4 * GPREG_SIZE)
#define IN_C30_OFFSET (5 * GPREG_SIZE)
#define IN_SIZE       (6 * GPREG_SIZE)

static_assert(IN_C24_OFFSET == offsetof(IncomingProbeRecord, c24), "IN_C24_OFFSET is incorrect");
static_assert(IN_C25_OFFSET == offsetof(IncomingProbeRecord, c25), "IN_C25_OFFSET is incorrect");
static_assert(IN_C26_OFFSET == offsetof(IncomingProbeRecord, c26), "IN_C26_OFFSET is incorrect");
static_assert(IN_C27_OFFSET == offsetof(IncomingProbeRecord, c27), "IN_C27_OFFSET is incorrect");
static_assert(IN_C28_OFFSET == offsetof(IncomingProbeRecord, c28), "IN_C22_OFFSET is incorrect");
static_assert(IN_C30_OFFSET == offsetof(IncomingProbeRecord, c30), "IN_C23_OFFSET is incorrect");
static_assert(IN_SIZE == sizeof(IncomingProbeRecord), "IN_SIZE is incorrect");
static_assert(!(sizeof(IncomingProbeRecord) & 0xf), "IncomingProbeStack must be 16-byte aligned");

struct OutgoingProbeRecord {
    UCPURegister nzcv;
    UCPURegister fpsr;
    UCPURegister c27;
    UCPURegister c28;
    UCPURegister cfp;
    UCPURegister clr;
};

#define OUT_NZCV_OFFSET (0 * GPREG_SIZE)
#define OUT_FPSR_OFFSET (1 * GPREG_SIZE)
#define OUT_C27_OFFSET  (2 * GPREG_SIZE)
#define OUT_C28_OFFSET  (3 * GPREG_SIZE)
#define OUT_CFP_OFFSET  (4 * GPREG_SIZE)
#define OUT_CLR_OFFSET  (5 * GPREG_SIZE)
#define OUT_SIZE        (6 * GPREG_SIZE)

static_assert(OUT_NZCV_OFFSET == offsetof(OutgoingProbeRecord, nzcv), "OUT_NZCV_OFFSET is incorrect");
static_assert(OUT_FPSR_OFFSET == offsetof(OutgoingProbeRecord, fpsr), "OUT_FPSR_OFFSET is incorrect");
static_assert(OUT_C27_OFFSET == offsetof(OutgoingProbeRecord, c27), "OUT_C27_OFFSET is incorrect");
static_assert(OUT_C28_OFFSET == offsetof(OutgoingProbeRecord, c28), "OUT_C28_OFFSET is incorrect");
static_assert(OUT_CFP_OFFSET == offsetof(OutgoingProbeRecord, cfp), "OUT_CFP_OFFSET is incorrect");
static_assert(OUT_CLR_OFFSET == offsetof(OutgoingProbeRecord, clr), "OUT_CLR_OFFSET is incorrect");
static_assert(OUT_SIZE == sizeof(OutgoingProbeRecord), "OUT_SIZE is incorrect");
static_assert(!(sizeof(OutgoingProbeRecord) & 0xf), "OutgoingProbeStack must be 16-byte aligned");

struct LRRestorationRecord {
    UCPURegister clr;
};

#define LR_RESTORATION_CLR_OFFSET (0 * GPREG_SIZE)
#define LR_RESTORATION_SIZE       (1 * GPREG_SIZE)

static_assert(LR_RESTORATION_CLR_OFFSET == offsetof(LRRestorationRecord, clr), "LR_RESTORATION_CLR_OFFSET is incorrect");
static_assert(LR_RESTORATION_SIZE == sizeof(LRRestorationRecord), "CLR_RESTORATION_SIZE is incorrect");
static_assert(!(sizeof(LRRestorationRecord) & 0xf), "LRRestorationRecord must be 16-byte aligned");

asm (
    ".text" "\n"
    ".balign 16" "\n"
    ".type " SYMBOL_STRING(ctiMasmProbeTrampoline) ", @function\n"
    ".globl " SYMBOL_STRING(ctiMasmProbeTrampoline) "\n"
    HIDE_SYMBOL(ctiMasmProbeTrampoline) "\n"
    SYMBOL_STRING(ctiMasmProbeTrampoline) ":" "\n"

    // MacroAssemblerARM64Caps::probe() has already generated code to store some values in an
    // IncomingProbeRecord. csp points to the IncomingProbeRecord.
    //
    // Incoming register values:
    //     c24: probe function
    //     c25: probe arg
    //     c26: scratch, was ctiMasmProbeTrampoline (this function)
    //     c27: scratch
    //     c28: Probe::executeProbe
    //     c30: return address
    //
    // All other registers need to be preserved.

    "add       c26, csp, #" STRINGIZE_VALUE_OF(IN_SIZE) "\n" // Compute the sp before the probe.

    "sub       csp, csp, #" STRINGIZE_VALUE_OF(PROBE_SIZE_PLUS_EXTRAS + OUT_SIZE) "\n"

    "stp       c24, c25, [csp, #" STRINGIZE_VALUE_OF(PROBE_PROBE_FUNCTION_OFFSET) "]" "\n" // Store the probe handler function and arg.

    "stp       c0, c1, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C0_OFFSET) "]" "\n"
    "mrs       x0, nzcv" "\n" // Preload nzcv.
    "stp       c2, c3, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C2_OFFSET) "]" "\n"
    "stp       c4, c5, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C4_OFFSET) "]" "\n"
    "mrs       x1, fpsr" "\n" // Preload fpsr.
    "stp       c6, c7, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C6_OFFSET) "]" "\n"
    "stp       c8, c9, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C8_OFFSET) "]" "\n"

    "ldp       c2, c3, [c26, #" STRINGIZE_VALUE_OF(IN_C24_OFFSET) "]" "\n" // Preload saved c24 and c25.
    "ldp       c4, c5, [c26, #" STRINGIZE_VALUE_OF(IN_C26_OFFSET) "]" "\n" // Preload saved c26 and c27.
    "ldp       c6, c7, [c26, #" STRINGIZE_VALUE_OF(IN_C28_OFFSET) "]" "\n" // Preload saved c28 and clr.

    "stp       c10, c11, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C10_OFFSET) "]" "\n"
    "stp       c12, c13, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C12_OFFSET) "]" "\n"
    "stp       c14, c15, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C14_OFFSET) "]" "\n"
    "stp       c16, c17, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C16_OFFSET) "]" "\n"
    "stp       c18, c19, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C18_OFFSET) "]" "\n"
    "stp       c20, c21, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C20_OFFSET) "]" "\n"
    "stp       c22, c23, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C22_OFFSET) "]" "\n"
    "stp       c2, c3, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C24_OFFSET) "]" "\n" // Store saved c24 and c25 (preloaded into c2 and c3 above).
    "stp       c4, c5, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C26_OFFSET) "]" "\n" // Store saved c26 and c27 (preloaded into c4 and c5 above).
    "stp       c6, c29, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C28_OFFSET) "]" "\n"
    "stp       c7, c26, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_CLR_OFFSET) "]" "\n" // Save values clr and csp (original csp value computed into c26 above).

    "str       clr, [csp, #" STRINGIZE_VALUE_OF(SAVED_PROBE_RETURN_PCC_OFFSET) "]" "\n" // Save a duplicate copy of return pcc (in clr).

    "add       clr, clr, #" STRINGIZE_VALUE_OF(2 * GPREG_SIZE) "\n" // The PC after the probe is at 2 instructions past the return point.
    "str       clr, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_PCC_OFFSET) "]" "\n"

    "stp       c0, c1, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_NZCV_OFFSET) "]" "\n" // Store nzcv and fpsr (preloaded into c0 and c1 above).

    "add       c9, csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_Q0_OFFSET) "\n"
    "stp       d0, d1, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q0)) "]" "\n"
    "stp       d2, d3, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q2)) "]" "\n"
    "stp       d4, d5, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q4)) "]" "\n"
    "stp       d6, d7, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q6)) "]" "\n"
    "stp       d8, d9, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q8)) "]" "\n"
    "stp       d10, d11, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q10)) "]" "\n"
    "stp       d12, d13, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q12)) "]" "\n"
    "stp       d14, d15, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q14)) "]" "\n"
    "stp       d16, d17, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q16)) "]" "\n"
    "stp       d18, d19, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q18)) "]" "\n"
    "stp       d20, d21, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q20)) "]" "\n"
    "stp       d22, d23, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q22)) "]" "\n"
    "stp       d24, d25, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q24)) "]" "\n"
    "stp       d26, d27, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q26)) "]" "\n"
    "stp       d28, d29, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q28)) "]" "\n"
    "stp       d30, d31, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q30)) "]" "\n"

    "mov       c27, csp" "\n" // Save the Probe::State* in a callee saved register.

    // Note: we haven't changed the value of fp. Hence, it is still pointing to the frame of
    // the caller of the probe (which is what we want in order to play nice with debuggers e.g. lldb).
    "mov       c0, csp" "\n" // Set the Probe::State* arg.
#if CPU(ARM64E)
#error ARM64E does not make sense with Morello.
#else
    "blr       c28" "\n" // Call the probe handler.
#endif

    // Make sure the Probe::State is entirely below the result stack pointer so
    // that register values are still preserved when we call the initializeStack
    // function.
    "ldr       c1, [c27, #" STRINGIZE_VALUE_OF(PROBE_CPU_CSP_OFFSET) "]" "\n" // Result csp.
    "add       c2, c27, #" STRINGIZE_VALUE_OF(PROBE_SIZE_PLUS_EXTRAS + OUT_SIZE) "\n" // End of Probe::State + buffer.
    "cmp       c1, c2" "\n"
    "bge     " LOCAL_LABEL_STRING(ctiMasmProbeTrampolineProbeStateIsSafe) "\n"

    // Allocate a safe place on the stack below the result stack pointer to stash the Probe::State.
    "sub       csp, c1, #" STRINGIZE_VALUE_OF(PROBE_SIZE_PLUS_EXTRAS + OUT_SIZE) "\n"

    // Copy the Probe::State to the safe place.
    // Note: we have to copy from low address to higher address because we're moving the
    // Probe::State to a lower address.
    "mov       c5, c27" "\n"
    "mov       c6, csp" "\n"
    "add       c7, c27, #" STRINGIZE_VALUE_OF(PROBE_SIZE_PLUS_EXTRAS) "\n"

    LOCAL_LABEL_STRING(ctiMasmProbeTrampolineCopyLoop) ":" "\n"
    "ldp       c3, c4, [c5], #16" "\n"
    "stp       c3, c4, [c6], #16" "\n"
    "cmp       c5, c7" "\n"
    "blt     " LOCAL_LABEL_STRING(ctiMasmProbeTrampolineCopyLoop) "\n"

    "mov       c27, csp" "\n"

    // Call initializeStackFunction if present.
    LOCAL_LABEL_STRING(ctiMasmProbeTrampolineProbeStateIsSafe) ":" "\n"
    "ldr       c2, [c27, #" STRINGIZE_VALUE_OF(PROBE_INIT_STACK_FUNCTION_OFFSET) "]" "\n"
    // There is no 'cbz' for capabilities, but testing the address (x2) is
    // sufficient here; if a non-zero, untagged capability is provided, crashing
    // (when we 'blr' to it) is a reasonable behaviour.
    "cbz       x2, " LOCAL_LABEL_STRING(ctiMasmProbeTrampolineRestoreRegisters) "\n"

    "mov       c0, c27" "\n" // Set the Probe::State* arg.
#if CPU(ARM64E)
#error ARM64E does not make sense with Morello.
#else
    "blr       c2" "\n" // Call the initializeStackFunction (loaded into c2 above).
#endif

    LOCAL_LABEL_STRING(ctiMasmProbeTrampolineRestoreRegisters) ":" "\n"

    "mov       csp, c27" "\n"

    // To enable probes to modify register state, we copy all registers
    // out of the Probe::State before returning. That is except for c18.
    // c18 is "reserved for the platform. Conforming software should not make use of it."
    // Hence, the JITs would not be using it, and the probe should also not be modifying it.

    "add       c9, csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_Q0_OFFSET) "\n"
    "ldp       d0, d1, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q0)) "]" "\n"
    "ldp       d2, d3, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q2)) "]" "\n"
    "ldp       d4, d5, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q4)) "]" "\n"
    "ldp       d6, d7, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q6)) "]" "\n"
    "ldp       d8, d9, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q8)) "]" "\n"
    "ldp       d10, d11, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q10)) "]" "\n"
    "ldp       d12, d13, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q12)) "]" "\n"
    "ldp       d14, d15, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q14)) "]" "\n"
    "ldp       d16, d17, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q16)) "]" "\n"
    "ldp       d18, d19, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q18)) "]" "\n"
    "ldp       d20, d21, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q20)) "]" "\n"
    "ldp       d22, d23, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q22)) "]" "\n"
    "ldp       d24, d25, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q24)) "]" "\n"
    "ldp       d26, d27, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q26)) "]" "\n"
    "ldp       d28, d29, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q28)) "]" "\n"
    "ldp       d30, d31, [c9, #" STRINGIZE_VALUE_OF(FPR_OFFSET(Q30)) "]" "\n"

    "ldp       c0, c1, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C0_OFFSET) "]" "\n"
    "ldp       c2, c3, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C2_OFFSET) "]" "\n"
    "ldp       c4, c5, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C4_OFFSET) "]" "\n"
    "ldp       c6, c7, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C6_OFFSET) "]" "\n"
    "ldp       c8, c9, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C8_OFFSET) "]" "\n"
    "ldp       c10, c11, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C10_OFFSET) "]" "\n"
    "ldp       c12, c13, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C12_OFFSET) "]" "\n"
    "ldp       c14, c15, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C14_OFFSET) "]" "\n"
    "ldp       c16, c17, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C16_OFFSET) "]" "\n"
    // c18 should not be modified by the probe. See comment above for details.
    "ldp       c19, c20, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C19_OFFSET) "]" "\n"
    "ldp       c21, c22, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C21_OFFSET) "]" "\n"
    "ldp       c23, c24, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C23_OFFSET) "]" "\n"
    "ldp       c25, c26, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C25_OFFSET) "]" "\n"

    // Remaining registers to restore are: fpsr, nzcv, c27, c28, cfp, clr, csp, and pcc.

    // The only way to set the pcc on ARM64_CAPS (Morello) is via an indirect branch
    // or a ret, which means we'll need a free register to do so. For our purposes, clr
    // happens to be available in applications of the probe where we may want to
    // continue executing at a different location (i.e. change the pcc) after the probe
    // returns. So, the ARM64_CAPS probe implementation will allow the probe handler to
    // either modify clr or pcc, but not both in the same probe invocation. The probe
    // mechanism ensures that we never try to modify both clr and pcc with a RELEASE_ASSERT
    // in Probe::executeProbe().

    // Determine if the probe handler changed the pc.
    "ldr       clr, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_CSP_OFFSET) "]" "\n" // preload the target csp.
    "ldr       c27, [csp, #" STRINGIZE_VALUE_OF(SAVED_PROBE_RETURN_PCC_OFFSET) "]" "\n"
    "ldr       c28, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_PCC_OFFSET) "]" "\n"
    "add       c27, c27, #" STRINGIZE_VALUE_OF(2 * GPREG_SIZE) "\n"
    "chkeq     c27, c28" "\n"  //  Unlike 'cmp', 'chkeq' compares all capability metadata bits.
    "bne     " LOCAL_LABEL_STRING(ctiMasmProbeTrampolineEnd) "\n"

     // We didn't change the PCC. So, let's prepare for setting a potentially new clr value.

     // 1. Make room for the LRRestorationRecord. The probe site will pop this off later.
    "sub       clr, clr, #" STRINGIZE_VALUE_OF(LR_RESTORATION_SIZE) "\n"
     // 2. Store the clr value to restore at the probe return site.
    "ldr       c27, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_CLR_OFFSET) "]" "\n"
    "str       c27, [clr, #" STRINGIZE_VALUE_OF(LR_RESTORATION_CLR_OFFSET) "]" "\n"
     // 3. Force the return ramp to return to the probe return site.
    "ldr       c27, [csp, #" STRINGIZE_VALUE_OF(SAVED_PROBE_RETURN_PCC_OFFSET) "]" "\n"
    "str       c27, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_PCC_OFFSET) "]" "\n"

    LOCAL_LABEL_STRING(ctiMasmProbeTrampolineEnd) ":" "\n"

    // Fill in the OutgoingProbeRecord.
    "sub       clr, clr, #" STRINGIZE_VALUE_OF(OUT_SIZE) "\n"

    "ldp       c27, c28, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_NZCV_OFFSET) "]" "\n"
    "stp       c27, c28, [clr, #" STRINGIZE_VALUE_OF(OUT_NZCV_OFFSET) "]" "\n"
    "ldp       c27, c28, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_C27_OFFSET) "]" "\n"
    "stp       c27, c28, [clr, #" STRINGIZE_VALUE_OF(OUT_C27_OFFSET) "]" "\n"
    "ldr       c27, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_CFP_OFFSET) "]" "\n"
    "ldr       c28, [csp, #" STRINGIZE_VALUE_OF(PROBE_CPU_PCC_OFFSET) "]" "\n" // Set up the outgoing record so that we'll jump to the new PCC.
    "stp       c27, c28, [clr, #" STRINGIZE_VALUE_OF(OUT_CFP_OFFSET) "]" "\n"
    "mov       csp, clr" "\n"

    // Restore the remaining registers and pop the OutgoingProbeRecord.
    "ldp       c27, c28, [csp], #" STRINGIZE_VALUE_OF(2 * GPREG_SIZE) "\n"
    "msr       nzcv, x27" "\n"
    "msr       fpsr, x28" "\n"
    "ldp       c27, c28, [csp], #" STRINGIZE_VALUE_OF(2 * GPREG_SIZE) "\n"
    "ldp       cfp, clr, [csp], #" STRINGIZE_VALUE_OF(2 * GPREG_SIZE) "\n"
    "ret       clr" "\n"
);
#endif // COMPILER(GCC_COMPATIBLE)

void MacroAssembler::probe(Probe::Function function, void* arg)
{
    subPtr(TrustedImm32(sizeof(IncomingProbeRecord)), csp);

    storePairCap(c24, c25, csp, TrustedImm32(offsetof(IncomingProbeRecord, c24)));
    storePairCap(c26, c27, csp, TrustedImm32(offsetof(IncomingProbeRecord, c26)));
    storePairCap(c28, c30, csp, TrustedImm32(offsetof(IncomingProbeRecord, c28))); // Note: c30 is clr.
    move(TrustedImmPtr(reinterpret_cast<void*>(ctiMasmProbeTrampoline)), c26);
    move(TrustedImmPtr(reinterpret_cast<void*>(Probe::executeProbe)), c28);
    move(TrustedImmPtr(reinterpret_cast<void*>(function)), c24);
    move(TrustedImmPtr(arg), c25);
    call(c26, CFunctionPtrTag);

    // ctiMasmProbeTrampoline should have restored every register except for lr and the sp.
    loadPtr(Address(csp, offsetof(LRRestorationRecord, clr)), clr);
    addPtr(TrustedImm32(sizeof(LRRestorationRecord)), csp);
}

#endif // ENABLE(MASM_PROBE)

} // namespace JSC

#endif // ENABLE(ASSEMBLER) && CPU(ARM64)

