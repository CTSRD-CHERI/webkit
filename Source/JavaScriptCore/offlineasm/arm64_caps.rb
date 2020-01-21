# Copyright (C) 2011-2019 Apple Inc. All rights reserved.
# Copyright (C) 2014 University of Szeged. All rights reserved.
# Copyright (C) 2020 Arm Ltd. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

require "ast"
require "opt"
require "risc"

# Naming conventions:
#
# c<number>  => GPR. This is both the generic name of the register, and the name used
#               to indicate that the register is used as capability register.
# x<number>  => GPR. This is both the generic name of the register, and the name used
#               to indicate that the register is used in 64-bit mode.
# w<number>  => GPR in 32-bit mode. This is the low 32-bits of the GPR. If it is
#               mutated then the high 32-bit part of the register is zero filled.
# q<number>  => FPR. This is the generic name of the register.
# d<number>  => FPR used as an IEEE 64-bit binary floating point number (i.e. double).
#
# GPR conventions, to match the baseline JIT:
#
#  c0  => t0, a0, r0
#  c1  => t1, a1, r1
#  c2  => t2, a2
#  c3  => t3, a3
#  c4  => t4
#  c5  => t5
#
# c19  =>             csr0
# c20  =>             csr1
# c21  =>             csr2
# c22  =>             csr3 (numberTag)
# c23  =>             csr4 (notCellMask)
# c24  =>             csr5 (ContinuousArenaMalloc::...)
# c25  =>             csr6 (metadataTable)
# c26  =>             csr7 (PB)
# c27  =>             csr8 (DDC)
# c28  =>             csr9
# c29  => cfr
# csp  => sp
# clr  => lr
#
# FPR conventions, to match the baseline JIT:
#
#  q0  => ft0, fa0, fr
#  q1  => ft1, fa1
#  q2  => ft2, fa2
#  q3  => ft3, fa3
#  q4  => ft4          (unused in baseline)
#  q5  => ft5          (unused in baseline)
#  q8  => csfr0        (Only the lower 64 bits)
#  q9  => csfr1        (Only the lower 64 bits)
# q10  => csfr2        (Only the lower 64 bits)
# q11  => csfr3        (Only the lower 64 bits)
# q12  => csfr4        (Only the lower 64 bits)
# q13  => csfr5        (Only the lower 64 bits)
# q14  => csfr6        (Only the lower 64 bits)
# q15  => csfr7        (Only the lower 64 bits)
# q31  => scratch

def arm64capsGPRName(name, kind)
    raise "bad GPR name #{name}" unless name =~ /^[cx]/
    number = name[1..-1]
    case kind
    when :word
        "w" + number
    when :ptr
        "c" + number
    when :quad
        "x" + number
    else
        raise "Wrong kind: #{kind}"
    end
end

def arm64capsFPRName(name, kind)
    raise "bad FPR kind #{kind}" unless kind == :double
    raise "bad FPR name #{name}" unless name =~ /^q/
    "d" + name[1..-1]
end

class SpecialRegister
    def arm64capsOperand(kind)
        case @name
        when /^x/
            arm64capsGPRName(@name, kind)
        when /^c/
            arm64capsGPRName(@name, kind)
        when /^q/
            arm64capsFPRName(@name, kind)
        else
            raise "Bad name: #{@name}"
        end
    end
end

ARM64_CAPS_EXTRA_GPRS = [SpecialRegister.new("c6"), SpecialRegister.new("c7")]
ARM64_CAPS_EXTRA_FPRS = [SpecialRegister.new("q31")]

class RegisterID
    def arm64capsOperand(kind)
        case @name
        when 't0', 'a0', 'r0'
            arm64capsGPRName('c0', kind)
        when 't1', 'a1', 'r1'
            arm64capsGPRName('c1', kind)
        when 't2', 'a2'
            arm64capsGPRName('c2', kind)
        when 't3', 'a3'
            arm64capsGPRName('c3', kind)
        when 't4'
            arm64capsGPRName('c4', kind)
        when 't5'
          arm64capsGPRName('c5', kind)
        when 'cfr'
            arm64capsGPRName('c29', kind)
        when 'csr0'
            arm64capsGPRName('c19', kind)
        when 'csr1'
            arm64capsGPRName('c20', kind)
        when 'csr2'
            arm64capsGPRName('c21', kind)
        when 'csr3'
            arm64capsGPRName('c22', kind)
        when 'csr4'
            arm64capsGPRName('c23', kind)
        when 'csr5'
            arm64capsGPRName('c24', kind)
        when 'csr6'
            arm64capsGPRName('c25', kind)
        when 'csr7'
            arm64capsGPRName('c26', kind)
        when 'DDC', 'csr8'
            arm64capsGPRName('c27', kind)
        when 'csr9'
            arm64capsGPRName('c28', kind)
        when 'sp'
            if kind == :ptr
                'csp'
            else
                'sp'
            end
        when 'lr'
            'clr'
        else
            raise "Bad register name #{@name} at #{codeOriginString}"
        end
    end

    def isSP
        name == 'sp'
    end
end

class FPRegisterID
    def arm64capsOperand(kind)
        case @name
        when 'ft0', 'fr', 'fa0'
            arm64capsFPRName('q0', kind)
        when 'ft1', 'fa1'
            arm64capsFPRName('q1', kind)
        when 'ft2', 'fa2'
            arm64capsFPRName('q2', kind)
        when 'ft3', 'fa3'
            arm64capsFPRName('q3', kind)
        when 'ft4'
            arm64capsFPRName('q4', kind)
        when 'ft5'
            arm64capsFPRName('q5', kind)
        when 'csfr0'
            arm64capsFPRName('q8', kind)
        when 'csfr1'
            arm64capsFPRName('q9', kind)
        when 'csfr2'
            arm64capsFPRName('q10', kind)
        when 'csfr3'
            arm64capsFPRName('q11', kind)
        when 'csfr4'
            arm64capsFPRName('q12', kind)
        when 'csfr5'
            arm64capsFPRName('q13', kind)
        when 'csfr6'
            arm64capsFPRName('q14', kind)
        when 'csfr7'
            arm64capsFPRName('q15', kind)
        else "Bad register name #{@name} at #{codeOriginString}"
        end
    end
end

class Immediate
    def arm64capsOperand(kind)
        raise "Invalid immediate #{value} at #{codeOriginString}" if value < 0 or value > 4095
        "\##{value}"
    end
