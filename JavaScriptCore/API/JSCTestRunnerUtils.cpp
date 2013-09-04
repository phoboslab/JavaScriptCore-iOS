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

#include "config.h"
#include "JSCTestRunnerUtils.h"

#include "APICast.h"
#include "CodeBlock.h"
#include "Operations.h"

namespace JSC {

static FunctionExecutable* getExecutable(JSContextRef context, JSValueRef theFunctionValueRef)
{
    ExecState* exec = toJS(context);
    JSValue theFunctionValue = toJS(exec, theFunctionValueRef);
    
    JSFunction* theFunction = jsDynamicCast<JSFunction*>(theFunctionValue);
    if (!theFunction)
        return 0;
    
    FunctionExecutable* executable = jsDynamicCast<FunctionExecutable*>(
        theFunction->executable());
    return executable;
}

JSValueRef numberOfDFGCompiles(JSContextRef context, JSValueRef theFunctionValueRef)
{
    bool pretendToHaveManyCompiles = false;
#if ENABLE(DFG_JIT)
    if (!Options::useJIT() || !Options::useDFGJIT())
        pretendToHaveManyCompiles = true;
#else
    pretendToHaveManyCompiles = true;
#endif
    
    if (FunctionExecutable* executable = getExecutable(context, theFunctionValueRef)) {
        CodeBlock* baselineCodeBlock = executable->baselineCodeBlockFor(CodeForCall);
        
        if (!baselineCodeBlock)
            return JSValueMakeNumber(context, 0);

        if (pretendToHaveManyCompiles)
            return JSValueMakeNumber(context, 1000000.0);
        return JSValueMakeNumber(context, baselineCodeBlock->numberOfDFGCompiles());
    }
    
    return JSValueMakeUndefined(context);
}

JSValueRef setNeverInline(JSContextRef context, JSValueRef theFunctionValueRef)
{
    if (FunctionExecutable* executable = getExecutable(context, theFunctionValueRef))
        executable->setNeverInline(true);
    
    return JSValueMakeUndefined(context);
}

} // namespace JSC

