#include "HeapPtr.h"
/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003-2019 Apple Inc. All Rights Reserved.
 *  Copyright (C) 2019 Arm Ltd. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#include "JSObject.h"
#include "RegExp.h"

namespace JSC {

class RegExpPrototype final : public JSNonFinalObject {
public:
    typedef JSNonFinalObject Base;
    static constexpr unsigned StructureFlags = Base::StructureFlags;

    static RegExpPrototype* create(VM& vm, JSGlobalObject* globalObject, Structure* structure)
    {
        RegExpPrototype* prototype = new (NotNull, allocateCell<RegExpPrototype>(vm.heap)) RegExpPrototype(vm, structure);
        prototype->finishCreation(vm, globalObject);
        return prototype;
    }

    DECLARE_INFO;

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
    }

protected:
    RegExpPrototype(VM&, Structure*);

private:
    void finishCreation(VM&, JSGlobalObject*);
};

EncodedJSValue JSC_HOST_CALL regExpProtoFuncMatchFast(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL regExpProtoFuncSearchFast(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL regExpProtoFuncSplitFast(HeapPtr<JSGlobalObject>, CallFrame*);
EncodedJSValue JSC_HOST_CALL regExpProtoFuncTestFast(HeapPtr<JSGlobalObject>, CallFrame*);

} // namespace JSC
