/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CopyVisitor.h"

#include "CopiedSpaceInlines.h"
#include "CopyVisitorInlines.h"
#include "CopyWorkList.h"
#include "GCThreadSharedData.h"
#include "JSCell.h"
#include "JSObject.h"
#include "Operations.h"
#include <wtf/Threading.h>

namespace JSC {

CopyVisitor::CopyVisitor(GCThreadSharedData& shared)
    : m_shared(shared)
{
}

void CopyVisitor::copyFromShared()
{
    size_t next, end;
    m_shared.getNextBlocksToCopy(next, end);
    while (next < end) {
        for (; next < end; ++next) {
            CopiedBlock* block = m_shared.m_blocksToCopy[next];
            if (!block->hasWorkList())
                continue;

            CopyWorkList& workList = block->workList();
            for (CopyWorkList::iterator it = workList.begin(); it != workList.end(); ++it)
                visitItem(*it);

            ASSERT(!block->liveBytes());
            m_shared.m_copiedSpace->recycleEvacuatedBlock(block);
        }
        m_shared.getNextBlocksToCopy(next, end);
    }
    ASSERT(next == end);
}

} // namespace JSC
