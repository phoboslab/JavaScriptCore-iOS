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

#ifndef Insertion_h
#define Insertion_h

namespace WTF {

template<typename T>
class Insertion {
public:
    Insertion() { }
    
    Insertion(size_t index, T element)
        : m_index(index)
        , m_element(element)
    {
    }
    
    size_t index() const { return m_index; }
    T element() const { return m_element; }
    
    bool operator<(const Insertion& other) const
    {
        return m_index < other.m_index;
    }
    
private:
    size_t m_index;
    T m_element;
};

template<typename TargetVectorType, typename InsertionVectorType>
void executeInsertions(TargetVectorType& target, InsertionVectorType& insertions)
{
    if (!insertions.size())
        return;
    target.grow(target.size() + insertions.size());
    size_t lastIndex = target.size();
    for (size_t indexInInsertions = insertions.size(); indexInInsertions--;) {
        size_t firstIndex = insertions[indexInInsertions].index() + indexInInsertions;
        size_t indexOffset = indexInInsertions + 1;
        for (size_t i = lastIndex; --i > firstIndex;)
            target[i] = target[i - indexOffset];
        target[firstIndex] = insertions[indexInInsertions].element();
        lastIndex = firstIndex;
    }
    insertions.resize(0);
}

} // namespace WTF

using WTF::Insertion;
using WTF::executeInsertions;

#endif // Insertion_h
