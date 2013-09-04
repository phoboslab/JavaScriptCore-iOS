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

#ifndef DFGBasicBlockInlines_h
#define DFGBasicBlockInlines_h

#include "DFGBasicBlock.h"
#include "DFGGraph.h"

#if ENABLE(DFG_JIT)

namespace JSC { namespace DFG {

#define DFG_DEFINE_APPEND_NODE(templatePre, templatePost, typeParams, valueParamsComma, valueParams, valueArgs) \
    templatePre typeParams templatePost inline Node* BasicBlock::appendNode(Graph& graph, SpeculatedType type valueParamsComma valueParams) \
    { \
        Node* result = graph.addNode(type valueParamsComma valueArgs); \
        append(result); \
        return result; \
    }
    DFG_VARIADIC_TEMPLATE_FUNCTION(DFG_DEFINE_APPEND_NODE)
#undef DFG_DEFINE_APPEND_NODE

#define DFG_DEFINE_APPEND_NODE(templatePre, templatePost, typeParams, valueParamsComma, valueParams, valueArgs) \
    templatePre typeParams templatePost inline Node* BasicBlock::appendNonTerminal(Graph& graph, SpeculatedType type valueParamsComma valueParams) \
    { \
        Node* result = graph.addNode(type valueParamsComma valueArgs); \
        insertBeforeLast(result); \
        return result; \
    }
    DFG_VARIADIC_TEMPLATE_FUNCTION(DFG_DEFINE_APPEND_NODE)
#undef DFG_DEFINE_APPEND_NODE

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGBasicBlockInlines_h

