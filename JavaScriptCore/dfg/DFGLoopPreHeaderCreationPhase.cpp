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
#include "DFGLoopPreHeaderCreationPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGBasicBlockInlines.h"
#include "DFGBlockInsertionSet.h"
#include "DFGGraph.h"
#include "DFGPhase.h"
#include "Operations.h"
#include <wtf/HashMap.h>

namespace JSC { namespace DFG {

class LoopPreHeaderCreationPhase : public Phase {
public:
    LoopPreHeaderCreationPhase(Graph& graph)
        : Phase(graph, "loop pre-header creation")
        , m_insertionSet(graph)
    {
    }
    
    bool run()
    {
        m_graph.m_dominators.computeIfNecessary(m_graph);
        m_graph.m_naturalLoops.computeIfNecessary(m_graph);
        
        for (unsigned loopIndex = m_graph.m_naturalLoops.numLoops(); loopIndex--;) {
            const NaturalLoop& loop = m_graph.m_naturalLoops.loop(loopIndex);
            BasicBlock* existingPreHeader = 0;
            bool needsNewPreHeader = false;
            for (unsigned predecessorIndex = loop.header()->predecessors.size(); predecessorIndex--;) {
                BasicBlock* predecessor = loop.header()->predecessors[predecessorIndex];
                if (m_graph.m_dominators.dominates(loop.header(), predecessor))
                    continue;
                if (!existingPreHeader) {
                    existingPreHeader = predecessor;
                    continue;
                }
                if (existingPreHeader == predecessor)
                    continue;
                needsNewPreHeader = true;
                break;
            }
            if (!needsNewPreHeader)
                continue;
            
            BasicBlock* preHeader = m_insertionSet.insertBefore(loop.header());
            preHeader->appendNode(
                m_graph, SpecNone, Jump, loop.header()->at(0)->codeOrigin, OpInfo(loop.header()));
            
            for (unsigned predecessorIndex = 0; predecessorIndex < loop.header()->predecessors.size(); predecessorIndex++) {
                BasicBlock* predecessor = loop.header()->predecessors[predecessorIndex];
                if (m_graph.m_dominators.dominates(loop.header(), predecessor))
                    continue;
                loop.header()->predecessors[predecessorIndex--] = loop.header()->predecessors.last();
                loop.header()->predecessors.takeLast();
                for (unsigned successorIndex = predecessor->numSuccessors(); successorIndex--;) {
                    BasicBlock*& successor = predecessor->successor(successorIndex);
                    if (successor != loop.header())
                        continue;
                    successor = preHeader;
                    preHeader->predecessors.append(predecessor);
                }
            }
            
            loop.header()->predecessors.append(preHeader);
        }
        
        return m_insertionSet.execute();
    }

    BlockInsertionSet m_insertionSet;
};

bool performLoopPreHeaderCreation(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Loop Pre-Header Creation Phase");
    return runPhase<LoopPreHeaderCreationPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)


