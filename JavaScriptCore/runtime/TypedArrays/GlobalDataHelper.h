#ifndef __JavaScriptCore__GlobalDataHelper__
#define __JavaScriptCore__GlobalDataHelper__

#include <wtf/HashMap.h>
#include "Lookup.h"

#include "JSObject.h"
#include "InternalFunction.h"

#include <runtime/JSGlobalObject.h>
#include <runtime/JSObject.h>
#include <runtime/ObjectPrototype.h>
#include <wtf/ArrayBuffer.h>

namespace WebCore {

enum ParameterDefaultPolicy {
    DefaultIsUndefined,
    DefaultIsNullString
};


#define MAYBE_MISSING_PARAMETER(exec, index, policy) (((policy) == DefaultIsNullString && (index) >= (exec)->argumentCount()) ? (JSValue()) : ((exec)->argument(index)))

static inline const JSC::HashTable* getHashTableForGlobalData(JSC::JSGlobalData& globalData, const JSC::HashTable* staticTable) {
	return globalData.typedArrayHashTableMap.get(staticTable);
}

template<class ConstructorClass>
inline JSC::JSObject* getDOMConstructor(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject)
{
	if (JSC::JSObject* constructor = globalObject->typedArrayConstructorMap.get(&ConstructorClass::s_info).get())
		return constructor;
		
	JSC::JSObject* constructor = ConstructorClass::create(exec, ConstructorClass::createStructure(exec->globalData(), globalObject, globalObject->objectPrototype()), globalObject);
	
	ASSERT(!globalObject->typedArrayConstructorMap.contains(&ConstructorClass::s_info));
	JSC::WriteBarrier<JSC::JSObject> temp;
	globalObject->typedArrayConstructorMap.add(&ConstructorClass::s_info, temp).iterator->second.set(exec->globalData(), globalObject, constructor);
	return constructor;
}

template<class PrototypeClass>
inline JSC::JSObject* getDOMPrototype(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject)
{
	if (JSC::JSObject* prototype = globalObject->typedArrayPrototypeMap.get(&PrototypeClass::s_info).get())
		return prototype;
		
	JSC::JSObject* prototype = PrototypeClass::create(exec->globalData(), globalObject,
		PrototypeClass::createStructure(exec->globalData(), globalObject, globalObject->objectPrototype()));
	
	ASSERT(!globalObject->typedArrayPrototypeMap.contains(&PrototypeClass::s_info));
	JSC::WriteBarrier<JSC::JSObject> temp;
	globalObject->typedArrayPrototypeMap.add(&PrototypeClass::s_info, temp).iterator->second.set(exec->globalData(), globalObject, prototype);
	return prototype;
}

}

#endif /* defined(__JavaScriptCore__GlobalDataHelper__) */
