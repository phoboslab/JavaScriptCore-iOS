/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef DFGCapabilities_h
#define DFGCapabilities_h

#include "CodeBlock.h"
#include "DFGCommon.h"
#include "DFGNode.h"
#include "Executable.h"
#include "Interpreter.h"
#include "Intrinsic.h"
#include "Options.h"
#include <wtf/Platform.h>

namespace JSC { namespace DFG {

#if ENABLE(DFG_JIT)
// Fast check functions; if they return true it is still necessary to
// check opcodes.
bool mightCompileEval(CodeBlock*);
bool mightCompileProgram(CodeBlock*);
bool mightCompileFunctionForCall(CodeBlock*);
bool mightCompileFunctionForConstruct(CodeBlock*);
bool mightInlineFunctionForCall(CodeBlock*);
bool mightInlineFunctionForClosureCall(CodeBlock*);
bool mightInlineFunctionForConstruct(CodeBlock*);

inline CapabilityLevel capabilityLevel(OpcodeID opcodeID, CodeBlock* codeBlock, Instruction* pc);

CapabilityLevel capabilityLevel(CodeBlock*);
#else // ENABLE(DFG_JIT)
inline bool mightCompileEval(CodeBlock*) { return false; }
inline bool mightCompileProgram(CodeBlock*) { return false; }
inline bool mightCompileFunctionForCall(CodeBlock*) { return false; }
inline bool mightCompileFunctionForConstruct(CodeBlock*) { return false; }
inline bool mightInlineFunctionForCall(CodeBlock*) { return false; }
inline bool mightInlineFunctionForClosureCall(CodeBlock*) { return false; }
inline bool mightInlineFunctionForConstruct(CodeBlock*) { return false; }

inline CapabilityLevel capabilityLevel(OpcodeID, CodeBlock*, Instruction*) { return CannotCompile; }
inline CapabilityLevel capabilityLevel(CodeBlock*) { return CannotCompile; }
#endif // ENABLE(DFG_JIT)

inline CapabilityLevel evalCapabilityLevel(CodeBlock* codeBlock)
{
    if (!mightCompileEval(codeBlock))
        return CannotCompile;
    
    return capabilityLevel(codeBlock);
}

inline CapabilityLevel programCapabilityLevel(CodeBlock* codeBlock)
{
    if (!mightCompileProgram(codeBlock))
        return CannotCompile;
    
    return capabilityLevel(codeBlock);
}

inline CapabilityLevel functionForCallCapabilityLevel(CodeBlock* codeBlock)
{
    if (!mightCompileFunctionForCall(codeBlock))
        return CannotCompile;
    
    return capabilityLevel(codeBlock);
}

inline CapabilityLevel functionForConstructCapabilityLevel(CodeBlock* codeBlock)
{
    if (!mightCompileFunctionForConstruct(codeBlock))
        return CannotCompile;
    
    return capabilityLevel(codeBlock);
}

inline bool canInlineFunctionForCall(CodeBlock* codeBlock)
{
    return mightInlineFunctionForCall(codeBlock) && canInline(capabilityLevel(codeBlock));
}

inline bool canInlineFunctionForClosureCall(CodeBlock* codeBlock)
{
    return mightInlineFunctionForClosureCall(codeBlock) && canInline(capabilityLevel(codeBlock));
}

inline bool canInlineFunctionForConstruct(CodeBlock* codeBlock)
{
    return mightInlineFunctionForConstruct(codeBlock) && canInline(capabilityLevel(codeBlock));
}

inline bool mightInlineFunctionFor(CodeBlock* codeBlock, CodeSpecializationKind kind)
{
    if (kind == CodeForCall)
        return mightInlineFunctionForCall(codeBlock);
    ASSERT(kind == CodeForConstruct);
    return mightInlineFunctionForConstruct(codeBlock);
}

inline bool mightInlineFunction(CodeBlock* codeBlock)
{
    return mightInlineFunctionFor(codeBlock, codeBlock->specializationKind());
}

inline bool canInlineFunctionFor(CodeBlock* codeBlock, CodeSpecializationKind kind, bool isClosureCall)
{
    if (isClosureCall) {
        ASSERT(kind == CodeForCall);
        return canInlineFunctionForClosureCall(codeBlock);
    }
    if (kind == CodeForCall)
        return canInlineFunctionForCall(codeBlock);
    ASSERT(kind == CodeForConstruct);
    return canInlineFunctionForConstruct(codeBlock);
}

} } // namespace JSC::DFG

#endif // DFGCapabilities_h

