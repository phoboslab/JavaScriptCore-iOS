/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "DFGCodeBlocks.h"

#include "CodeBlock.h"
#include "SlotVisitor.h"
#include <wtf/Vector.h>

namespace JSC {

#if ENABLE(DFG_JIT)

DFGCodeBlocks::DFGCodeBlocks() { }

DFGCodeBlocks::~DFGCodeBlocks()
{
    Vector<RefPtr<CodeBlock>, 16> toRemove;
    
    for (HashSet<CodeBlock*>::iterator iter = m_set.begin(); iter != m_set.end(); ++iter) {
        if ((*iter)->jitCode()->dfgCommon()->isJettisoned)
            toRemove.append(adoptRef(*iter));
    }
}

void DFGCodeBlocks::jettison(PassRefPtr<CodeBlock> codeBlockPtr)
{
    // We don't want to delete it now; we just want its pointer.
    CodeBlock* codeBlock = codeBlockPtr.leakRef();
    
    ASSERT(codeBlock);
    ASSERT(JITCode::isOptimizingJIT(codeBlock->jitType()));
    
    // It should not have already been jettisoned.
    ASSERT(!codeBlock->jitCode()->dfgCommon()->isJettisoned);

    // We should have this block already.
    ASSERT(m_set.find(codeBlock) != m_set.end());
    
    codeBlock->jitCode()->dfgCommon()->isJettisoned = true;
}

void DFGCodeBlocks::clearMarks()
{
    for (HashSet<CodeBlock*>::iterator iter = m_set.begin(); iter != m_set.end(); ++iter) {
        (*iter)->jitCode()->dfgCommon()->mayBeExecuting = false;
        (*iter)->jitCode()->dfgCommon()->visitAggregateHasBeenCalled = false;
    }
}

void DFGCodeBlocks::deleteUnmarkedJettisonedCodeBlocks()
{
    Vector<RefPtr<CodeBlock>, 16> toRemove;
    
    for (HashSet<CodeBlock*>::iterator iter = m_set.begin(); iter != m_set.end(); ++iter) {
        if ((*iter)->jitCode()->dfgCommon()->isJettisoned && !(*iter)->jitCode()->dfgCommon()->mayBeExecuting)
            toRemove.append(adoptRef(*iter));
    }
}

void DFGCodeBlocks::traceMarkedCodeBlocks(SlotVisitor& visitor)
{
    for (HashSet<CodeBlock*>::iterator iter = m_set.begin(); iter != m_set.end(); ++iter) {
        if ((*iter)->jitCode()->dfgCommon()->mayBeExecuting)
            (*iter)->visitAggregate(visitor);
    }
}

#else // ENABLE(DFG_JIT)

void DFGCodeBlocks::jettison(PassRefPtr<CodeBlock>)
{
}

#endif // ENABLE(DFG_JIT)

} // namespace JSC


