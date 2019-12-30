#include "HeapPtr.h"
/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2007 Maks Orlovich
 *  Copyright (C) 2019 Arm Ltd. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#pragma once

#include "JSCJSValue.h"
#include <unicode/uchar.h>

namespace JSC {

class ArgList;
class CallFrame;
class JSObject;

// FIXME: These functions should really be in JSGlobalObject.cpp, but putting them there
// is a 0.5% reduction.

extern const ASCIILiteral ObjectProtoCalledOnNullOrUndefinedError;

EncodedJSValue JSC_HOST_CALL globalFuncEval(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncParseInt(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncParseFloat(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncDecodeURI(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncDecodeURIComponent(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncEncodeURI(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncEncodeURIComponent(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncEscape(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncUnescape(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncThrowTypeError(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncThrowTypeErrorArgumentsCalleeAndCaller(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncMakeTypeError(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncProtoGetter(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncProtoSetter(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncHostPromiseRejectionTracker(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncBuiltinLog(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncBuiltinDescribe(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncImportModule(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncPropertyIsEnumerable(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncOwnKeys(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL globalFuncDateTimeFormat(HeapPtr<JSGlobalObject>, CallFrame*);

double jsToNumber(StringView);

} // namespace JSC
