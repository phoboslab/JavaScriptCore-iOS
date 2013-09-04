/*
 * Copyright (C) 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef AtomicStringImpl_h
#define AtomicStringImpl_h

#include "StringImpl.h"

namespace WTF {

class AtomicStringImpl : public StringImpl
{
public:
    AtomicStringImpl() : StringImpl(0) {}
};

#if !ASSERT_DISABLED
// AtomicStringImpls created from StaticASCIILiteral will ASSERT
// in the generic ValueCheck<T>::checkConsistency
// as they are not allocated by fastMalloc.
// We don't currently have any way to detect that case
// so we ignore the consistency check for all AtomicStringImpls*.
template<> struct
ValueCheck<AtomicStringImpl*> {
    static void checkConsistency(const AtomicStringImpl*) { }
};

template<> struct
ValueCheck<const AtomicStringImpl*> {
    static void checkConsistency(const AtomicStringImpl*) { }
};
#endif

}

using WTF::AtomicStringImpl;

#endif
