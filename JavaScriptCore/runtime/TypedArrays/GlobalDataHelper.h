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

template<class TypeClass>
inline JSC::Structure* getDOMStructure(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject)
{
	if (JSC::Structure* structure = globalObject->typedArrayStructureMap.get(&TypeClass::s_info).get()) {
		return structure;
	}
	
	
	JSC::JSObject * proto = TypeClass::createPrototype(exec, globalObject);
	JSC::Structure *structure = TypeClass::createStructure(exec->globalData(), globalObject, proto);
	
	globalObject->typedArrayStructureMap.set(
		&TypeClass::s_info,
		JSC::WriteBarrier<JSC::Structure>(globalObject->globalData(), globalObject, structure)
	);
	return structure;
}

template<class TypeClass>
inline JSC::JSObject* getDOMPrototype(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject)
{
	return JSC::jsCast<JSC::JSObject*>(asObject(getDOMStructure<TypeClass>(exec, globalObject)->storedPrototype()));
}

}

#endif /* defined(__JavaScriptCore__GlobalDataHelper__) */
