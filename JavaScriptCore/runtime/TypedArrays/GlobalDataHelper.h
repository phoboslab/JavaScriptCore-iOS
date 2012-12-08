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
	ASSERT_UNUSED(&globalData, &globalData);
	// PL FIXME: this should return a copy per globalData. I think.
	return staticTable;
}


template<class ConstructorClass>
static inline JSC::JSObject* getDOMConstructor(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject)
{
	static ConstructorClass * globalConstructor;
	if( !globalConstructor ) {
		globalConstructor = ConstructorClass::create(exec, ConstructorClass::createStructure(exec->globalData(), globalObject, globalObject->objectPrototype()), globalObject);
	}
	return (JSC::JSObject *)globalConstructor;
}


template<class PrototypeClass>
static inline JSC::JSObject* getDOMPrototype(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject)
{
	static PrototypeClass * globalPrototype;
	if( !globalPrototype ) {
		JSC::JSGlobalData &data = exec->globalData();
		
		globalPrototype = PrototypeClass::create(data, globalObject,
			PrototypeClass::createStructure(data, globalObject, globalObject->objectPrototype()));
	}
	return (JSC::JSObject *)globalPrototype;
}


}

#endif /* defined(__JavaScriptCore__GlobalDataHelper__) */
