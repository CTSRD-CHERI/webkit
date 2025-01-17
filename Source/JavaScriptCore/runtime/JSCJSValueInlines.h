/*
 * Copyright (C) 2011-2019 Apple Inc. All rights reserved.
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

#include "CatchScope.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "Identifier.h"
#include "InternalFunction.h"
#include "JSBigInt.h"
#include "JSCJSValue.h"
#include "JSCellInlines.h"
#include "JSFunction.h"
#include "JSObject.h"
#include "JSProxy.h"
#include "JSStringInlines.h"
#include "MathCommon.h"
#include <wtf/Variant.h>
#include <wtf/text/StringImpl.h>

namespace JSC {

ALWAYS_INLINE int32_t JSValue::toInt32(JSGlobalObject* globalObject) const
{
    if (isInt32())
        return asInt32();
    return JSC::toInt32(toNumber(globalObject));
}

inline uint32_t JSValue::toUInt32(JSGlobalObject* globalObject) const
{
    // See comment on JSC::toUInt32, in JSCJSValue.h.
    return toInt32(globalObject);
}

inline uint32_t JSValue::toIndex(JSGlobalObject* globalObject, const char* errorName) const
{
    VM& vm = getVM(globalObject);
    auto scope = DECLARE_THROW_SCOPE(vm);

    double d = toNumber(globalObject);
    RETURN_IF_EXCEPTION(scope, 0);
    if (d <= -1) {
        throwException(globalObject, scope, createRangeError(globalObject, makeString(errorName, " cannot be negative")));
        return 0;
    }
    if (d > std::numeric_limits<unsigned>::max()) {
        throwException(globalObject, scope, createRangeError(globalObject, makeString(errorName, " too large")));
        return 0;
    }

    if (isInt32())
        return asInt32();
    RELEASE_AND_RETURN(scope, JSC::toInt32(d));
}

inline bool JSValue::isUInt32() const
{
    return isInt32() && asInt32() >= 0;
}

inline uint32_t JSValue::asUInt32() const
{
    ASSERT(isUInt32());
    return asInt32();
}

inline double JSValue::asNumber() const
{
    ASSERT(isNumber());
    return isInt32() ? asInt32() : asDouble();
}

inline JSValue jsNaN()
{
    return JSValue(JSValue::EncodeAsDouble, PNaN);
}

inline JSValue::JSValue(char i)
{
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(unsigned char i)
{
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(short i)
{
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(unsigned short i)
{
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(unsigned i)
{
    if (static_cast<int32_t>(i) < 0) {
        *this = JSValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(long i)
{
    if (static_cast<int32_t>(i) != i) {
        *this = JSValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(unsigned long i)
{
    if (static_cast<uint32_t>(i) != i) {
        *this = JSValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = JSValue(static_cast<uint32_t>(i));
}

inline JSValue::JSValue(long long i)
{
    if (static_cast<int32_t>(i) != i) {
        *this = JSValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = JSValue(static_cast<int32_t>(i));
}

inline JSValue::JSValue(unsigned long long i)
{
    if (static_cast<uint32_t>(i) != i) {
        *this = JSValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = JSValue(static_cast<uint32_t>(i));
}

inline JSValue::JSValue(double d)
{
    if (canBeStrictInt32(d)) {
        *this = JSValue(static_cast<int32_t>(d));
        return;
    }
    *this = JSValue(EncodeAsDouble, d);
}

inline EncodedJSValue JSValue::encode(JSValue value)
{
    return value.u.asEncodedJSValue;
}

inline JSValue JSValue::decode(EncodedJSValue encodedJSValue)
{
    JSValue v;
    v.u.asEncodedJSValue = encodedJSValue;
    return v;
}

#if USE(JSVALUE32_64)
inline JSValue::JSValue()
{
    u.asBits.tag = EmptyValueTag;
    u.asBits.payload = 0;
}

inline JSValue::JSValue(JSNullTag)
{
    u.asBits.tag = NullTag;
    u.asBits.payload = 0;
}
    
inline JSValue::JSValue(JSUndefinedTag)
{
    u.asBits.tag = UndefinedTag;
    u.asBits.payload = 0;
}
    
inline JSValue::JSValue(JSTrueTag)
{
    u.asBits.tag = BooleanTag;
    u.asBits.payload = 1;
}
    
inline JSValue::JSValue(JSFalseTag)
{
    u.asBits.tag = BooleanTag;
    u.asBits.payload = 0;
}

inline JSValue::JSValue(HashTableDeletedValueTag)
{
    u.asBits.tag = DeletedValueTag;
    u.asBits.payload = 0;
}

inline JSValue::JSValue(JSCell* ptr)
{
    LOG_CHERI("Creating a new JSValue(JSCell) for %p\n", ptr);
    if (ptr) {
        LOG_CHERI("Setting tag to CellTag: %d\n", CellTag);
        u.asBits.tag = CellTag;
    }
    else {
        LOG_CHERI("Setting tag to EmptyValueTag: %d\n", EmptyValueTag);
        u.asBits.tag = EmptyValueTag;
    }
    LOG_CHERI("Setting payload to %d\n", reinterpret_cast<int32_t>(ptr));
    u.asBits.payload = reinterpret_cast<int32_t>(ptr);
}

inline JSValue::JSValue(const JSCell* ptr)
{
    LOG_CHERI("Creating a new JSValue(JSCell) for const %p\n", ptr);
    if (ptr) {
        LOG_CHERI("Setting tag to CellTag: %d\n", CellTag);
        u.asBits.tag = CellTag;
    }
    else {
        LOG_CHERI("Setting tag to EmptyValueTag: %d\n", EmptyValueTag);
        u.asBits.tag = EmptyValueTag;
    }
    LOG_CHERI("Setting payload to %d\n", reinterpret_cast<int32_t>(ptr));
    u.asBits.payload = reinterpret_cast<int32_t>(const_cast<JSCell*>(ptr));
}

inline JSValue::operator bool() const
{
    ASSERT(tag() != DeletedValueTag);
    return tag() != EmptyValueTag;
}

inline bool JSValue::operator==(const JSValue& other) const
{
    return u.asEncodedJSValue == other.u.asEncodedJSValue;
}

inline bool JSValue::operator!=(const JSValue& other) const
{
    return u.asEncodedJSValue != other.u.asEncodedJSValue;
}

inline bool JSValue::isEmpty() const
{
    return tag() == EmptyValueTag;
}

inline bool JSValue::isUndefined() const
{
    return tag() == UndefinedTag;
}

inline bool JSValue::isNull() const
{
    return tag() == NullTag;
}

inline bool JSValue::isUndefinedOrNull() const
{
    return isUndefined() || isNull();
}

inline bool JSValue::isCell() const
{
    return tag() == CellTag;
}

inline bool JSValue::isInt32() const
{
    return tag() == Int32Tag;
}

inline bool JSValue::isDouble() const
{
    return tag() < LowestTag;
}

inline bool JSValue::isTrue() const
{
    return tag() == BooleanTag && payload();
}

inline bool JSValue::isFalse() const
{
    return tag() == BooleanTag && !payload();
}

inline uint32_t JSValue::tag() const
{
    return u.asBits.tag;
}
    
inline int32_t JSValue::payload() const
{
    return u.asBits.payload;
}
    
inline int32_t JSValue::asInt32() const
{
    ASSERT(isInt32());
    return u.asBits.payload;
}
    
inline double JSValue::asDouble() const
{
    ASSERT(isDouble());
    return u.asDouble;
}
    
ALWAYS_INLINE JSCell* JSValue::asCell() const
{
    ASSERT(isCell());
    return reinterpret_cast<JSCell*>(u.asBits.payload);
}

ALWAYS_INLINE JSValue::JSValue(EncodeAsDoubleTag, double d)
{
    ASSERT(!isImpureNaN(d));
    u.asDouble = d;
}

inline JSValue::JSValue(int i)
{
    u.asBits.tag = Int32Tag;
    u.asBits.payload = i;
}

inline JSValue::JSValue(int32_t tag, int32_t payload)
{
    u.asBits.tag = tag;
    u.asBits.payload = payload;
}

inline bool JSValue::isNumber() const
{
    return isInt32() || isDouble();
}

inline bool JSValue::isBoolean() const
{
    return tag() == BooleanTag;
}

inline bool JSValue::asBoolean() const
{
    ASSERT(isBoolean());
    return payload();
}

#else // !USE(JSVALUE32_64) i.e. USE(JSVALUE64)

// 0x0 can never occur naturally because it has a tag of 00, indicating a pointer value, but a payload of 0x0, which is in the (invalid) zero page.
inline JSValue::JSValue()
{
    u.asEncodedJSValue = ValueEmpty;
}

// 0x4 can never occur naturally because it has a tag of 00, indicating a pointer value, but a payload of 0x4, which is in the (invalid) zero page.
inline JSValue::JSValue(HashTableDeletedValueTag)
{
    u.asEncodedJSValue = ValueDeleted;
}

inline JSValue::JSValue(JSCell* ptr)
{
    //LOG_CHERI("Creating JSValue from ptr: %p\n", ptr);
    u.ptr = ptr;
}

inline JSValue::JSValue(const JSCell* ptr)
{
    //LOG_CHERI("Creating JSValue from const ptr: %p\n", ptr);
    u.ptr = const_cast<JSCell*>(ptr);
}

inline JSValue::operator bool() const
{
    return u.asEncodedJSValue;
}

inline bool JSValue::operator==(const JSValue& other) const
{
    return u.asEncodedJSValue == other.u.asEncodedJSValue;
}

inline bool JSValue::operator!=(const JSValue& other) const
{
    return u.asEncodedJSValue != other.u.asEncodedJSValue;
}

inline bool JSValue::isEmpty() const
{
    return u.asEncodedJSValue == ValueEmpty;
}

inline bool JSValue::isUndefined() const
{
    return asValue() == JSValue(JSUndefined);
}

inline bool JSValue::isNull() const
{
    return asValue() == JSValue(JSNull);
}

inline bool JSValue::isTrue() const
{
    return asValue() == JSValue(JSTrue);
}

inline bool JSValue::isFalse() const
{
    return asValue() == JSValue(JSFalse);
}

inline bool JSValue::asBoolean() const
{
    ASSERT(isBoolean());
    return asValue() == JSValue(JSTrue);
}

inline int32_t JSValue::asInt32() const
{
    ASSERT(isInt32());
    return static_cast<int32_t>(u.asEncodedJSValue);
}

inline EncodedJSValue JSValue::asEncodedJSValue() const
{
    return u.asEncodedJSValue;
}

inline bool JSValue::isDouble() const
{
    return isNumber() && !isInt32();
}

inline JSValue::JSValue(JSNullTag)
{
    u.asEncodedJSValue = ValueNull;
}
    
inline JSValue::JSValue(JSUndefinedTag)
{
    u.asEncodedJSValue = ValueUndefined;
}

inline JSValue::JSValue(JSTrueTag)
{
    u.asEncodedJSValue = ValueTrue;
}

inline JSValue::JSValue(JSFalseTag)
{
    u.asEncodedJSValue = ValueFalse;
}

inline bool JSValue::isUndefinedOrNull() const
{
    // Undefined and null share the same value, bar the 'undefined' bit in the extended tag.
#if !defined(__CHERI_PURE_CAPABILITY__) || ENABLE(JSHEAP_CHERI_OFFSET_REFS)
    return (u.asEncodedJSValue & ~UndefinedTag) == ValueNull;
#else
    return WTF::Pointer::clearLowBits<UndefinedTag>(u.asEncodedJSValue) == ValueNull;
#endif
}

inline bool JSValue::isBoolean() const
{
#if !defined(__CHERI_PURE_CAPABILITY__) || ENABLE(JSHEAP_CHERI_OFFSET_REFS)
    return (u.asEncodedJSValue & ~1) == ValueFalse;
#else
    return WTF::Pointer::clearLowBits<1>(u.asEncodedJSValue) == ValueFalse;
#endif
}

inline bool JSValue::isCell() const
{
    return !((uint64_t) u.asEncodedJSValue & NotCellMask);
}

inline bool JSValue::isInt32() const
{
    return ((uint64_t) u.asEncodedJSValue & NumberTag) == NumberTag;
}

inline int64_t reinterpretDoubleToInt64(double value)
{
    return bitwise_cast<int64_t>(value);
}
inline double reinterpretInt64ToDouble(int64_t value)
{
    return bitwise_cast<double>(value);
}

ALWAYS_INLINE JSValue::JSValue(EncodeAsDoubleTag, double d)
{
    // XXXKG: for some reason this conversion doesn't happen earlier
    if (isImpureNaN(d))
        d = purifyNaN(d);
    ASSERT(!isImpureNaN(d));
    u.asEncodedJSValue = reinterpretDoubleToInt64(d) + JSValue::DoubleEncodeOffset;
    //LOG_CHERI("storing %f as %p, isImpureNaN? %d", d, u.asEncodedJSValue, isImpureNaN(d));
}

inline JSValue::JSValue(int i)
{
    //LOG_CHERI("Setting u.asEncodedJSValue to %d (%p)\n", TagTypeNumber | static_cast<uint32_t>(i), intptr_t(TagTypeNumber | static_cast<uint32_t>(i)));
    u.asEncodedJSValue = JSValue::NumberTag | static_cast<uint32_t>(i);
}

inline double JSValue::asDouble() const
{
    ASSERT(isDouble());
    return reinterpretInt64ToDouble((int64_t) u.asEncodedJSValue - JSValue::DoubleEncodeOffset);
}

inline bool JSValue::isNumber() const
{
    return (uint64_t) u.asEncodedJSValue & JSValue::NumberTag;
}

ALWAYS_INLINE JSCell* JSValue::asCell() const
{
    ASSERT(isCell());
    return u.ptr;
}

#endif // USE(JSVALUE64)

inline int64_t tryConvertToInt52(double number)
{
    if (number != number)
        return JSValue::notInt52;
#if OS(WINDOWS) && CPU(X86)
    // The VS Compiler for 32-bit builds generates a floating point error when attempting to cast
    // from an infinity to a 64-bit integer. We leave this routine with the floating point error
    // left in a register, causing undefined behavior in later floating point operations.
    //
    // To avoid this issue, we check for infinity here, and return false in that case.
    if (std::isinf(number))
        return JSValue::notInt52;
#endif
    int64_t asInt64 = static_cast<int64_t>(number);
    if (asInt64 != number)
        return JSValue::notInt52;
    if (!asInt64 && std::signbit(number))
        return JSValue::notInt52;
    if (asInt64 >= (static_cast<int64_t>(1) << (JSValue::numberOfInt52Bits - 1)))
        return JSValue::notInt52;
    if (asInt64 < -(static_cast<int64_t>(1) << (JSValue::numberOfInt52Bits - 1)))
        return JSValue::notInt52;
    return asInt64;
}

inline bool isInt52(double number)
{
    return tryConvertToInt52(number) != JSValue::notInt52;
}

inline bool JSValue::isAnyInt() const
{
    if (isInt32())
        return true;
    if (!isNumber())
        return false;
    return isInt52(asDouble());
}

inline int64_t JSValue::asAnyInt() const
{
    ASSERT(isAnyInt());
    if (isInt32())
        return asInt32();
    return static_cast<int64_t>(asDouble());
}

inline bool JSValue::isInt32AsAnyInt() const
{
    if (!isAnyInt())
        return false;
    int64_t value = asAnyInt();
    return value >= INT32_MIN && value <= INT32_MAX;
}

inline int32_t JSValue::asInt32AsAnyInt() const
{
    ASSERT(isInt32AsAnyInt());
    if (isInt32())
        return asInt32();
    return static_cast<int32_t>(asDouble());
}

inline bool JSValue::isUInt32AsAnyInt() const
{
    if (!isAnyInt())
        return false;
    int64_t value = asAnyInt();
    return value >= 0 && value <= UINT32_MAX;
}

inline uint32_t JSValue::asUInt32AsAnyInt() const
{
    ASSERT(isUInt32AsAnyInt());
    if (isUInt32())
        return asUInt32();
    return static_cast<uint32_t>(asDouble());
}

inline bool JSValue::isString() const
{
    return isCell() && asCell()->isString();
}

inline bool JSValue::isBigInt() const
{
    return isCell() && asCell()->isBigInt();
}

inline bool JSValue::isSymbol() const
{
    return isCell() && asCell()->isSymbol();
}

inline bool JSValue::isPrimitive() const
{
    return !isCell() || asCell()->isString() || asCell()->isSymbol() || asCell()->isBigInt();
}

inline bool JSValue::isGetterSetter() const
{
    return isCell() && asCell()->isGetterSetter();
}

inline bool JSValue::isCustomGetterSetter() const
{
    return isCell() && asCell()->isCustomGetterSetter();
}

inline bool JSValue::isObject() const
{
    return isCell() && asCell()->isObject();
}

inline bool JSValue::getString(JSGlobalObject* globalObject, String& s) const
{
    return isCell() && asCell()->getString(globalObject, s);
}

inline String JSValue::getString(JSGlobalObject* globalObject) const
{
    return isCell() ? asCell()->getString(globalObject) : String();
}

template <typename Base> String HandleConverter<Base, Unknown>::getString(JSGlobalObject* globalObject) const
{
    return jsValue().getString(globalObject);
}

inline JSObject* JSValue::getObject() const
{
    return isCell() ? asCell()->getObject() : 0;
}

ALWAYS_INLINE bool JSValue::getUInt32(uint32_t& v) const
{
    if (isInt32()) {
        int32_t i = asInt32();
        v = static_cast<uint32_t>(i);
        return i >= 0;
    }
    if (isDouble()) {
        double d = asDouble();
        v = static_cast<uint32_t>(d);
        return v == d;
    }
    return false;
}

ALWAYS_INLINE Identifier JSValue::toPropertyKey(JSGlobalObject* globalObject) const
{
    VM& vm = getVM(globalObject);
    auto scope = DECLARE_THROW_SCOPE(vm);

    if (isString())
        RELEASE_AND_RETURN(scope, asString(*this)->toIdentifier(globalObject));

    JSValue primitive = toPrimitive(globalObject, PreferString);
    RETURN_IF_EXCEPTION(scope, vm.propertyNames->emptyIdentifier);
    if (primitive.isSymbol())
        RELEASE_AND_RETURN(scope, Identifier::fromUid(asSymbol(primitive)->privateName()));

    auto string = primitive.toString(globalObject);
    RETURN_IF_EXCEPTION(scope, { });
    RELEASE_AND_RETURN(scope, string->toIdentifier(globalObject));
}

inline JSValue JSValue::toPrimitive(JSGlobalObject* globalObject, PreferredPrimitiveType preferredType) const
{
    return isCell() ? asCell()->toPrimitive(globalObject, preferredType) : asValue();
}

inline PreferredPrimitiveType toPreferredPrimitiveType(JSGlobalObject* globalObject, JSValue value)
{
    VM& vm = getVM(globalObject);
    auto scope = DECLARE_THROW_SCOPE(vm);

    if (!value.isString()) {
        throwTypeError(globalObject, scope, "Primitive hint is not a string."_s);
        return NoPreference;
    }

    StringImpl* hintString = asString(value)->value(globalObject).impl();
    RETURN_IF_EXCEPTION(scope, NoPreference);

    if (WTF::equal(hintString, "default"))
        return NoPreference;
    if (WTF::equal(hintString, "number"))
        return PreferNumber;
    if (WTF::equal(hintString, "string"))
        return PreferString;

    throwTypeError(globalObject, scope, "Expected primitive hint to match one of 'default', 'number', 'string'."_s);
    return NoPreference;
}

inline bool JSValue::getPrimitiveNumber(JSGlobalObject* globalObject, double& number, JSValue& value)
{
    if (isInt32()) {
        number = asInt32();
        value = *this;
        return true;
    }
    if (isDouble()) {
        number = asDouble();
        value = *this;
        return true;
    }
    if (isCell())
        return asCell()->getPrimitiveNumber(globalObject, number, value);
    if (isTrue()) {
        number = 1.0;
        value = *this;
        return true;
    }
    if (isFalse() || isNull()) {
        number = 0.0;
        value = *this;
        return true;
    }
    ASSERT(isUndefined());
    number = PNaN;
    value = *this;
    return true;
}

ALWAYS_INLINE double JSValue::toNumber(JSGlobalObject* globalObject) const
{
    if (isInt32())
        return asInt32();
    if (isDouble())
        return asDouble();
    return toNumberSlowCase(globalObject);
}

ALWAYS_INLINE Variant<JSBigInt*, double> JSValue::toNumeric(JSGlobalObject* globalObject) const
{
    if (isInt32())
        return asInt32();
    if (isDouble())
        return asDouble();
    if (isBigInt())
        return asBigInt(*this);

    VM& vm = getVM(globalObject);
    auto scope = DECLARE_THROW_SCOPE(vm);
    JSValue primValue = this->toPrimitive(globalObject, PreferNumber);
    RETURN_IF_EXCEPTION(scope, 0);
    if (primValue.isBigInt())
        return asBigInt(primValue);
    double value = primValue.toNumber(globalObject);
    RETURN_IF_EXCEPTION(scope, 0);
    return value;
}

ALWAYS_INLINE Variant<JSBigInt*, int32_t> JSValue::toBigIntOrInt32(JSGlobalObject* globalObject) const
{
    if (isInt32())
        return asInt32();
    if (isDouble() && canBeInt32(asDouble()))
        return static_cast<int32_t>(asDouble());
    if (isBigInt())
        return asBigInt(*this);

    VM& vm = getVM(globalObject);
    auto scope = DECLARE_THROW_SCOPE(vm);
    JSValue primValue = this->toPrimitive(globalObject, PreferNumber);
    RETURN_IF_EXCEPTION(scope, 0);
    if (primValue.isBigInt())
        return asBigInt(primValue);
    int32_t value = primValue.toInt32(globalObject);
    RETURN_IF_EXCEPTION(scope, 0);
    return value;
}

inline JSObject* JSValue::toObject(JSGlobalObject* globalObject) const
{
    return isCell() ? asCell()->toObject(globalObject) : toObjectSlowCase(globalObject);
}

inline bool JSValue::isFunction(VM& vm) const
{
    if (!isCell())
        return false;
    return asCell()->isFunction(vm);
}

inline bool JSValue::isCallable(VM& vm, CallType& callType, CallData& callData) const
{
    if (!isCell())
        return false;
    return asCell()->isCallable(vm, callType, callData);
}

inline bool JSValue::isConstructor(VM& vm) const
{
    if (!isCell())
        return false;
    return asCell()->isConstructor(vm);
}

inline bool JSValue::isConstructor(VM& vm, ConstructType& constructType, ConstructData& constructData) const
{
    if (!isCell())
        return false;
    return asCell()->isConstructor(vm, constructType, constructData);
}

// this method is here to be after the inline declaration of JSCell::inherits
inline bool JSValue::inherits(VM& vm, const ClassInfo* classInfo) const
{
    return isCell() && asCell()->inherits(vm, classInfo);
}

template<typename Target>
inline bool JSValue::inherits(VM& vm) const
{
    return isCell() && asCell()->inherits<Target>(vm);
}

inline const ClassInfo* JSValue::classInfoOrNull(VM& vm) const
{
    return isCell() ? asCell()->classInfo(vm) : nullptr;
}

inline JSValue JSValue::toThis(JSGlobalObject* globalObject, ECMAMode ecmaMode) const
{
    return isCell() ? asCell()->methodTable(getVM(globalObject))->toThis(asCell(), globalObject, ecmaMode) : toThisSlowCase(globalObject, ecmaMode);
}

ALWAYS_INLINE JSValue JSValue::get(JSGlobalObject* globalObject, PropertyName propertyName) const
{
    PropertySlot slot(asValue(), PropertySlot::InternalMethodType::Get);
    return get(globalObject, propertyName, slot);
}

ALWAYS_INLINE JSValue JSValue::get(JSGlobalObject* globalObject, PropertyName propertyName, PropertySlot& slot) const
{
    auto scope = DECLARE_THROW_SCOPE(getVM(globalObject));
    bool hasSlot = getPropertySlot(globalObject, propertyName, slot);
    EXCEPTION_ASSERT(!scope.exception() || !hasSlot);
    if (!hasSlot)
        return jsUndefined();
    RELEASE_AND_RETURN(scope, slot.getValue(globalObject, propertyName));
}

template<typename CallbackWhenNoException>
ALWAYS_INLINE typename std::result_of<CallbackWhenNoException(bool, PropertySlot&)>::type JSValue::getPropertySlot(JSGlobalObject* globalObject, PropertyName propertyName, CallbackWhenNoException callback) const
{
    PropertySlot slot(asValue(), PropertySlot::InternalMethodType::Get);
    return getPropertySlot(globalObject, propertyName, slot, callback);
}

template<typename CallbackWhenNoException>
ALWAYS_INLINE typename std::result_of<CallbackWhenNoException(bool, PropertySlot&)>::type JSValue::getPropertySlot(JSGlobalObject* globalObject, PropertyName propertyName, PropertySlot& slot, CallbackWhenNoException callback) const
{
    auto scope = DECLARE_THROW_SCOPE(getVM(globalObject));
    bool found = getPropertySlot(globalObject, propertyName, slot);
    RETURN_IF_EXCEPTION(scope, { });
    RELEASE_AND_RETURN(scope, callback(found, slot));
}

ALWAYS_INLINE bool JSValue::getPropertySlot(JSGlobalObject* globalObject, PropertyName propertyName, PropertySlot& slot) const
{
    auto scope = DECLARE_THROW_SCOPE(getVM(globalObject));
    // If this is a primitive, we'll need to synthesize the prototype -
    // and if it's a string there are special properties to check first.
    JSObject* object;
    if (UNLIKELY(!isObject())) {
        if (isString()) {
            bool hasProperty = asString(*this)->getStringPropertySlot(globalObject, propertyName, slot);
            RETURN_IF_EXCEPTION(scope, false);
            if (hasProperty)
                return true;
        }
        object = synthesizePrototype(globalObject);
        EXCEPTION_ASSERT(!!scope.exception() == !object);
        if (UNLIKELY(!object))
            return false;
    } else
        object = asObject(asCell());

    RELEASE_AND_RETURN(scope, object->getPropertySlot(globalObject, propertyName, slot));
}

ALWAYS_INLINE bool JSValue::getOwnPropertySlot(JSGlobalObject* globalObject, PropertyName propertyName, PropertySlot& slot) const
{
    // If this is a primitive, we'll need to synthesize the prototype -
    // and if it's a string there are special properties to check first.
    auto scope = DECLARE_THROW_SCOPE(getVM(globalObject));
    if (UNLIKELY(!isObject())) {
        if (isString())
            RELEASE_AND_RETURN(scope, asString(*this)->getStringPropertySlot(globalObject, propertyName, slot));

        if (isUndefinedOrNull())
            throwException(globalObject, scope, createNotAnObjectError(globalObject, *this));
        return false;
    }
    RELEASE_AND_RETURN(scope, asObject(asCell())->getOwnPropertySlotInline(globalObject, propertyName, slot));
}

ALWAYS_INLINE JSValue JSValue::get(JSGlobalObject* globalObject, unsigned propertyName) const
{
    PropertySlot slot(asValue(), PropertySlot::InternalMethodType::Get);
    return get(globalObject, propertyName, slot);
}

ALWAYS_INLINE JSValue JSValue::get(JSGlobalObject* globalObject, unsigned propertyName, PropertySlot& slot) const
{
    auto scope = DECLARE_THROW_SCOPE(getVM(globalObject));
    // If this is a primitive, we'll need to synthesize the prototype -
    // and if it's a string there are special properties to check first.
    JSObject* object;
    if (UNLIKELY(!isObject())) {
        if (isString()) {
            bool hasProperty = asString(*this)->getStringPropertySlot(globalObject, propertyName, slot);
            RETURN_IF_EXCEPTION(scope, { });
            if (hasProperty)
                RELEASE_AND_RETURN(scope, slot.getValue(globalObject, propertyName));
        }
        object = synthesizePrototype(globalObject);
        EXCEPTION_ASSERT(!!scope.exception() == !object);
        if (UNLIKELY(!object))
            return JSValue();
    } else
        object = asObject(asCell());
    
    bool hasSlot = object->getPropertySlot(globalObject, propertyName, slot);
    EXCEPTION_ASSERT(!scope.exception() || !hasSlot);
    if (!hasSlot)
        return jsUndefined();
    RELEASE_AND_RETURN(scope, slot.getValue(globalObject, propertyName));
}

ALWAYS_INLINE JSValue JSValue::get(JSGlobalObject* globalObject, uint64_t propertyName) const
{
    if (LIKELY(propertyName <= std::numeric_limits<unsigned>::max()))
        return get(globalObject, static_cast<unsigned>(propertyName));
    return get(globalObject, Identifier::from(getVM(globalObject), static_cast<double>(propertyName)));
}

inline bool JSValue::put(JSGlobalObject* globalObject, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    if (UNLIKELY(!isCell()))
        return putToPrimitive(globalObject, propertyName, value, slot);

    return asCell()->methodTable(getVM(globalObject))->put(asCell(), globalObject, propertyName, value, slot);
}

ALWAYS_INLINE bool JSValue::putInline(JSGlobalObject* globalObject, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    if (UNLIKELY(!isCell()))
        return putToPrimitive(globalObject, propertyName, value, slot);
    return asCell()->putInline(globalObject, propertyName, value, slot);
}

inline bool JSValue::putByIndex(JSGlobalObject* globalObject, unsigned propertyName, JSValue value, bool shouldThrow)
{
    if (UNLIKELY(!isCell()))
        return putToPrimitiveByIndex(globalObject, propertyName, value, shouldThrow);

    return asCell()->methodTable(getVM(globalObject))->putByIndex(asCell(), globalObject, propertyName, value, shouldThrow);
}

inline Structure* JSValue::structureOrNull() const
{
    if (isCell())
        return asCell()->structure();
    return nullptr;
}

inline JSValue JSValue::structureOrUndefined() const
{
    if (isCell())
        return JSValue(asCell()->structure());
    return jsUndefined();
}

// ECMA 11.9.3
inline bool JSValue::equal(JSGlobalObject* globalObject, JSValue v1, JSValue v2)
{
    if (v1.isInt32() && v2.isInt32())
        return v1 == v2;

    return equalSlowCase(globalObject, v1, v2);
}

ALWAYS_INLINE bool JSValue::equalSlowCaseInline(JSGlobalObject* globalObject, JSValue v1, JSValue v2)
{
    VM& vm = getVM(globalObject);
    auto scope = DECLARE_THROW_SCOPE(vm);
    do {
        if (v1.isNumber() && v2.isNumber())
            return v1.asNumber() == v2.asNumber();

        bool s1 = v1.isString();
        bool s2 = v2.isString();
        if (s1 && s2)
            RELEASE_AND_RETURN(scope, asString(v1)->equal(globalObject, asString(v2)));

        if (v1.isBigInt() && s2) {
            String v2String = asString(v2)->value(globalObject);
            RETURN_IF_EXCEPTION(scope, false);
            JSBigInt* n = JSBigInt::stringToBigInt(globalObject, v2String);
            RETURN_IF_EXCEPTION(scope, false);
            if (!n)
                return false;
            
            v2 = JSValue(n);
            continue;
        }

        if (s1 && v2.isBigInt()) {
            String v1String = asString(v1)->value(globalObject);
            RETURN_IF_EXCEPTION(scope, false);
            JSBigInt* n = JSBigInt::stringToBigInt(globalObject, v1String);
            RETURN_IF_EXCEPTION(scope, false);
            if (!n)
                return false;
            
            v1 = JSValue(n);
            continue;
        }

        if (v1.isUndefinedOrNull()) {
            if (v2.isUndefinedOrNull())
                return true;
            if (!v2.isCell())
                return false;
            return v2.asCell()->structure(vm)->masqueradesAsUndefined(globalObject);
        }

        if (v2.isUndefinedOrNull()) {
            if (!v1.isCell())
                return false;
            return v1.asCell()->structure(vm)->masqueradesAsUndefined(globalObject);
        }

        if (v1.isObject()) {
            if (v2.isObject())
                return v1 == v2;
            JSValue p1 = v1.toPrimitive(globalObject);
            RETURN_IF_EXCEPTION(scope, false);
            v1 = p1;
            if (v1.isInt32() && v2.isInt32())
                return v1 == v2;
            continue;
        }

        if (v2.isObject()) {
            JSValue p2 = v2.toPrimitive(globalObject);
            RETURN_IF_EXCEPTION(scope, false);
            v2 = p2;
            if (v1.isInt32() && v2.isInt32())
                return v1 == v2;
            continue;
        }

        bool sym1 = v1.isSymbol();
        bool sym2 = v2.isSymbol();
        if (sym1 || sym2) {
            if (sym1 && sym2)
                return asSymbol(v1) == asSymbol(v2);
            return false;
        }

        if (s1 || s2) {
            double d1 = v1.toNumber(globalObject);
            RETURN_IF_EXCEPTION(scope, false);
            double d2 = v2.toNumber(globalObject);
            RETURN_IF_EXCEPTION(scope, false);
            return d1 == d2;
        }

        if (v1.isBoolean()) {
            if (v2.isNumber())
                return static_cast<double>(v1.asBoolean()) == v2.asNumber();
            else if (v2.isBigInt()) {
                v1 = JSValue(v1.toNumber(globalObject));
                continue;
            }
        } else if (v2.isBoolean()) {
            if (v1.isNumber())
                return v1.asNumber() == static_cast<double>(v2.asBoolean());
            else if (v1.isBigInt()) {
                v2 = JSValue(v2.toNumber(globalObject));
                continue;
            }
        }
        
        if (v1.isBigInt() && v2.isBigInt())
            return JSBigInt::equals(asBigInt(v1), asBigInt(v2));
        
        if (v1.isBigInt() && v2.isNumber())
            return asBigInt(v1)->equalsToNumber(v2);

        if (v2.isBigInt() && v1.isNumber())
            return asBigInt(v2)->equalsToNumber(v1);

        return v1 == v2;
    } while (true);
}

// ECMA 11.9.3
ALWAYS_INLINE bool JSValue::strictEqualSlowCaseInline(JSGlobalObject* globalObject, JSValue v1, JSValue v2)
{
    ASSERT(v1.isCell() && v2.isCell());

    if (v1.asCell()->isString() && v2.asCell()->isString())
        return asString(v1)->equal(globalObject, asString(v2));
    if (v1.isBigInt() && v2.isBigInt())
        return JSBigInt::equals(asBigInt(v1), asBigInt(v2));
    return v1 == v2;
}

inline bool JSValue::strictEqual(JSGlobalObject* globalObject, JSValue v1, JSValue v2)
{
    if (v1.isInt32() && v2.isInt32())
        return v1 == v2;

    if (v1.isNumber() && v2.isNumber())
        return v1.asNumber() == v2.asNumber();

    if (!v1.isCell() || !v2.isCell())
        return v1 == v2;

    return strictEqualSlowCaseInline(globalObject, v1, v2);
}

inline int32_t JSValue::asInt32ForArithmetic() const
{
    if (isBoolean())
        return asBoolean();
    return asInt32();
}

inline TriState JSValue::pureStrictEqual(JSValue v1, JSValue v2)
{
    if (v1.isInt32() && v2.isInt32())
        return triState(v1 == v2);

    if (v1.isNumber() && v2.isNumber())
        return triState(v1.asNumber() == v2.asNumber());

    if (!v1.isCell() || !v2.isCell())
        return triState(v1 == v2);
    
    if (v1.asCell()->isString() && v2.asCell()->isString()) {
        const StringImpl* v1String = asString(v1)->tryGetValueImpl();
        const StringImpl* v2String = asString(v2)->tryGetValueImpl();
        if (!v1String || !v2String)
            return MixedTriState;
        return triState(WTF::equal(*v1String, *v2String));
    }
    
    return triState(v1 == v2);
}

inline TriState JSValue::pureToBoolean() const
{
    if (isInt32())
        return asInt32() ? TrueTriState : FalseTriState;
    if (isDouble())
        return isNotZeroAndOrdered(asDouble()) ? TrueTriState : FalseTriState; // false for NaN
    if (isCell())
        return asCell()->pureToBoolean();
    return isTrue() ? TrueTriState : FalseTriState;
}

ALWAYS_INLINE bool JSValue::requireObjectCoercible(JSGlobalObject* globalObject) const
{
    VM& vm = getVM(globalObject);
    auto scope = DECLARE_THROW_SCOPE(vm);

    if (!isUndefinedOrNull())
        return true;
    throwException(globalObject, scope, createNotAnObjectError(globalObject, *this));
    return false;
}

ALWAYS_INLINE bool isThisValueAltered(const PutPropertySlot& slot, JSObject* baseObject)
{
    JSValue thisValue = slot.thisValue();
    if (LIKELY(thisValue == baseObject))
        return false;

    if (!thisValue.isObject())
        return true;
    JSObject* thisObject = asObject(thisValue);
    // Only PureForwardingProxyType can be seen as the same to the original target object.
    if (thisObject->type() == PureForwardingProxyType && jsCast<JSProxy*>(thisObject)->target() == baseObject)
        return false;
    return true;
}

// See section 7.2.9: https://tc39.github.io/ecma262/#sec-samevalue
ALWAYS_INLINE bool sameValue(JSGlobalObject* globalObject, JSValue a, JSValue b)
{
    if (!a.isNumber())
        return JSValue::strictEqual(globalObject, a, b);
    if (!b.isNumber())
        return false;
    double x = a.asNumber();
    double y = b.asNumber();
    bool xIsNaN = std::isnan(x);
    bool yIsNaN = std::isnan(y);
    if (xIsNaN || yIsNaN)
        return xIsNaN && yIsNaN;
    return bitwise_cast<uint64_t>(x) == bitwise_cast<uint64_t>(y);
}

} // namespace JSC
