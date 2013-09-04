/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef DFGPlan_h
#define DFGPlan_h

#include <wtf/Platform.h>

#include "CompilationResult.h"
#include "DFGDesiredIdentifiers.h"
#include "DFGDesiredStructureChains.h"
#include "DFGDesiredTransitions.h"
#include "DFGDesiredWatchpoints.h"
#include "DFGDesiredWeakReferences.h"
#include "DFGDesiredWriteBarriers.h"
#include "DFGFinalizer.h"
#include "DeferredCompilationCallback.h"
#include "Operands.h"
#include "ProfilerCompilation.h"
#include <wtf/ThreadSafeRefCounted.h>

namespace JSC {

class CodeBlock;

namespace DFG {

class LongLivedState;

#if ENABLE(DFG_JIT)

struct Plan : public ThreadSafeRefCounted<Plan> {
    Plan(
        PassRefPtr<CodeBlock>, unsigned osrEntryBytecodeIndex, unsigned numVarsWithValues);
    ~Plan();
    
    void compileInThread(LongLivedState&);
    
    CompilationResult finalizeWithoutNotifyingCallback();
    void finalizeAndNotifyCallback();
    
    void notifyReady();
    
    CodeBlock* key();
    
    VM& vm;
    RefPtr<CodeBlock> codeBlock;
    const unsigned osrEntryBytecodeIndex;
    const unsigned numVarsWithValues;
    Operands<JSValue> mustHandleValues;

    RefPtr<Profiler::Compilation> compilation;

    OwnPtr<Finalizer> finalizer;
    
    DesiredWatchpoints watchpoints;
    DesiredIdentifiers identifiers;
    DesiredStructureChains chains;
    DesiredWeakReferences weakReferences;
    DesiredWriteBarriers writeBarriers;
    DesiredTransitions transitions;

    double beforeFTL;
    
    bool isCompiled;

    RefPtr<DeferredCompilationCallback> callback;

private:
    enum CompilationPath { FailPath, DFGPath, FTLPath };
    CompilationPath compileInThreadImpl(LongLivedState&);
    
    bool isStillValid();
    void reallyAdd(CommonData*);
};

#else // ENABLE(DFG_JIT)

class Plan : public RefCounted<Plan> {
    // Dummy class to allow !ENABLE(DFG_JIT) to build.
};

#endif // ENABLE(DFG_JIT)

} } // namespace JSC::DFG

#endif // DFGPlan_h

