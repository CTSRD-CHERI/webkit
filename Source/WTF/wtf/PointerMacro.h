/**
 * Copyright (C) 2019 Arm Ltd. All rights reserved.
 */

#ifndef WTF_PointerMacro_h
#define WTF_PointerMacro_h

#include <wtf/Platform.h>
#include <wtf/Assertions.h>

#include <type_traits>

#ifdef __cplusplus

namespace WTF {

struct Pointer
{
public:
    template<unsigned lowBitsMask, typename PointerT>
    static inline uint32_t getLowBits(PointerT ptr) {
        COMPILE_ASSERT(lowBitsMask <= 63, "Cannot use more than the low 6 pointer bits");
        COMPILE_ASSERT(std::is_pointer<PointerT>::value
                       || (std::is_same<PointerT, uintptr_t>::value)
                       || (std::is_same<PointerT, intptr_t>::value),
                       "The argument should be a pointer value");

#pragma clang diagnostic push
#ifdef __CHERI_PURE_CAPABILITY__
#pragma clang diagnostic ignored "-Wcheri-bitwise-operations"
#endif
        // The additional bits are stored using bitwise or -> they are stored in the
        // offset field. Note: extracting them with bitwise and returns a
        // LHS-derived capability, so we only want to return the offset of that
        // result. The simple approach of `if ((x & 3) == 3)` would always return
        // false since the LHS of the == is a valid capability with offset 3 and
        // the RHS is an untagged intcap_t with offset 3
        // See https://github.com/CTSRD-CHERI/clang/issues/189
        uintptr_t result = ((uintptr_t) ptr) & lowBitsMask;
        // Return the offset or the address depending on the compiler mode
#pragma clang diagnostic pop

        return static_cast<uint32_t>(result);
    }

    template<unsigned lowBitsMask, typename PointerT>
    static inline PointerT clearLowBits(PointerT ptr) {
        COMPILE_ASSERT(lowBitsMask <= 63, "Cannot use more than the low 6 pointer bits");
        constexpr uintptr_t clearingMask = ~uintptr_t(lowBitsMask);
        COMPILE_ASSERT(ptrdiff_t(clearingMask) < 0, "");
        COMPILE_ASSERT(std::is_pointer<PointerT>::value
                       || (std::is_same<PointerT, uintptr_t>::value)
                       || (std::is_same<PointerT, intptr_t>::value),
                       "The argument should be a pointer value");

        // See https://github.com/CTSRD-CHERI/clang/issues/189
#pragma clang diagnostic push
#ifdef __CHERI_PURE_CAPABILITY__
#pragma clang diagnostic ignored "-Wcheri-bitwise-operations"
#endif
        PointerT result = (PointerT) (((uintptr_t) ptr) & clearingMask);
#pragma clang diagnostic pop

        // Bitwise operations on __uintcap_t always operate on the offset field
#ifdef __CHERI_PURE_CAPABILITY__
        ASSERT(__builtin_cheri_base_get(reinterpret_cast<void*>(ptr)) ==
               __builtin_cheri_base_get(reinterpret_cast<void*>(result)));
#endif

        return result;
    }

    // This one is not a template since unlike the mask values the bits parameter
    // might not be a compile-time constant
    // XXXAR: this function is not actually needed since bitwise or works
    // as expected but I added it for symmetry.
    template<typename PointerT>
    static inline PointerT setLowBits(PointerT ptr, uintptr_t bits) {
        COMPILE_ASSERT(std::is_pointer<PointerT>::value
                       || (std::is_same<PointerT, uintptr_t>::value)
                       || (std::is_same<PointerT, intptr_t>::value),
                       "The argument should be a pointer value");
        ASSERT(bits <= 63 && "Cannot use more than the low 6 pointer bits");

        uintptr_t ptrUint = (uintptr_t) ptr;
        ptrUint |= bits;
        return (PointerT) ptrUint;
    }

private:
    Pointer() =delete;
};

} // namespace WTF

#endif /* __cplusplus */

#endif /* WTF_PointerMacro_h */
