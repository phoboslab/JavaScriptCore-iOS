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

#ifndef ButterflyInlines_h
#define ButterflyInlines_h

#include "ArrayStorage.h"
#include "Butterfly.h"
#include "CopiedSpaceInlines.h"
#include "CopyVisitor.h"
#include "VM.h"
#include "Structure.h"

namespace JSC {

inline Butterfly* Butterfly::createUninitialized(VM& vm, JSCell* intendedOwner, size_t preCapacity, size_t propertyCapacity, bool hasIndexingHeader, size_t indexingPayloadSizeInBytes)
{
    void* temp;
    size_t size = totalSize(preCapacity, propertyCapacity, hasIndexingHeader, indexingPayloadSizeInBytes);
    RELEASE_ASSERT(vm.heap.tryAllocateStorage(intendedOwner, size, &temp));
    Butterfly* result = fromBase(temp, preCapacity, propertyCapacity);
    return result;
}

inline Butterfly* Butterfly::create(VM& vm, JSCell* intendedOwner, size_t preCapacity, size_t propertyCapacity, bool hasIndexingHeader, const IndexingHeader& indexingHeader, size_t indexingPayloadSizeInBytes)
{
    Butterfly* result = createUninitialized(
        vm, intendedOwner, preCapacity, propertyCapacity, hasIndexingHeader,
        indexingPayloadSizeInBytes);
    if (hasIndexingHeader)
        *result->indexingHeader() = indexingHeader;
    return result;
}

inline Butterfly* Butterfly::create(VM& vm, JSCell* intendedOwner, Structure* structure)
{
    return create(
        vm, intendedOwner, 0, structure->outOfLineCapacity(),
        structure->hasIndexingHeader(intendedOwner), IndexingHeader(), 0);
}

inline Butterfly* Butterfly::createUninitializedDuringCollection(CopyVisitor& visitor, size_t preCapacity, size_t propertyCapacity, bool hasIndexingHeader, size_t indexingPayloadSizeInBytes)
{
    size_t size = totalSize(preCapacity, propertyCapacity, hasIndexingHeader, indexingPayloadSizeInBytes);
    Butterfly* result = fromBase(
        visitor.allocateNewSpace(size),
        preCapacity, propertyCapacity);
    return result;
}

inline void* Butterfly::base(Structure* structure)
{
    return base(indexingHeader()->preCapacity(structure), structure->outOfLineCapacity());
}

inline Butterfly* Butterfly::growPropertyStorage(
    VM& vm, JSCell* intendedOwner, size_t preCapacity, size_t oldPropertyCapacity,
    bool hasIndexingHeader, size_t indexingPayloadSizeInBytes, size_t newPropertyCapacity)
{
    RELEASE_ASSERT(newPropertyCapacity > oldPropertyCapacity);
    Butterfly* result = createUninitialized(
        vm, intendedOwner, preCapacity, newPropertyCapacity, hasIndexingHeader,
        indexingPayloadSizeInBytes);
    memcpy(
        result->propertyStorage() - oldPropertyCapacity,
        propertyStorage() - oldPropertyCapacity,
        totalSize(0, oldPropertyCapacity, hasIndexingHeader, indexingPayloadSizeInBytes));
    return result;
}

inline Butterfly* Butterfly::growPropertyStorage(
    VM& vm, JSCell* intendedOwner, Structure* structure, size_t oldPropertyCapacity,
    size_t newPropertyCapacity)
{
    return growPropertyStorage(
        vm, intendedOwner, indexingHeader()->preCapacity(structure), oldPropertyCapacity,
        structure->hasIndexingHeader(intendedOwner),
        indexingHeader()->indexingPayloadSizeInBytes(structure), newPropertyCapacity);
}

inline Butterfly* Butterfly::growPropertyStorage(
    VM& vm, JSCell* intendedOwner, Structure* oldStructure, size_t newPropertyCapacity)
{
    return growPropertyStorage(
        vm, intendedOwner, oldStructure, oldStructure->outOfLineCapacity(),
        newPropertyCapacity);
}

inline Butterfly* Butterfly::createOrGrowArrayRight(
    Butterfly* oldButterfly, VM& vm, JSCell* intendedOwner, Structure* oldStructure,
    size_t propertyCapacity, bool hadIndexingHeader, size_t oldIndexingPayloadSizeInBytes,
    size_t newIndexingPayloadSizeInBytes)
{
    if (!oldButterfly) {
        return create(
            vm, intendedOwner, 0, propertyCapacity, true, IndexingHeader(),
            newIndexingPayloadSizeInBytes);
    }
    return oldButterfly->growArrayRight(
        vm, intendedOwner, oldStructure, propertyCapacity, hadIndexingHeader,
        oldIndexingPayloadSizeInBytes, newIndexingPayloadSizeInBytes);
}

inline Butterfly* Butterfly::growArrayRight(
    VM& vm, JSCell* intendedOwner, Structure* oldStructure, size_t propertyCapacity,
    bool hadIndexingHeader, size_t oldIndexingPayloadSizeInBytes,
    size_t newIndexingPayloadSizeInBytes)
{
    ASSERT_UNUSED(oldStructure, !indexingHeader()->preCapacity(oldStructure));
    ASSERT_UNUSED(oldStructure, hadIndexingHeader == oldStructure->hasIndexingHeader(intendedOwner));
    void* theBase = base(0, propertyCapacity);
    size_t oldSize = totalSize(0, propertyCapacity, hadIndexingHeader, oldIndexingPayloadSizeInBytes);
    size_t newSize = totalSize(0, propertyCapacity, true, newIndexingPayloadSizeInBytes);
    if (!vm.heap.tryReallocateStorage(intendedOwner, &theBase, oldSize, newSize))
        return 0;
    return fromBase(theBase, 0, propertyCapacity);
}

inline Butterfly* Butterfly::growArrayRight(
    VM& vm, JSCell* intendedOwner, Structure* oldStructure,
    size_t newIndexingPayloadSizeInBytes)
{
    return growArrayRight(
        vm, intendedOwner, oldStructure, oldStructure->outOfLineCapacity(),
        oldStructure->hasIndexingHeader(intendedOwner), 
        indexingHeader()->indexingPayloadSizeInBytes(oldStructure),
        newIndexingPayloadSizeInBytes);
}

inline Butterfly* Butterfly::resizeArray(
    VM& vm, JSCell* intendedOwner, size_t propertyCapacity, bool oldHasIndexingHeader,
    size_t oldIndexingPayloadSizeInBytes, size_t newPreCapacity, bool newHasIndexingHeader,
    size_t newIndexingPayloadSizeInBytes)
{
    Butterfly* result = createUninitialized(
        vm, intendedOwner, newPreCapacity, propertyCapacity, newHasIndexingHeader,
        newIndexingPayloadSizeInBytes);
    // FIXME: This could be made much more efficient if we used the property size,
    // not the capacity.
    void* to = result->propertyStorage() - propertyCapacity;
    void* from = propertyStorage() - propertyCapacity;
    size_t size = std::min(
        totalSize(0, propertyCapacity, oldHasIndexingHeader, oldIndexingPayloadSizeInBytes),
        totalSize(0, propertyCapacity, newHasIndexingHeader, newIndexingPayloadSizeInBytes));
    memcpy(to, from, size);
    return result;
}

inline Butterfly* Butterfly::resizeArray(
    VM& vm, JSCell* intendedOwner, Structure* structure, size_t newPreCapacity,
    size_t newIndexingPayloadSizeInBytes)
{
    bool hasIndexingHeader = structure->hasIndexingHeader(intendedOwner);
    return resizeArray(
        vm, intendedOwner, structure->outOfLineCapacity(), hasIndexingHeader,
        indexingHeader()->indexingPayloadSizeInBytes(structure), newPreCapacity,
        hasIndexingHeader, newIndexingPayloadSizeInBytes);
}

inline Butterfly* Butterfly::unshift(Structure* structure, size_t numberOfSlots)
{
    ASSERT(hasArrayStorage(structure->indexingType()));
    ASSERT(numberOfSlots <= indexingHeader()->preCapacity(structure));
    unsigned propertyCapacity = structure->outOfLineCapacity();
    // FIXME: It would probably be wise to rewrite this as a loop since (1) we know in which
    // direction we're moving memory so we don't need the extra check of memmove and (2) we're
    // moving a small amount of memory in the common case so the throughput of memmove won't
    // amortize the overhead of calling it. And no, we cannot rely on the C++ compiler to
    // inline memmove (particularly since the size argument is likely to be variable), nor can
    // we rely on the compiler to recognize the ordering of the pointer arguments (since
    // propertyCapacity is variable and could cause wrap-around as far as the compiler knows).
    memmove(
        propertyStorage() - numberOfSlots - propertyCapacity,
        propertyStorage() - propertyCapacity,
        sizeof(EncodedJSValue) * propertyCapacity + sizeof(IndexingHeader) + ArrayStorage::sizeFor(0));
    return IndexingHeader::fromEndOf(propertyStorage() - numberOfSlots)->butterfly();
}

inline Butterfly* Butterfly::shift(Structure* structure, size_t numberOfSlots)
{
    ASSERT(hasArrayStorage(structure->indexingType()));
    unsigned propertyCapacity = structure->outOfLineCapacity();
    // FIXME: See comment in unshift(), above.
    memmove(
        propertyStorage() - propertyCapacity + numberOfSlots,
        propertyStorage() - propertyCapacity,
        sizeof(EncodedJSValue) * propertyCapacity + sizeof(IndexingHeader) + ArrayStorage::sizeFor(0));
    return IndexingHeader::fromEndOf(propertyStorage() + numberOfSlots)->butterfly();
}

} // namespace JSC

#endif // ButterflyInlines_h

