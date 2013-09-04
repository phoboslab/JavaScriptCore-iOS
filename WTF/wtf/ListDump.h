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

#ifndef ListDump_h
#define ListDump_h

#include "CommaPrinter.h"
#include "PrintStream.h"
#include "StringPrintStream.h"

namespace WTF {

template<typename T>
class ListDump {
public:
    ListDump(const T& list, const char* comma)
        : m_list(list)
        , m_comma(comma)
    {
    }
    
    void dump(PrintStream& out) const
    {
        for (typename T::const_iterator iter = m_list.begin(); iter != m_list.end(); ++iter)
            out.print(m_comma, *iter);
    }

private:
    const T& m_list;
    CommaPrinter m_comma;
};

template<typename T>
class MapDump {
public:
    MapDump(const T& map, const char* arrow, const char* comma)
        : m_map(map)
        , m_arrow(arrow)
        , m_comma(comma)
    {
    }
    
    void dump(PrintStream& out) const
    {
        for (typename T::const_iterator iter = m_map.begin(); iter != m_map.end(); ++iter)
            out.print(m_comma, iter->key, m_arrow, iter->value);
    }
    
private:
    const T& m_map;
    const char* m_arrow;
    CommaPrinter m_comma;
};

template<typename T>
ListDump<T> listDump(const T& list, const char* comma = ", ")
{
    return ListDump<T>(list, comma);
}

template<typename T, typename Comparator>
CString sortedListDump(const T& list, const Comparator& comparator, const char* comma = ", ")
{
    Vector<typename T::ValueType> myList;
    myList.appendRange(list.begin(), list.end());
    std::sort(myList.begin(), myList.end(), comparator);
    StringPrintStream out;
    CommaPrinter commaPrinter(comma);
    for (unsigned i = 0; i < myList.size(); ++i)
        out.print(commaPrinter, myList[i]);
    return out.toCString();
}

template<typename T>
inline bool lessThan(const T& a, const T& b)
{
    return a < b;
}

template<typename T>
CString sortedListDump(const T& list, const char* comma = ", ")
{
    return sortedListDump(list, lessThan<typename T::ValueType>, comma);
}

template<typename T>
MapDump<T> mapDump(const T& map, const char* arrow = "=>", const char* comma = ", ")
{
    return MapDump<T>(map, arrow, comma);
}

template<typename T, typename Comparator>
CString sortedMapDump(const T& map, const Comparator& comparator, const char* arrow = "=>", const char* comma = ", ")
{
    Vector<typename T::KeyType> keys;
    for (typename T::const_iterator iter = map.begin(); iter != map.end(); ++iter)
        keys.append(iter->key);
    std::sort(keys.begin(), keys.end(), comparator);
    StringPrintStream out;
    CommaPrinter commaPrinter(comma);
    for (unsigned i = 0; i < keys.size(); ++i)
        out.print(commaPrinter, keys[i], arrow, map.get(keys[i]));
    return out.toCString();
}

} // namespace WTF

using WTF::listDump;
using WTF::sortedListDump;
using WTF::mapDump;
using WTF::sortedMapDump;

#endif // ListDump_h

