/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef SlotVisitorInlines_h
#define SlotVisitorInlines_h

#include "CopiedBlockInlines.h"
#include "CopiedSpaceInlines.h"
#include "Options.h"
#include "SlotVisitor.h"
#include "Weak.h"
#include "WeakInlines.h"

namespace JSC {

ALWAYS_INLINE void SlotVisitor::append(JSValue* slot, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        JSValue& value = slot[i];
        internalAppend(&value, value);
    }
}

template<typename T>
inline void SlotVisitor::appendUnbarrieredPointer(T** slot)
{
    ASSERT(slot);
    JSCell* cell = *slot;
    internalAppend(slot, cell);
}

ALWAYS_INLINE void SlotVisitor::append(JSValue* slot)
{
    ASSERT(slot);
    internalAppend(slot, *slot);
}

ALWAYS_INLINE void SlotVisitor::appendUnbarrieredValue(JSValue* slot)
{
    ASSERT(slot);
    internalAppend(slot, *slot);
}

ALWAYS_INLINE void SlotVisitor::append(JSCell** slot)
{
    ASSERT(slot);
    internalAppend(slot, *slot);
}

template<typename T>
ALWAYS_INLINE void SlotVisitor::appendUnbarrieredWeak(Weak<T>* weak)
{
    ASSERT(weak);
    if (weak->get())
        internalAppend(0, weak->get());
}

ALWAYS_INLINE void SlotVisitor::internalAppend(void* from, JSValue value)
{
    if (!value || !value.isCell())
        return;
    internalAppend(from, value.asCell());
}

ALWAYS_INLINE void SlotVisitor::internalAppend(void* from, JSCell* cell)
{
    ASSERT(!m_isCheckingForDefaultMarkViolation);
    if (!cell)
        return;
#if ENABLE(ALLOCATION_LOGGING)
    dataLogF("JSC GC noticing reference from %p to %p.\n", from, cell);
#else
    UNUSED_PARAM(from);
#endif
#if ENABLE(GC_VALIDATION)
    validate(cell);
#endif
    if (Heap::testAndSetMarked(cell) || !cell->structure())
        return;

    m_visitCount++;
        
    MARK_LOG_CHILD(*this, cell);

    // Should never attempt to mark something that is zapped.
    ASSERT(!cell->isZapped());
        
    m_stack.append(cell);
}

template<typename T> inline void SlotVisitor::append(WriteBarrierBase<T>* slot)
{
    internalAppend(slot, *slot->slot());
}

ALWAYS_INLINE void SlotVisitor::appendValues(WriteBarrierBase<Unknown>* barriers, size_t count)
{
    append(barriers->slot(), count);
}

inline void SlotVisitor::addWeakReferenceHarvester(WeakReferenceHarvester* weakReferenceHarvester)
{
    m_shared.m_weakReferenceHarvesters.addThreadSafe(weakReferenceHarvester);
}

inline void SlotVisitor::addUnconditionalFinalizer(UnconditionalFinalizer* unconditionalFinalizer)
{
    m_shared.m_unconditionalFinalizers.addThreadSafe(unconditionalFinalizer);
}

inline void SlotVisitor::addOpaqueRoot(void* root)
{
#if ENABLE(PARALLEL_GC)
    if (Options::numberOfGCMarkers() == 1) {
        // Put directly into the shared HashSet.
        m_shared.m_opaqueRoots.add(root);
        return;
    }
    // Put into the local set, but merge with the shared one every once in
    // a while to make sure that the local sets don't grow too large.
    mergeOpaqueRootsIfProfitable();
    m_opaqueRoots.add(root);
#else
    m_opaqueRoots.add(root);
#endif
}

inline bool SlotVisitor::containsOpaqueRoot(void* root)
{
    ASSERT(!m_isInParallelMode);
#if ENABLE(PARALLEL_GC)
    ASSERT(m_opaqueRoots.isEmpty());
    return m_shared.m_opaqueRoots.contains(root);
#else
    return m_opaqueRoots.contains(root);
#endif
}

inline TriState SlotVisitor::containsOpaqueRootTriState(void* root)
{
    if (m_opaqueRoots.contains(root))
        return TrueTriState;
    MutexLocker locker(m_shared.m_opaqueRootsLock);
    if (m_shared.m_opaqueRoots.contains(root))
        return TrueTriState;
    return MixedTriState;
}

inline int SlotVisitor::opaqueRootCount()
{
    ASSERT(!m_isInParallelMode);
#if ENABLE(PARALLEL_GC)
    ASSERT(m_opaqueRoots.isEmpty());
    return m_shared.m_opaqueRoots.size();
#else
    return m_opaqueRoots.size();
#endif
}

inline void SlotVisitor::mergeOpaqueRootsIfNecessary()
{
    if (m_opaqueRoots.isEmpty())
        return;
    mergeOpaqueRoots();
}
    
inline void SlotVisitor::mergeOpaqueRootsIfProfitable()
{
    if (static_cast<unsigned>(m_opaqueRoots.size()) < Options::opaqueRootMergeThreshold())
        return;
    mergeOpaqueRoots();
}
    
inline void SlotVisitor::donate()
{
    ASSERT(m_isInParallelMode);
    if (Options::numberOfGCMarkers() == 1)
        return;
    
    donateKnownParallel();
}

inline void SlotVisitor::donateAndDrain()
{
    donate();
    drain();
}

inline void SlotVisitor::copyLater(JSCell* owner, CopyToken token, void* ptr, size_t bytes)
{
    ASSERT(bytes);
    CopiedBlock* block = CopiedSpace::blockFor(ptr);
    if (block->isOversize()) {
        m_shared.m_copiedSpace->pin(block);
        return;
    }

    if (block->isPinned())
        return;

    block->reportLiveBytes(owner, token, bytes);
}
    
inline void SlotVisitor::reportExtraMemoryUsage(size_t size)
{
    size_t* counter = &m_shared.m_vm->heap.m_extraMemoryUsage;
    
#if ENABLE(COMPARE_AND_SWAP)
    for (;;) {
        size_t oldSize = *counter;
        if (WTF::weakCompareAndSwapSize(counter, oldSize, oldSize + size))
            return;
    }
#else
    (*counter) += size;
#endif
}

} // namespace JSC

#endif // SlotVisitorInlines_h