end

class Address
    def arm64capsOperand(kind)
        if compressedBaseFlag.value == 0
	    baseType = :ptr
            raise "Invalid offset #{offset.value} at #{codeOriginString}" if offset.value < -255 or offset.value > 4095
	else
	    baseType = :quad
            raise "Invalid offset #{offset.value} at #{codeOriginString}" if offset.value < -128 or offset.value > 127
	end

        "[#{base.arm64capsOperand(baseType)}, \##{offset.value}]"
    end

    def arm64capsEmitLea(destination, kind)
        $asm.puts "add #{destination.arm64capsOperand(kind)}, #{base.arm64capsOperand(kind)}, \##{offset.value}"
    end
end

class BaseIndex
    def arm64capsOperand(kind)
        if compressedBaseFlag.value == 0
	    baseType = :ptr
	else
	    baseType = :quad
	end

        raise "Invalid offset #{offset.value} at #{codeOriginString}" if offset.value != 0
        "[#{base.arm64capsOperand(baseType)}, #{index.arm64capsOperand(:quad)}, lsl \##{scaleShift}]"
    end

    def arm64capsEmitLea(destination, kind)
        if kind == :ptr
            indexKind = :quad
        else
            indexKind = kind
        end

        original_dst = destination

        if scaleShift != 0
            scaleOperand = ", lsl \##{scaleShift}"
            if kind == :ptr and scaleShift == 3
                raise "Base is being overriden. Can't use it to reconstruct capability." unless destination != base
                $asm.puts "lsl #{destination.arm64capsOperand(:quad)}, #{index.arm64capsOperand(indexKind)}, \##{scaleShift}"
                $asm.puts "add #{destination.arm64capsOperand(kind)}, #{base.arm64capsOperand(kind)}, #{destination.arm64capsOperand(:quad)}"
                return
            end
        else
            scaleOperand = ""
        end
        $asm.puts "add #{destination.arm64capsOperand(kind)}, #{base.arm64capsOperand(kind)}, #{index.arm64capsOperand(indexKind)} #{scaleOperand}"
    end
end

class AbsoluteAddress
    def arm64capsOperand(kind)
        raise "Unconverted absolute address #{address.value} at #{codeOriginString}"
    end
end

# FIXME: We could support AbsoluteAddress for lea, but we don't.

#
# Actual lowering code follows.
#

