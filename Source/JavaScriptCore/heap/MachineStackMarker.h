/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003-2017 Apple Inc. All rights reserved.
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

#include "RegisterState.h"
#include <wtf/Lock.h>
#include <wtf/ScopedLambda.h>
#include <wtf/ThreadGroup.h>

namespace JSC {

class CodeBlockSet;
class ConservativeRoots;
class Heap;
class JITStubRoutineSet;

struct CurrentThreadState {
    void* stackOrigin { nullptr };
    void* stackTop { nullptr };
    RegisterState* registerState { nullptr };
};
    
class MachineThreads {
    WTF_MAKE_FAST_ALLOCATED;
    WTF_MAKE_NONCOPYABLE(MachineThreads);
public:
    MachineThreads();

    void gatherConservativeRoots(ConservativeRoots&, JITStubRoutineSet&, CodeBlockSet&, CurrentThreadState*, Thread*);

    // Only needs to be called by clients that can use the same heap from multiple threads.
    bool addCurrentThread() { return m_threadGroup->addCurrentThread() == ThreadGroupAddResult::NewlyAdded; }

    WordLock& getLock() { return m_threadGroup->getLock(); }
    const ListHashSet<Ref<Thread>>& threads(const AbstractLocker& locker) const { return m_threadGroup->threads(locker); }

private:
    void gatherFromCurrentThread(ConservativeRoots&, JITStubRoutineSet&, CodeBlockSet&, CurrentThreadState&);

    void tryCopyOtherThreadStack(Thread&, void*, size_t capacity, size_t*);
    bool tryCopyOtherThreadStacks(const AbstractLocker&, void*, size_t capacity, size_t*, Thread&);

    std::shared_ptr<ThreadGroup> m_threadGroup;
};

#if defined(__CHERI_PURE_CAPABILITY__)
#define GET_STACK_BOUNDED_POINTER_TO_VARIABLE(var) \
    __builtin_cheri_offset_increment( \
        currentStackPointer(), \
        (vaddr_t) &(var) - __builtin_cheri_address_get(currentStackPointer()))
#else
#define GET_STACK_BOUNDED_POINTER_TO_VARIABLE(var) \
    &(var)
#endif

#define DECLARE_AND_COMPUTE_CURRENT_THREAD_STATE(stateName) \
    CurrentThreadState stateName; \
    stateName.stackTop = GET_STACK_BOUNDED_POINTER_TO_VARIABLE(stateName); \
    stateName.stackOrigin = Thread::current().stack().origin(); \
    ALLOCATE_AND_GET_REGISTER_STATE(stateName ## _registerState); \
    stateName.registerState = &stateName ## _registerState

// The return value is meaningless. We just use it to suppress tail call optimization.
int callWithCurrentThreadState(const ScopedLambda<void(CurrentThreadState&)>&);

} // namespace JSC