def arm64capsLowerMalformedLoadStoreAddresses(list)
    newList = []

    def isAddressMalformed(opcode, operand)
        malformed = false
        if operand.is_a? Address
            if opcode =~ /p$/
	        if operand.compressedBaseFlag.value == 0
                    malformed ||= (not (0..4095).include? operand.offset.value)
                else
                    malformed ||= (not (-128..127).include? operand.offset.value)
		end
            else
	        if operand.compressedBaseFlag.value == 0
                    malformed ||= (not (-255..4095).include? operand.offset.value)
                else
                    malformed ||= (not (-32..31).include? operand.offset.value)
		end
            end
            if opcode =~ /q$/
                malformed ||= operand.offset.value % 8
            end
        end
        malformed
    end

    list.each {
        | node |
        if node.is_a? Instruction
            if node.opcode =~ /^store/ and isAddressMalformed(node.opcode, node.operands[1])
                address = node.operands[1]
                tmp = Tmp.new(codeOrigin, :gpr)
                newList << Instruction.new(node.codeOrigin, "move", [address.offset, tmp])
                newList << Instruction.new(node.codeOrigin, node.opcode, [node.operands[0], BaseIndex.new(node.codeOrigin, address.base, tmp, Immediate.new(codeOrigin, 1), Immediate.new(codeOrigin, 0), address.compressedBaseFlag)], node.annotation)
            elsif node.opcode =~ /^load/ and isAddressMalformed(node.opcode, node.operands[0])
                address = node.operands[0]
                tmp = Tmp.new(codeOrigin, :gpr)
                newList << Instruction.new(node.codeOrigin, "move", [address.offset, tmp])
                newList << Instruction.new(node.codeOrigin, node.opcode, [BaseIndex.new(node.codeOrigin, address.base, tmp, Immediate.new(codeOrigin, 1), Immediate.new(codeOrigin, 0), address.compressedBaseFlag), node.operands[1]], node.annotation)
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end

def arm64capsLowerLabelReferences(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            case node.opcode
            when "loadi", "loadis", "loadp", "loadq", "loadv", "loadvmc", "loadb", "loadbsi", "loadbsq", "loadh", "loadhsi", "loadhsq", "leap"
                labelRef = node.operands[0]
                if labelRef.is_a? LabelReference
                    tmp = Tmp.new(node.codeOrigin, :gpr)
                    newList << Instruction.new(codeOrigin, "globaladdr", [LabelReference.new(node.codeOrigin, labelRef.label), tmp])
                    newList << Instruction.new(codeOrigin, node.opcode, [Address.new(node.codeOrigin, tmp, Immediate.new(node.codeOrigin, labelRef.offset)), node.operands[1]])
                else
                    newList << node
                end
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end

def arm64capsLowerMalformedAnd(list)
    newList = []

    list.each {
        | node |
        if node.is_a? Instruction and node.opcode =~ /^and/
            if node.operands.size == 3
                dst = node.operands[2]
                src1 = node.operands[0]
                src2 = node.operands[1]
            else
                dst = node.operands[1]
                src1 = node.operands[1]
                src2 = node.operands[0]
            end

            if src2.immediate? and src2.value == 0 # XXXBFG should this be ~0?
                if src1 != dst
                    if node.opcode == "andp"
                        move_opcode = "movep"
                    else
                        move_opcode = "move"
                    end

                    newList << Instruction.new(node.codeOrigin, move_opcode, [src1, dst])
                end
            elsif node.opcode == "andp"
                original_dst = dst

                if dst == src1 or dst == src2 # XXXBFG do we only have to use a tmp when dst == src1?
                    dst = Tmp.new(codeOrigin, :gpr)
                end

                newList << Instruction.new(node.codeOrigin, "andq", [src1, src2, dst])
                newList << Instruction.new(node.codeOrigin, "cvtz", [src1, dst, original_dst])
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end

def arm64capsLowerMalformedSub(list)
    newList = []

    list.each {
        | node |
        if node.is_a? Instruction
            if node.opcode == "subp"
                if node.operands.size == 3
                    dst = node.operands[2]
                    src1 = node.operands[0]
                    src2 = node.operands[1]
                else
                    dst = node.operands[1]
                    src1 = node.operands[1]
                    src2 = node.operands[0]
                end

                tmp = Tmp.new(codeOrigin, :gpr)

                if src1.is_a? RegisterID and src1.isSP
                    newList << Instruction.new(node.codeOrigin, "movep", [src1, tmp])
                    src1 = tmp
                end

                original_dst = dst

                if dst.isSP or (dst == src1 and not src2.is_a? Immediate)
                    dst = tmp
                end

                if not src2.is_a? Immediate
                    newList << Instruction.new(node.codeOrigin, "subq", [src1, src2, dst], node.annotation)
                    newList << Instruction.new(node.codeOrigin, "cvtz", [src1, dst, dst])
                else
                    newList << Instruction.new(node.codeOrigin, "subp", [src1, src2, dst], node.annotation)
                end

                if original_dst != dst
                    newList << Instruction.new(node.codeOrigin, "movep", [dst, original_dst])
                end
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end

class Sequence
    def getModifiedListARM64_CAPS(result = @list)
        result = riscLowerNot(result)
        result = riscLowerSimpleBranchOps(result)

        result = riscLowerHardBranchOps64(result)
        result = riscLowerShiftOps(result)
        result = arm64capsLowerMalformedLoadStoreAddresses(result)
        result = arm64capsLowerLabelReferences(result)
        result = arm64capsLowerMalformedAnd(result)
        result = riscLowerMalformedAddresses(result) {
            | node, address |
            case node.opcode
            when "loadb", "loadbsi", "loadbsq", "storeb", /^bb/, /^btb/, /^cb/, /^tb/
                size = 1
            when "loadh", "loadhsi", "loadhsq"
                size = 2
            when "loadi", "loadis", "storei", "addi", "andi", "lshifti", "muli", "negi",
                "noti", "ori", "rshifti", "urshifti", "subi", "xori", /^bi/, /^bti/,
                /^ci/, /^ti/, "addis", "subis", "mulis", "smulli", "leai", "printi"
                size = 4
            when "loadq", "storeq", "loadd", "stored", "lshiftq", "negq", "rshiftq",
                "urshiftq", "addq", "mulq", "andq", "orq", "subq", "xorq", "addd",
                "divd", "subd", "muld", "sqrtd", /^bq/, /^btq/, /^cq/, /^tq/, /^bd/,
                "leaq"
                size = 8
            when "loadp", "storep", "lshiftp", "negp", "rshiftp",
                "urshiftp", "addp", "mulp", "andp", "orp", "subp", "xorp",
                /^bp/, /^btp/, /^cp/, /^tp/,
                "jmp", "call", "leap", "printp"
                size = 16
            when "loadv", "loadvmc", "storev"
                if $options.has_key?(:jsheap_cheri_offset_refs)
                    size = 8
                else
                    size = 16
                end
            else
                raise "Bad instruction #{node.opcode} for heap access at #{node.codeOriginString}: #{node.dump}"
            end

            if address.is_a? BaseIndex
                address.offset.value == 0 and
                    (node.opcode =~ /^lea/ or address.scale == 1 or address.scaleValue == size)
            elsif address.is_a? Address
                if size == 16
	            if address.compressedBaseFlag.value == 0
                        (0..4095).include? address.offset.value
                    else
                        (-128..127).include? address.offset.value
	            end
                else
	            if address.compressedBaseFlag.value == 0
                        (-255..4095).include? address.offset.value
                    else
                        (-32..31).include? address.offset.value
	            end
                end
            else
                false
            end
        }
        result = riscLowerMisplacedImmediates(result, ["storeb", "storei", "storep", "storeq", "storev"])
        result = riscLowerMalformedImmediates(result, 0..4095)
        result = arm64capsLowerMalformedSub(result)
        result = riscLowerMisplacedAddresses(result)
        result = riscLowerMalformedAddresses(result) {
            | node, address |
            case node.opcode
            when /^load/
                true
            when /^store/
                not (address.is_a? Address and address.offset.value < 0)
            when /^lea/
                true
            when /^print/
                true
            else
                raise "Bad instruction #{node.opcode} for heap access at #{node.codeOriginString}"
            end
        }
        result = riscLowerTest(result)
        result = assignRegistersToTemporaries(result, :gpr, ARM64_CAPS_EXTRA_GPRS)
        result = assignRegistersToTemporaries(result, :fpr, ARM64_CAPS_EXTRA_FPRS)
        return result
    end
end

def arm64capsOperands(operands, kinds)
    if kinds.is_a? Array
        raise "Mismatched operand lists: #{operands.inspect} and #{kinds.inspect}" if operands.size != kinds.size
    else
        kinds = operands.map{ kinds }
    end
    (0...operands.size).map {
        | index |
        operands[index].arm64capsOperand(kinds[index])
    }.join(', ')
end

def arm64capsFlippedOperands(operands, kinds)
    if kinds.is_a? Array
        kinds = [kinds[-1]] + kinds[0..-2]
    end
    arm64capsOperands([operands[-1]] + operands[0..-2], kinds)
end

# TAC = three address code.
def arm64capsTACOperands(operands, kind)
    if operands.size == 3
        return arm64capsFlippedOperands(operands, kind)
    end

    raise unless operands.size == 2

    if kind.is_a? Array
        kind1 = kind[1]
    else
        kind1 = kind
    end

    return operands[1].arm64capsOperand(kind1) + ", " + arm64capsFlippedOperands(operands, kind)
end

def emitARM64CAPSMakeCapabilityFromPointer(operands)
    if operands.size == 3
        src1 = operands[0]
        src2 = operands[1]
        dst = operands[2]
    else
        src1 = operands[0]
        src2 = operands[1]
        dst = operands[1]
    end

    emitARM64CAPSTAC("cvtz", [src1, src2, dst], [:ptr, :quad, :ptr])
end

def emitARM64CAPSAdd(opcode, operands, kind)
    if kind == :ptr
        if operands.size == 3
            if operands[0].immediate?
                kind = [:ptr, :ptr, :quad]
            else
                kind = [:ptr, :quad, :ptr]
            end
        else
            kind = [:quad, :ptr]
        end
    end

    if operands.size == 3
        raise unless operands[1].register?
        raise unless operands[2].register?

        if operands[0].immediate?
            if operands[0].value == 0 and opcode !~ /s$/
                if operands[1] != operands[2]
                    $asm.puts "mov #{arm64capsFlippedOperands(operands[1..2], kind)}"
                end
            else
                $asm.puts "#{opcode} #{arm64capsOperands(operands.reverse, kind)}"
            end
            return
        end

        raise unless operands[0].register?
        $asm.puts "#{opcode} #{arm64capsFlippedOperands(operands, kind)}"
        return
    end

    raise unless operands.size == 2

    if operands[0].immediate? and operands[0].value == 0 and opcode !~ /s$/
        return
    end

    $asm.puts "#{opcode} #{arm64capsTACOperands(operands, kind)}"
end

def emitARM64CAPSMul(opcode, operands, kind)
    if operands.size == 2 and operands[0].is_a? Immediate
        imm = operands[0].value
        if imm > 0 and isPowerOfTwo(imm)
            emitARM64CAPSLShift([Immediate.new(nil, Math.log2(imm).to_i), operands[1]], kind)
            return
        end
    end

    $asm.puts "madd #{arm64capsTACOperands(operands, kind)}, #{arm64capsGPRName('xzr', kind)}"
end

def emitARM64CAPSSub(opcode, operands, kind)
    if kind == :ptr
        if operands.size == 3
            kind = [:ptr, :quad, :ptr]

            deduction = operands[1]
        else
            kind = [:quad, :ptr]

            deduction = operands[0]
        end

        raise "Deduction should be immediate for subp" unless deduction.is_a? Immediate
    else
        kind = Array.new(operands.size, kind)
    end

    if operands.size == 3
        raise unless operands[0].register?
        raise unless operands[2].register?

        if operands[1].immediate?
            if operands[1].value == 0 and opcode !~ /s$/
                kind = [kind[0], kind[2]]

                if operands[0] != operands[2]
                    $asm.puts "mov #{arm64capsFlippedOperands([operands[0], operands[2]], kind)}"
                end
                return
            end
        end
    end

    if operands.size == 2
        if operands[0].immediate? and operands[0].value == 0 and opcode !~ /s$/
            return
        end
    end

    emitARM64CAPSTAC(opcode, operands, kind)
end

def emitARM64CAPSUnflipped(opcode, operands, kind)
    $asm.puts "#{opcode} #{arm64capsOperands(operands, kind)}"
end

def emitARM64CAPSTAC(opcode, operands, kind)
    $asm.puts "#{opcode} #{arm64capsTACOperands(operands, kind)}"
end

def emitARM64CAPS(opcode, operands, kind)
    $asm.puts "#{opcode} #{arm64capsFlippedOperands(operands, kind)}"
end

def emitARM64CAPSAccess(opcode, opcodeNegativeOffset, register, memory, kind)
    if memory.is_a? Address and memory.offset.value < 0
        raise unless -256 <= memory.offset.value
        $asm.puts "#{opcodeNegativeOffset} #{register.arm64capsOperand(kind)}, #{memory.arm64capsOperand(kind)}"
        return
    end

    $asm.puts "#{opcode} #{register.arm64capsOperand(kind)}, #{memory.arm64capsOperand(kind)}"
end

def emitARM64CAPSShift(opcodeRegs, opcodeImmediate, operands, kind)
    if operands.size == 3 and operands[1].immediate?
        magicNumbers = yield operands[1].value
        $asm.puts "#{opcodeImmediate} #{operands[2].arm64capsOperand(kind)}, #{operands[0].arm64capsOperand(kind)}, \##{magicNumbers[0]}, \##{magicNumbers[1]}"
        return
    end

    if operands.size == 2 and operands[0].immediate?
        magicNumbers = yield operands[0].value
        $asm.puts "#{opcodeImmediate} #{operands[1].arm64capsOperand(kind)}, #{operands[1].arm64capsOperand(kind)}, \##{magicNumbers[0]}, \##{magicNumbers[1]}"
        return
    end

    emitARM64CAPSTAC(opcodeRegs, operands, kind)
end

def emitARM64CAPSLShift(operands, kind)
    emitARM64CAPSShift("lslv", "ubfm", operands, kind) {
        | value |
        case kind
        when :word
            [32 - value, 31 - value]
        when :ptr
            bitSize = 64
            [bitSize - value, bitSize - 1 - value]
        when :quad
            [64 - value, 63 - value]
        end
    }
end

def emitARM64CAPSBranch(opcode, operands, kind, branchOpcode)
    emitARM64CAPSUnflipped(opcode, operands[0..-2], kind)
    $asm.puts "#{branchOpcode} #{operands[-1].asmLabel}"
end

def emitARM64CAPSCompare(operands, kind, compareCode)
    emitARM64CAPSUnflipped("subs #{arm64capsGPRName('xzr', kind)}, ", operands[0..-2], kind)
    $asm.puts "csinc #{operands[-1].arm64capsOperand(:word)}, wzr, wzr, #{compareCode}"
end

def emitARM64CAPSMoveImmediate(value, target)
    first = true
    isNegative = value < 0
    [48, 32, 16, 0].each {
        | shift |
        currentValue = (value >> shift) & 0xffff
        next if currentValue == (isNegative ? 0xffff : 0) and (shift != 0 or !first)
        if first
            if isNegative
                $asm.puts "movn #{target.arm64capsOperand(:quad)}, \##{(~currentValue) & 0xffff}, lsl \##{shift}"
            else
                $asm.puts "movz #{target.arm64capsOperand(:quad)}, \##{currentValue}, lsl \##{shift}"
            end
            first = false
        else
            $asm.puts "movk #{target.arm64capsOperand(:quad)}, \##{currentValue}, lsl \##{shift}"
        end
    }
end

class Instruction
    def lowerARM64_CAPS
        case opcode
        when 'cvtz'
            emitARM64CAPSMakeCapabilityFromPointer(operands)
        when 'addi'
            emitARM64CAPSAdd("add", operands, :word)
        when 'addis'
            emitARM64CAPSAdd("adds", operands, :word)
        when 'addp'
            emitARM64CAPSAdd("add", operands, :ptr)
        when 'addps'
            emitARM64CAPSAdd("adds", operands, :ptr)
        when 'addq'
            emitARM64CAPSAdd("add", operands, :quad)
        when 'addqs'
            emitARM64CAPSAdd("adds", operands, :quad)
        when "andi"
            emitARM64CAPSTAC("and", operands, :word)
        when "andq"
            emitARM64CAPSTAC("and", operands, :quad)
        when "ori"
            emitARM64CAPSTAC("orr", operands, :word)
        when "orp"
            # FIXME: Consider removing this, if we don't need it.
            raise "ARM64_CAPS does not support this opcode yet, #{codeOriginString}"
        when "orq"
            emitARM64CAPSTAC("orr", operands, :quad)
        when "xori"
            emitARM64CAPSTAC("eor", operands, :word)
        when "xorp"
            # FIXME: Consider removing this, if we don't need it.
            raise "ARM64_CAPS does not support this opcode yet, #{codeOriginString}"
        when "xorq"
            emitARM64CAPSTAC("eor", operands, :quad)
        when "lshifti"
            emitARM64CAPSLShift(operands, :word)
        when "lshiftp"
            # FIXME: Consider removing this, if we don't need it.
            raise "ARM64_CAPS does not support this opcode yet, #{codeOriginString}"
        when "lshiftq"
            emitARM64CAPSLShift(operands, :quad)
        when "rshifti"
            emitARM64CAPSShift("asrv", "sbfm", operands, :word) {
                | value |
                [value, 31]
            }
        when "rshiftp"
            # FIXME: Consider remove this, and replacing uses with rshiftq. It
            # doesn't make sense to shift a capability, but we implement the
            # behaviour expected by arch-agnostic code.
            emitARM64CAPSShift("asrv", "sbfm", operands, :quad) {
                | value |
                bitSize = 64
                [value, bitSize - 1]
            }
        when "rshiftq"
            emitARM64CAPSShift("asrv", "sbfm", operands, :quad) {
                | value |
                [value, 63]
            }
        when "urshifti"
            emitARM64CAPSShift("lsrv", "ubfm", operands, :word) {
                | value |
                [value, 31]
            }
        when "urshiftp"
            # FIXME: Consider removing this, if we don't need it.
            raise "ARM64_CAPS does not support this opcode yet, #{codeOriginString}"
        when "urshiftq"
            emitARM64CAPSShift("lsrv", "ubfm", operands, :quad) {
                | value |
                [value, 63]
            }
        when "muli"
            emitARM64CAPSMul('mul', operands, :word)
        when "mulp"
            # FIXME: Consider removing this, if we don't need it.
            raise "ARM64_CAPS does not support this opcode yet, #{codeOriginString}"
        when "mulq"
            emitARM64CAPSMul('mul', operands, :quad)
        when "subi"
            emitARM64CAPSSub("sub", operands, :word)
        when "subp"
            emitARM64CAPSSub("sub", operands, :ptr)
        when "subq"
            emitARM64CAPSSub("sub", operands, :quad)
        when "subis"
            emitARM64CAPSSub("subs", operands, :word)
        when "negi"
            $asm.puts "sub #{operands[0].arm64capsOperand(:word)}, wzr, #{operands[0].arm64capsOperand(:word)}"
        when "negp"
            # FIXME: Is this useful? Consider removing it.
            $asm.puts "sub #{operands[0].arm64capsOperand(:ptr)}, #{arm64capsGPRName('xzr', :ptr)}, #{operands[0].arm64capsOperand(:ptr)}"
        when "negq"
            $asm.puts "sub #{operands[0].arm64capsOperand(:quad)}, xzr, #{operands[0].arm64capsOperand(:quad)}"
        when "loadi"
            emitARM64CAPSAccess("ldr", "ldur", operands[1], operands[0], :word)
        when "loadis"
            emitARM64CAPSAccess("ldrsw", "ldursw", operands[1], operands[0], :quad)
        when "loadp"
            emitARM64CAPSAccess("ldr", "ldur", operands[1], operands[0], :ptr)
        when "loadq"
            emitARM64CAPSAccess("ldr", "ldur", operands[1], operands[0], :quad)
        when "loadv"
            if $options.has_key?(:jsheap_cheri_offset_refs)
                emitARM64CAPSAccess("ldr", "ldur", operands[1], operands[0], :quad)
            else
                emitARM64CAPSAccess("ldr", "ldur", operands[1], operands[0], :ptr)
            end
        when "loadvmc"
            if $options.has_key?(:jsheap_cheri_offset_refs)
                emitARM64CAPSAccess("ldr", "ldur", operands[1], operands[0], :quad)
            else
                emitARM64CAPSAccess("ldr", "ldur", operands[1], operands[0], :ptr)
            end
        when "storei"
            emitARM64CAPSUnflipped("str", operands, :word)
        when "storep"
            emitARM64CAPSUnflipped("str", operands, :ptr)
        when "storeq"
            emitARM64CAPSUnflipped("str", operands, :quad)
        when "storev"
            if $options.has_key?(:jsheap_cheri_offset_refs)
                emitARM64CAPSUnflipped("str", operands, :quad)
            else
                emitARM64CAPSUnflipped("str", operands, :ptr)
            end
        when "loadb"
            emitARM64CAPSAccess("ldrb", "ldurb", operands[1], operands[0], :word)
        when "loadbsi"
            emitARM64CAPSAccess("ldrsb", "ldursb", operands[1], operands[0], :word)
        when "loadbsq"
            emitARM64CAPSAccess("ldrsb", "ldursb", operands[1], operands[0], :quad)
        when "storeb"
            emitARM64CAPSUnflipped("strb", operands, :word)
        when "loadh"
            emitARM64CAPSAccess("ldrh", "ldurh", operands[1], operands[0], :word)
        when "loadhsi"
            emitARM64CAPSAccess("ldrsh", "ldursh", operands[1], operands[0], :word)
        when "loadhsq"
            emitARM64CAPSAccess("ldrsh", "ldursh", operands[1], operands[0], :quad)
        when "storeh"
            emitARM64CAPSUnflipped("strh", operands, :word)
        when "loadd"
            emitARM64CAPSAccess("ldr", "ldur", operands[1], operands[0], :double)
        when "stored"
            emitARM64CAPSUnflipped("str", operands, :double)
        when "addd"
            emitARM64CAPSTAC("fadd", operands, :double)
        when "divd"
            emitARM64CAPSTAC("fdiv", operands, :double)
        when "subd"
            emitARM64CAPSTAC("fsub", operands, :double)
        when "muld"
            emitARM64CAPSTAC("fmul", operands, :double)
        when "sqrtd"
            emitARM64CAPS("fsqrt", operands, :double)
        when "ci2d"
            emitARM64CAPS("scvtf", operands, [:word, :double])
        when "bdeq"
            emitARM64CAPSBranch("fcmp", operands, :double, "b.eq")
        when "bdneq"
            emitARM64CAPSUnflipped("fcmp", operands[0..1], :double)
            isUnordered = LocalLabel.unique("bdneq")
            $asm.puts "b.vs #{LocalLabelReference.new(codeOrigin, isUnordered).asmLabel}"
            $asm.puts "b.ne #{operands[2].asmLabel}"
            isUnordered.lower("ARM64_CAPS")
        when "bdgt"
            emitARM64CAPSBranch("fcmp", operands, :double, "b.gt")
        when "bdgteq"
            emitARM64CAPSBranch("fcmp", operands, :double, "b.ge")
        when "bdlt"
            emitARM64CAPSBranch("fcmp", operands, :double, "b.mi")
        when "bdlteq"
            emitARM64CAPSBranch("fcmp", operands, :double, "b.ls")
        when "bdequn"
            emitARM64CAPSUnflipped("fcmp", operands[0..1], :double)
            $asm.puts "b.vs #{operands[2].asmLabel}"
            $asm.puts "b.eq #{operands[2].asmLabel}"
        when "bdnequn"
            emitARM64CAPSBranch("fcmp", operands, :double, "b.ne")
        when "bdgtun"
            emitARM64CAPSBranch("fcmp", operands, :double, "b.hi")
        when "bdgtequn"
            emitARM64CAPSBranch("fcmp", operands, :double, "b.pl")
        when "bdltun"
            emitARM64CAPSBranch("fcmp", operands, :double, "b.lt")
        when "bdltequn"
            emitARM64CAPSBranch("fcmp", operands, :double, "b.le")
        when "btd2i"
            # FIXME: May be a good idea to just get rid of this instruction, since the interpreter
            # currently does not use it.
            raise "ARM64_CAPS does not support this opcode yet, #{codeOriginString}"
        when "td2i"
            emitARM64CAPS("fcvtzs", operands, [:double, :word])
        when "bcd2i"
            # FIXME: Remove this instruction, or use it and implement it. Currently it's not
            # used.
            raise "ARM64_CAPS does not support this opcode yet, #{codeOriginString}"
        when "movdz"
            # FIXME: Remove it or support it.
            raise "ARM64_CAPS does not support this opcode yet, #{codeOriginString}"
        when "popq"
            operands.each_slice(2) {
                | ops |
                # Note that the operands are in the reverse order of the case for push.
                # This is due to the fact that order matters for pushing and popping, and
                # on platforms that only push/pop one slot at a time they pop their
                # arguments in the reverse order that they were pushed. In order to remain
                # compatible with those platforms we assume here that that's what has been done.

                # So for example, if we did push(A, B, C, D), we would then pop(D, C, B, A).
                # But since the ordering of arguments doesn't change on arm64caps between the stp and ldp
                # instructions we need to flip flop the argument positions that were passed to us.
                $asm.puts "ldp #{ops[1].arm64capsOperand(:quad)}, #{ops[0].arm64capsOperand(:quad)}, [sp], #16"
            }
        when "popp"
            operands.each_slice(2) {
                | ops |
                # Note that the operands are in the reverse order of the case for push.
                # This is due to the fact that order matters for pushing and popping, and
                # on platforms that only push/pop one slot at a time they pop their
                # arguments in the reverse order that they were pushed. In order to remain
                # compatible with those platforms we assume here that that's what has been done.

                # So for example, if we did push(A, B, C, D), we would then pop(D, C, B, A).
                # But since the ordering of arguments doesn't change on arm64caps between the stp and ldp
                # instructions we need to flip flop the argument positions that were passed to us.
                $asm.puts "ldp #{ops[1].arm64capsOperand(:ptr)}, #{ops[0].arm64capsOperand(:ptr)}, [csp], #32"
            }
        when "pushq"
            operands.each_slice(2) {
                | ops |
                $asm.puts "stp #{ops[0].arm64capsOperand(:quad)}, #{ops[1].arm64capsOperand(:quad)}, [csp, #-16]!"
            }
        when "pushp"
            operands.each_slice(2) {
                | ops |
                $asm.puts "stp #{ops[0].arm64capsOperand(:ptr)}, #{ops[1].arm64capsOperand(:ptr)}, [csp, #-32]!"
            }
        when "move"
            if operands[0].immediate?
                emitARM64CAPSMoveImmediate(operands[0].value, operands[1])
            else
                emitARM64CAPS("mov", operands, :quad)
            end
        when "movep"
            if operands[0].immediate?
                emitARM64CAPSMoveImmediate(operands[0].value, operands[1])
            else
                emitARM64CAPS("mov", operands, :ptr)
            end
        when "sxi2p"
            emitARM64CAPS("sxtw", operands, [:word, :ptr])
        when "sxi2q"
            emitARM64CAPS("sxtw", operands, [:word, :quad])
        when "zxi2p"
            emitARM64CAPS("uxtw", operands, [:word, :ptr])
        when "zxi2q"
            emitARM64CAPS("uxtw", operands, [:word, :quad])
        when "nop"
            $asm.puts "nop"
        when "bieq", "bbeq"
            if operands[0].immediate? and operands[0].value == 0
                $asm.puts "cbz #{operands[1].arm64capsOperand(:word)}, #{operands[2].asmLabel}"
            elsif operands[1].immediate? and operands[1].value == 0
                $asm.puts "cbz #{operands[0].arm64capsOperand(:word)}, #{operands[2].asmLabel}"
            else
                emitARM64CAPSBranch("subs wzr, ", operands, :word, "b.eq")
            end
        when "bpeq"
            if operands[0].immediate? and operands[0].value == 0
                $asm.puts "cbz #{operands[1].arm64capsOperand(:ptr)}, #{operands[2].asmLabel}"
            elsif operands[1].immediate? and operands[1].value == 0
                $asm.puts "cbz #{operands[0].arm64capsOperand(:ptr)}, #{operands[2].asmLabel}"
            else
                emitARM64CAPSBranch("subs #{arm64capsGPRName('xzr', :ptr)}, ", operands, :ptr, "b.eq")
            end
        when "bqeq"
            if operands[0].immediate? and operands[0].value == 0
                $asm.puts "cbz #{operands[1].arm64capsOperand(:quad)}, #{operands[2].asmLabel}"
            elsif operands[1].immediate? and operands[1].value == 0
                $asm.puts "cbz #{operands[0].arm64capsOperand(:quad)}, #{operands[2].asmLabel}"
            else
                emitARM64CAPSBranch("subs xzr, ", operands, :quad, "b.eq")
            end
        when "bineq", "bbneq"
            if operands[0].immediate? and operands[0].value == 0
                $asm.puts "cbnz #{operands[1].arm64capsOperand(:word)}, #{operands[2].asmLabel}"
            elsif operands[1].immediate? and operands[1].value == 0
                $asm.puts "cbnz #{operands[0].arm64capsOperand(:word)}, #{operands[2].asmLabel}"
            else
                emitARM64CAPSBranch("subs wzr, ", operands, :word, "b.ne")
            end
        when "bpneq"
            if operands[0].immediate? and operands[0].value == 0
                $asm.puts "cbnz #{operands[1].arm64capsOperand(:ptr)}, #{operands[2].asmLabel}"
            elsif operands[1].immediate? and operands[1].value == 0
                $asm.puts "cbnz #{operands[0].arm64capsOperand(:ptr)}, #{operands[2].asmLabel}"
            else
                emitARM64CAPSBranch("subs #{arm64capsGPRName('xzr', :ptr)}, ", operands, :ptr, "b.ne")
            end
        when "bqneq"
            if operands[0].immediate? and operands[0].value == 0
                $asm.puts "cbnz #{operands[1].arm64capsOperand(:quad)}, #{operands[2].asmLabel}"
            elsif operands[1].immediate? and operands[1].value == 0
                $asm.puts "cbnz #{operands[0].arm64capsOperand(:quad)}, #{operands[2].asmLabel}"
            else
                emitARM64CAPSBranch("subs xzr, ", operands, :quad, "b.ne")
            end
        when "bia", "bba"
            emitARM64CAPSBranch("subs wzr, ", operands, :word, "b.hi")
        when "bpa"
            emitARM64CAPSBranch("subs #{arm64capsGPRName('xzr', :ptr)}, ", operands, :ptr, "b.hi")
        when "bqa"
            emitARM64CAPSBranch("subs xzr, ", operands, :quad, "b.hi")
        when "biaeq", "bbaeq"
            emitARM64CAPSBranch("subs wzr, ", operands, :word, "b.hs")
        when "bpaeq"
            emitARM64CAPSBranch("subs #{arm64capsGPRName('xzr', :ptr)}, ", operands, :ptr, "b.hs")
        when "bqaeq"
            emitARM64CAPSBranch("subs xzr, ", operands, :quad, "b.hs")
        when "bib", "bbb"
            emitARM64CAPSBranch("subs wzr, ", operands, :word, "b.lo")
        when "bpb"
            emitARM64CAPSBranch("subs #{arm64capsGPRName('xzr', :quad)}, ", operands, :quad, "b.lo")
        when "bqb"
            emitARM64CAPSBranch("subs xzr, ", operands, :quad, "b.lo")
        when "bibeq", "bbbeq"
            emitARM64CAPSBranch("subs wzr, ", operands, :word, "b.ls")
        when "bpbeq"
            emitARM64CAPSBranch("subs #{arm64capsGPRName('xzr', :ptr)}, ", operands, :ptr, "b.ls")
        when "bqbeq"
            emitARM64CAPSBranch("subs xzr, ", operands, :quad, "b.ls")
        when "bigt", "bbgt"
            emitARM64CAPSBranch("subs wzr, ", operands, :word, "b.gt")
        when "bpgt"
            emitARM64CAPSBranch("subs #{arm64capsGPRName('xzr', :ptr)}, ", operands, :ptr, "b.gt")
        when "bqgt"
            emitARM64CAPSBranch("subs xzr, ", operands, :quad, "b.gt")
        when "bigteq", "bbgteq"
            emitARM64CAPSBranch("subs wzr, ", operands, :word, "b.ge")
        when "bpgteq"
            emitARM64CAPSBranch("subs #{arm64capsGPRName('xzr', :ptr)}, ", operands, :ptr, "b.ge")
        when "bqgteq"
            emitARM64CAPSBranch("subs xzr, ", operands, :quad, "b.ge")
        when "bilt", "bblt"
            emitARM64CAPSBranch("subs wzr, ", operands, :word, "b.lt")
        when "bplt"
            emitARM64CAPSBranch("subs #{arm64capsGPRName('xzr', :ptr)}, ", operands, :ptr, "b.lt")
        when "bqlt"
            emitARM64CAPSBranch("subs xzr, ", operands, :quad, "b.lt")
        when "bilteq", "bblteq"
            emitARM64CAPSBranch("subs wzr, ", operands, :word, "b.le")
        when "bplteq"
            emitARM64CAPSBranch("subs #{arm64capsGPRName('xzr', :ptr)}, ", operands, :ptr, "b.le")
        when "bqlteq"
            emitARM64CAPSBranch("subs xzr, ", operands, :quad, "b.le")
        when "jmp"
            if operands[0].label?
                $asm.puts "b #{operands[0].asmLabel}"
            else
                emitARM64CAPSUnflipped("br", operands, :ptr)
            end
        when "call"
            if operands[0].label?
                $asm.puts "bl #{operands[0].asmLabel}"
            else
                emitARM64CAPSUnflipped("blr", operands, :ptr)
            end
        when "break"
            $asm.puts "brk \#0"
        when "ret"
            $asm.puts "ret"
        when "cieq", "cbeq"
            emitARM64CAPSCompare(operands, :word, "ne")
        when "cpeq"
            emitARM64CAPSCompare(operands, :ptr, "ne")
        when "cqeq"
            emitARM64CAPSCompare(operands, :quad, "ne")
        when "cineq", "cbneq"
            emitARM64CAPSCompare(operands, :word, "eq")
        when "cpneq"
            emitARM64CAPSCompare(operands, :ptr, "eq")
        when "cqneq"
            emitARM64CAPSCompare(operands, :quad, "eq")
        when "cia", "cba"
            emitARM64CAPSCompare(operands, :word, "ls")
        when "cpa"
            emitARM64CAPSCompare(operands, :ptr, "ls")
        when "cqa"
            emitARM64CAPSCompare(operands, :quad, "ls")
        when "ciaeq", "cbaeq"
            emitARM64CAPSCompare(operands, :word, "lo")
        when "cpaeq"
            emitARM64CAPSCompare(operands, :ptr, "lo")
        when "cqaeq"
            emitARM64CAPSCompare(operands, :quad, "lo")
        when "cib", "cbb"
            emitARM64CAPSCompare(operands, :word, "hs")
        when "cpb"
            emitARM64CAPSCompare(operands, :ptr, "hs")
        when "cqb"
            emitARM64CAPSCompare(operands, :quad, "hs")
        when "cibeq", "cbbeq"
            emitARM64CAPSCompare(operands, :word, "hi")
        when "cpbeq"
            emitARM64CAPSCompare(operands, :ptr, "hi")
        when "cqbeq"
            emitARM64CAPSCompare(operands, :quad, "hi")
        when "cilt", "cblt"
            emitARM64CAPSCompare(operands, :word, "ge")
        when "cplt"
            emitARM64CAPSCompare(operands, :ptr, "ge")
        when "cqlt"
            emitARM64CAPSCompare(operands, :quad, "ge")
        when "cilteq", "cblteq"
            emitARM64CAPSCompare(operands, :word, "gt")
        when "cplteq"
            emitARM64CAPSCompare(operands, :ptr, "gt")
        when "cqlteq"
            emitARM64CAPSCompare(operands, :quad, "gt")
        when "cigt", "cbgt"
            emitARM64CAPSCompare(operands, :word, "le")
        when "cpgt"
            emitARM64CAPSCompare(operands, :ptr, "le")
        when "cqgt"
            emitARM64CAPSCompare(operands, :quad, "le")
        when "cigteq", "cbgteq"
            emitARM64CAPSCompare(operands, :word, "lt")
        when "cpgteq"
            emitARM64CAPSCompare(operands, :ptr, "lt")
        when "cqgteq"
            emitARM64CAPSCompare(operands, :quad, "lt")
        when "peek"
            $asm.puts "ldr #{operands[1].arm64capsOperand(:quad)}, [sp, \##{operands[0].value * 8}]"
        when "poke"
            $asm.puts "str #{operands[1].arm64capsOperand(:quad)}, [sp, \##{operands[0].value * 8}]"
        when "fp2d"
            emitARM64CAPS("fmov", operands, [:ptr, :double])
        when "fq2d"
            emitARM64CAPS("fmov", operands, [:quad, :double])
        when "fd2p"
            emitARM64CAPS("fmov", operands, [:double, :ptr])
        when "fd2q"
            emitARM64CAPS("fmov", operands, [:double, :quad])
        when "bo"
            $asm.puts "b.vs #{operands[0].asmLabel}"
        when "bs"
            $asm.puts "b.mi #{operands[0].asmLabel}"
        when "bz"
            $asm.puts "b.eq #{operands[0].asmLabel}"
        when "bnz"
            $asm.puts "b.ne #{operands[0].asmLabel}"
        when "leai"
            operands[0].arm64capsEmitLea(operands[1], :word)
        when "leap"
            operands[0].arm64capsEmitLea(operands[1], :ptr)
        when "leaq"
            operands[0].arm64capsEmitLea(operands[1], :quad)
        when "smulli"
            $asm.puts "smaddl #{operands[2].arm64capsOperand(:quad)}, #{operands[0].arm64capsOperand(:word)}, #{operands[1].arm64capsOperand(:word)}, xzr"
        when "memfence"
            $asm.puts "dmb sy"
        when "bfiq"
            $asm.puts "bfi #{operands[3].arm64capsOperand(:quad)}, #{operands[0].arm64capsOperand(:quad)}, #{operands[1].value}, #{operands[2].value}"
        when "pcrtoaddr"
            $asm.puts "adrp #{operands[1].arm64capsOperand(:ptr)}, #{operands[0].value}"
            $asm.puts "add #{operands[1].arm64capsOperand(:ptr)}, #{operands[1].arm64capsOperand(:ptr)}, #:lo12:#{operands[0].value}"
        when "globaladdr"
            uid = $asm.newUID

            # On Darwin, use Macho-O GOT relocation specifiers, along with
            # the labels required for the .loh directive.
            $asm.putStr("#if OS(DARWIN)")
            $asm.puts "L_offlineasm_loh_adrp_#{uid}:"
            $asm.puts "adrp #{operands[1].arm64capsOperand(:ptr)}, #{operands[0].asmLabel}@GOTPAGE"
            $asm.puts "L_offlineasm_loh_ldr_#{uid}:"
            $asm.puts "ldr #{operands[1].arm64capsOperand(:ptr)}, [#{operands[1].arm64capsOperand(:ptr)}, #{operands[0].asmLabel}@GOTPAGEOFF]"

            # On Linux/FreeBSD, use ELF GOT relocation specifiers.
            $asm.putStr("#elif OS(LINUX) || OS(FREEBSD)")
            $asm.puts "adrp #{operands[1].arm64capsOperand(:ptr)}, :got:#{operands[0].asmLabel}"
            $asm.puts "ldr #{operands[1].arm64capsOperand(:ptr)}, [#{operands[1].arm64capsOperand(:ptr)}, :got_lo12:#{operands[0].asmLabel}]"

            # Throw a compiler error everywhere else.
            $asm.putStr("#else")
            $asm.putStr("#error Missing globaladdr implementation")
            $asm.putStr("#endif")

            $asm.deferAction {
                # On Darwin, also include the .loh directive using the generated labels.
                $asm.putStr("#if OS(DARWIN)")
                $asm.puts ".loh AdrpLdrGot L_offlineasm_loh_adrp_#{uid}, L_offlineasm_loh_ldr_#{uid}"
                $asm.putStr("#endif")
            }
        when "print", "printi", "printb", "printq", "printp", "printc"
            $asm.putStr("/* print instructions not supported in arm64caps llint */")
        when "makecap"
            $asm.putStr("/* makecap instruction is NOP */")
        else
            lowerDefault
        end
    end
end

