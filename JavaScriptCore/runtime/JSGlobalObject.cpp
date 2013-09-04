/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSGlobalObject.h"

#include "Arguments.h"
#include "ArrayConstructor.h"
#include "ArrayPrototype.h"
#include "BooleanConstructor.h"
#include "BooleanPrototype.h"
#include "CodeBlock.h"
#include "CodeCache.h"
#include "DateConstructor.h"
#include "DatePrototype.h"
#include "Debugger.h"
#include "Error.h"
#include "ErrorConstructor.h"
#include "ErrorPrototype.h"
#include "FunctionConstructor.h"
#include "FunctionPrototype.h"
#include "GetterSetter.h"
#include "Interpreter.h"
#include "JSAPIWrapperObject.h"
#include "JSActivation.h"
#include "JSArrayBuffer.h"
#include "JSArrayBufferConstructor.h"
#include "JSArrayBufferPrototype.h"
#include "JSBoundFunction.h"
#include "JSCallbackConstructor.h"
#include "JSCallbackFunction.h"
#include "JSCallbackObject.h"
#include "JSDataView.h"
#include "JSDataViewPrototype.h"
#include "JSFunction.h"
#include "JSGenericTypedArrayViewConstructorInlines.h"
#include "JSGenericTypedArrayViewInlines.h"
#include "JSGenericTypedArrayViewPrototypeInlines.h"
#include "JSGlobalObjectFunctions.h"
#include "JSLock.h"
#include "JSNameScope.h"
#include "JSONObject.h"
#include "JSPromise.h"
#include "JSPromiseCallback.h"
#include "JSPromiseConstructor.h"
#include "JSPromisePrototype.h"
#include "JSPromiseResolver.h"
#include "JSPromiseResolverConstructor.h"
#include "JSPromiseResolverPrototype.h"
#include "JSTypedArrayConstructors.h"
#include "JSTypedArrayPrototypes.h"
#include "JSTypedArrays.h"
#include "JSWithScope.h"
#include "LegacyProfiler.h"
#include "Lookup.h"
#include "MathObject.h"
#include "NameConstructor.h"
#include "NameInstance.h"
#include "NamePrototype.h"
#include "NativeErrorConstructor.h"
#include "NativeErrorPrototype.h"
#include "NumberConstructor.h"
#include "NumberPrototype.h"
#include "ObjCCallbackFunction.h"
#include "ObjectConstructor.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "ParserError.h"
#include "RegExpConstructor.h"
#include "RegExpMatchesArray.h"
#include "RegExpObject.h"
#include "RegExpPrototype.h"
#include "StrictEvalActivation.h"
#include "StringConstructor.h"
#include "StringPrototype.h"

#include "JSGlobalObject.lut.h"

namespace JSC {

const ClassInfo JSGlobalObject::s_info = { "GlobalObject", &Base::s_info, 0, ExecState::globalObjectTable, CREATE_METHOD_TABLE(JSGlobalObject) };

const GlobalObjectMethodTable JSGlobalObject::s_globalObjectMethodTable = { &allowsAccessFrom, &supportsProfiling, &supportsRichSourceInfo, &shouldInterruptScript, &javaScriptExperimentsEnabled, 0 };

/* Source for JSGlobalObject.lut.h
@begin globalObjectTable
  parseInt              globalFuncParseInt              DontEnum|Function 2
  parseFloat            globalFuncParseFloat            DontEnum|Function 1
  isNaN                 globalFuncIsNaN                 DontEnum|Function 1
  isFinite              globalFuncIsFinite              DontEnum|Function 1
  escape                globalFuncEscape                DontEnum|Function 1
  unescape              globalFuncUnescape              DontEnum|Function 1
  decodeURI             globalFuncDecodeURI             DontEnum|Function 1
  decodeURIComponent    globalFuncDecodeURIComponent    DontEnum|Function 1
  encodeURI             globalFuncEncodeURI             DontEnum|Function 1
  encodeURIComponent    globalFuncEncodeURIComponent    DontEnum|Function 1
@end
*/

JSGlobalObject::JSGlobalObject(VM& vm, Structure* structure, const GlobalObjectMethodTable* globalObjectMethodTable)
    : Base(vm, structure, 0)
    , m_masqueradesAsUndefinedWatchpoint(adoptRef(new WatchpointSet(InitializedWatching)))
    , m_havingABadTimeWatchpoint(adoptRef(new WatchpointSet(InitializedWatching)))
    , m_varInjectionWatchpoint(adoptRef(new WatchpointSet(InitializedWatching)))
    , m_weakRandom(Options::forceWeakRandomSeed() ? Options::forcedWeakRandomSeed() : static_cast<unsigned>(randomNumber() * (std::numeric_limits<unsigned>::max() + 1.0)))
    , m_evalEnabled(true)
    , m_globalObjectMethodTable(globalObjectMethodTable ? globalObjectMethodTable : &s_globalObjectMethodTable)
{
}

JSGlobalObject::~JSGlobalObject()
{
    if (m_debugger)
        m_debugger->detach(this);

    if (LegacyProfiler* profiler = vm().enabledProfiler())
        profiler->stopProfiling(this);
}

void JSGlobalObject::destroy(JSCell* cell)
{
    static_cast<JSGlobalObject*>(cell)->JSGlobalObject::~JSGlobalObject();
}

void JSGlobalObject::setGlobalThis(VM& vm, JSObject* globalThis)
{ 
    m_globalThis.set(vm, this, globalThis);
}

void JSGlobalObject::init(JSObject* thisValue)
{
    ASSERT(vm().currentThreadIsHoldingAPILock());

    setGlobalThis(vm(), thisValue);
    JSGlobalObject::globalExec()->init(0, 0, this, CallFrame::noCaller(), 0, 0);

    m_debugger = 0;

    reset(prototype());
}

void JSGlobalObject::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    JSGlobalObject* thisObject = jsCast<JSGlobalObject*>(cell);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(thisObject));

    if (symbolTablePut(thisObject, exec, propertyName, value, slot.isStrictMode()))
        return;
    Base::put(thisObject, exec, propertyName, value, slot);
}

bool JSGlobalObject::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName propertyName, const PropertyDescriptor& descriptor, bool shouldThrow)
{
    JSGlobalObject* thisObject = jsCast<JSGlobalObject*>(object);
    PropertySlot slot(thisObject);
    // silently ignore attempts to add accessors aliasing vars.
    if (descriptor.isAccessorDescriptor() && symbolTableGet(thisObject, propertyName, slot))
        return false;
    return Base::defineOwnProperty(thisObject, exec, propertyName, descriptor, shouldThrow);
}

int JSGlobalObject::addGlobalVar(const Identifier& ident, ConstantMode constantMode, FunctionMode functionMode)
{
    ConcurrentJITLocker locker(symbolTable()->m_lock);
    int index = symbolTable()->size(locker);
    SymbolTableEntry newEntry(index, (constantMode == IsConstant) ? ReadOnly : 0);
    if (functionMode == IsFunctionToSpecialize)
        newEntry.attemptToWatch();
    SymbolTable::Map::AddResult result = symbolTable()->add(locker, ident.impl(), newEntry);
    if (result.isNewEntry)
        addRegisters(1);
    else {
        result.iterator->value.notifyWrite();
        index = result.iterator->value.getIndex();
    }
    return index;
}

static inline JSObject* lastInPrototypeChain(JSObject* object)
{
    JSObject* o = object;
    while (o->prototype().isObject())
        o = asObject(o->prototype());
    return o;
}

void JSGlobalObject::reset(JSValue prototype)
{
    ExecState* exec = JSGlobalObject::globalExec();

    m_functionPrototype.set(exec->vm(), this, FunctionPrototype::create(exec, this, FunctionPrototype::createStructure(exec->vm(), this, jsNull()))); // The real prototype will be set once ObjectPrototype is created.
    m_functionStructure.set(exec->vm(), this, JSFunction::createStructure(exec->vm(), this, m_functionPrototype.get()));
    m_boundFunctionStructure.set(exec->vm(), this, JSBoundFunction::createStructure(exec->vm(), this, m_functionPrototype.get()));
    m_namedFunctionStructure.set(exec->vm(), this, Structure::addPropertyTransition(exec->vm(), m_functionStructure.get(), exec->vm().propertyNames->name, DontDelete | ReadOnly | DontEnum, 0, m_functionNameOffset));
    m_internalFunctionStructure.set(exec->vm(), this, InternalFunction::createStructure(exec->vm(), this, m_functionPrototype.get()));
    JSFunction* callFunction = 0;
    JSFunction* applyFunction = 0;
    m_functionPrototype->addFunctionProperties(exec, this, &callFunction, &applyFunction);
    m_callFunction.set(exec->vm(), this, callFunction);
    m_applyFunction.set(exec->vm(), this, applyFunction);
    m_objectPrototype.set(exec->vm(), this, ObjectPrototype::create(exec, this, ObjectPrototype::createStructure(exec->vm(), this, jsNull())));
    GetterSetter* protoAccessor = GetterSetter::create(exec);
    protoAccessor->setGetter(exec->vm(), JSFunction::create(exec, this, 0, String(), globalFuncProtoGetter));
    protoAccessor->setSetter(exec->vm(), JSFunction::create(exec, this, 0, String(), globalFuncProtoSetter));
    m_objectPrototype->putDirectAccessor(exec, exec->propertyNames().underscoreProto, protoAccessor, Accessor | DontEnum);
    m_functionPrototype->structure()->setPrototypeWithoutTransition(exec->vm(), m_objectPrototype.get());
    
    m_arrayBufferPrototype.set(exec->vm(), this, JSArrayBufferPrototype::create(exec, this, JSArrayBufferPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_typedArrays[toIndex(TypeInt8)].prototype.set(exec->vm(), this, JSInt8ArrayPrototype::create(exec, this, JSInt8ArrayPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_typedArrays[toIndex(TypeInt16)].prototype.set(exec->vm(), this, JSInt16ArrayPrototype::create(exec, this, JSInt16ArrayPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_typedArrays[toIndex(TypeInt32)].prototype.set(exec->vm(), this, JSInt32ArrayPrototype::create(exec, this, JSInt32ArrayPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_typedArrays[toIndex(TypeUint8)].prototype.set(exec->vm(), this, JSUint8ArrayPrototype::create(exec, this, JSUint8ArrayPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_typedArrays[toIndex(TypeUint8Clamped)].prototype.set(exec->vm(), this, JSUint8ClampedArrayPrototype::create(exec, this, JSUint8ClampedArrayPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_typedArrays[toIndex(TypeUint16)].prototype.set(exec->vm(), this, JSUint16ArrayPrototype::create(exec, this, JSUint16ArrayPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_typedArrays[toIndex(TypeUint32)].prototype.set(exec->vm(), this, JSUint32ArrayPrototype::create(exec, this, JSUint32ArrayPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_typedArrays[toIndex(TypeFloat32)].prototype.set(exec->vm(), this, JSFloat32ArrayPrototype::create(exec, this, JSFloat32ArrayPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_typedArrays[toIndex(TypeFloat64)].prototype.set(exec->vm(), this, JSFloat64ArrayPrototype::create(exec, this, JSFloat64ArrayPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_typedArrays[toIndex(TypeDataView)].prototype.set(exec->vm(), this, JSDataViewPrototype::create(exec->vm(), JSDataViewPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    
    m_arrayBufferStructure.set(exec->vm(), this, JSArrayBuffer::createStructure(exec->vm(), this, m_arrayBufferPrototype.get()));
    m_typedArrays[toIndex(TypeInt8)].structure.set(exec->vm(), this, JSInt8Array::createStructure(exec->vm(), this, m_typedArrays[toIndex(TypeInt8)].prototype.get()));
    m_typedArrays[toIndex(TypeInt16)].structure.set(exec->vm(), this, JSInt16Array::createStructure(exec->vm(), this, m_typedArrays[toIndex(TypeInt16)].prototype.get()));
    m_typedArrays[toIndex(TypeInt32)].structure.set(exec->vm(), this, JSInt32Array::createStructure(exec->vm(), this, m_typedArrays[toIndex(TypeInt32)].prototype.get()));
    m_typedArrays[toIndex(TypeUint8)].structure.set(exec->vm(), this, JSUint8Array::createStructure(exec->vm(), this, m_typedArrays[toIndex(TypeUint8)].prototype.get()));
    m_typedArrays[toIndex(TypeUint8Clamped)].structure.set(exec->vm(), this, JSUint8ClampedArray::createStructure(exec->vm(), this, m_typedArrays[toIndex(TypeUint8Clamped)].prototype.get()));
    m_typedArrays[toIndex(TypeUint16)].structure.set(exec->vm(), this, JSUint16Array::createStructure(exec->vm(), this, m_typedArrays[toIndex(TypeUint16)].prototype.get()));
    m_typedArrays[toIndex(TypeUint32)].structure.set(exec->vm(), this, JSUint32Array::createStructure(exec->vm(), this, m_typedArrays[toIndex(TypeUint32)].prototype.get()));
    m_typedArrays[toIndex(TypeFloat32)].structure.set(exec->vm(), this, JSFloat32Array::createStructure(exec->vm(), this, m_typedArrays[toIndex(TypeFloat32)].prototype.get()));
    m_typedArrays[toIndex(TypeFloat64)].structure.set(exec->vm(), this, JSFloat64Array::createStructure(exec->vm(), this, m_typedArrays[toIndex(TypeFloat64)].prototype.get()));
    m_typedArrays[toIndex(TypeDataView)].structure.set(exec->vm(), this, JSDataView::createStructure(exec->vm(), this, m_typedArrays[toIndex(TypeDataView)].prototype.get()));

    m_nameScopeStructure.set(exec->vm(), this, JSNameScope::createStructure(exec->vm(), this, jsNull()));
    m_activationStructure.set(exec->vm(), this, JSActivation::createStructure(exec->vm(), this, jsNull()));
    m_strictEvalActivationStructure.set(exec->vm(), this, StrictEvalActivation::createStructure(exec->vm(), this, jsNull()));
    m_withScopeStructure.set(exec->vm(), this, JSWithScope::createStructure(exec->vm(), this, jsNull()));

    m_nullPrototypeObjectStructure.set(exec->vm(), this, JSFinalObject::createStructure(vm(), this, jsNull(), JSFinalObject::defaultInlineCapacity()));

    m_callbackFunctionStructure.set(exec->vm(), this, JSCallbackFunction::createStructure(exec->vm(), this, m_functionPrototype.get()));
    m_argumentsStructure.set(exec->vm(), this, Arguments::createStructure(exec->vm(), this, m_objectPrototype.get()));
    m_callbackConstructorStructure.set(exec->vm(), this, JSCallbackConstructor::createStructure(exec->vm(), this, m_objectPrototype.get()));
    m_callbackObjectStructure.set(exec->vm(), this, JSCallbackObject<JSDestructibleObject>::createStructure(exec->vm(), this, m_objectPrototype.get()));
#if JSC_OBJC_API_ENABLED
    m_objcCallbackFunctionStructure.set(exec->vm(), this, ObjCCallbackFunction::createStructure(exec->vm(), this, m_functionPrototype.get()));
    m_objcWrapperObjectStructure.set(exec->vm(), this, JSCallbackObject<JSAPIWrapperObject>::createStructure(exec->vm(), this, m_objectPrototype.get()));
#endif

    m_arrayPrototype.set(exec->vm(), this, ArrayPrototype::create(exec, this, ArrayPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    
    m_originalArrayStructureForIndexingShape[UndecidedShape >> IndexingShapeShift].set(exec->vm(), this, JSArray::createStructure(exec->vm(), this, m_arrayPrototype.get(), ArrayWithUndecided));
    m_originalArrayStructureForIndexingShape[Int32Shape >> IndexingShapeShift].set(exec->vm(), this, JSArray::createStructure(exec->vm(), this, m_arrayPrototype.get(), ArrayWithInt32));
    m_originalArrayStructureForIndexingShape[DoubleShape >> IndexingShapeShift].set(exec->vm(), this, JSArray::createStructure(exec->vm(), this, m_arrayPrototype.get(), ArrayWithDouble));
    m_originalArrayStructureForIndexingShape[ContiguousShape >> IndexingShapeShift].set(exec->vm(), this, JSArray::createStructure(exec->vm(), this, m_arrayPrototype.get(), ArrayWithContiguous));
    m_originalArrayStructureForIndexingShape[ArrayStorageShape >> IndexingShapeShift].set(exec->vm(), this, JSArray::createStructure(exec->vm(), this, m_arrayPrototype.get(), ArrayWithArrayStorage));
    m_originalArrayStructureForIndexingShape[SlowPutArrayStorageShape >> IndexingShapeShift].set(exec->vm(), this, JSArray::createStructure(exec->vm(), this, m_arrayPrototype.get(), ArrayWithSlowPutArrayStorage));
    for (unsigned i = 0; i < NumberOfIndexingShapes; ++i)
        m_arrayStructureForIndexingShapeDuringAllocation[i] = m_originalArrayStructureForIndexingShape[i];
    
    m_regExpMatchesArrayStructure.set(exec->vm(), this, RegExpMatchesArray::createStructure(exec->vm(), this, m_arrayPrototype.get()));

    m_stringPrototype.set(exec->vm(), this, StringPrototype::create(exec, this, StringPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_stringObjectStructure.set(exec->vm(), this, StringObject::createStructure(exec->vm(), this, m_stringPrototype.get()));

    m_booleanPrototype.set(exec->vm(), this, BooleanPrototype::create(exec, this, BooleanPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_booleanObjectStructure.set(exec->vm(), this, BooleanObject::createStructure(exec->vm(), this, m_booleanPrototype.get()));

    m_numberPrototype.set(exec->vm(), this, NumberPrototype::create(exec, this, NumberPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_numberObjectStructure.set(exec->vm(), this, NumberObject::createStructure(exec->vm(), this, m_numberPrototype.get()));

    m_datePrototype.set(exec->vm(), this, DatePrototype::create(exec, this, DatePrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_dateStructure.set(exec->vm(), this, DateInstance::createStructure(exec->vm(), this, m_datePrototype.get()));

    RegExp* emptyRegex = RegExp::create(exec->vm(), "", NoFlags);
    
    m_regExpPrototype.set(exec->vm(), this, RegExpPrototype::create(exec, this, RegExpPrototype::createStructure(exec->vm(), this, m_objectPrototype.get()), emptyRegex));
    m_regExpStructure.set(exec->vm(), this, RegExpObject::createStructure(exec->vm(), this, m_regExpPrototype.get()));

    m_errorPrototype.set(exec->vm(), this, ErrorPrototype::create(exec, this, ErrorPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_errorStructure.set(exec->vm(), this, ErrorInstance::createStructure(exec->vm(), this, m_errorPrototype.get()));

    m_promisePrototype.set(exec->vm(), this, JSPromisePrototype::create(exec, this, JSPromisePrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_promiseStructure.set(exec->vm(), this, JSPromise::createStructure(exec->vm(), this, m_promisePrototype.get()));

    m_promiseResolverPrototype.set(exec->vm(), this, JSPromiseResolverPrototype::create(exec, this, JSPromiseResolverPrototype::createStructure(exec->vm(), this, m_objectPrototype.get())));
    m_promiseResolverStructure.set(exec->vm(), this, JSPromiseResolver::createStructure(exec->vm(), this, m_promiseResolverPrototype.get()));
    m_promiseCallbackStructure.set(exec->vm(), this, JSPromiseCallback::createStructure(exec->vm(), this, m_functionPrototype.get()));
    m_promiseWrapperCallbackStructure.set(exec->vm(), this, JSPromiseWrapperCallback::createStructure(exec->vm(), this, m_functionPrototype.get()));
    
    // Constructors

    JSCell* objectConstructor = ObjectConstructor::create(exec, this, ObjectConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_objectPrototype.get());
    JSCell* functionConstructor = FunctionConstructor::create(exec, this, FunctionConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_functionPrototype.get());
    JSCell* arrayConstructor = ArrayConstructor::create(exec, this, ArrayConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_arrayPrototype.get());
    JSCell* stringConstructor = StringConstructor::create(exec, this, StringConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_stringPrototype.get());
    JSCell* booleanConstructor = BooleanConstructor::create(exec, this, BooleanConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_booleanPrototype.get());
    JSCell* numberConstructor = NumberConstructor::create(exec, this, NumberConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_numberPrototype.get());
    JSCell* dateConstructor = DateConstructor::create(exec, this, DateConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_datePrototype.get());
    JSCell* promiseConstructor = JSPromiseConstructor::create(exec, this, JSPromiseConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_promisePrototype.get());
    JSCell* promiseResolverConstructor = JSPromiseResolverConstructor::create(exec, this, JSPromiseResolverConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_promiseResolverPrototype.get());

    m_regExpConstructor.set(exec->vm(), this, RegExpConstructor::create(exec, this, RegExpConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_regExpPrototype.get()));

    m_errorConstructor.set(exec->vm(), this, ErrorConstructor::create(exec, this, ErrorConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_errorPrototype.get()));

    Structure* nativeErrorPrototypeStructure = NativeErrorPrototype::createStructure(exec->vm(), this, m_errorPrototype.get());
    Structure* nativeErrorStructure = NativeErrorConstructor::createStructure(exec->vm(), this, m_functionPrototype.get());
    m_evalErrorConstructor.set(exec->vm(), this, NativeErrorConstructor::create(exec, this, nativeErrorStructure, nativeErrorPrototypeStructure, ASCIILiteral("EvalError")));
    m_rangeErrorConstructor.set(exec->vm(), this, NativeErrorConstructor::create(exec, this, nativeErrorStructure, nativeErrorPrototypeStructure, ASCIILiteral("RangeError")));
    m_referenceErrorConstructor.set(exec->vm(), this, NativeErrorConstructor::create(exec, this, nativeErrorStructure, nativeErrorPrototypeStructure, ASCIILiteral("ReferenceError")));
    m_syntaxErrorConstructor.set(exec->vm(), this, NativeErrorConstructor::create(exec, this, nativeErrorStructure, nativeErrorPrototypeStructure, ASCIILiteral("SyntaxError")));
    m_typeErrorConstructor.set(exec->vm(), this, NativeErrorConstructor::create(exec, this, nativeErrorStructure, nativeErrorPrototypeStructure, ASCIILiteral("TypeError")));
    m_URIErrorConstructor.set(exec->vm(), this, NativeErrorConstructor::create(exec, this, nativeErrorStructure, nativeErrorPrototypeStructure, ASCIILiteral("URIError")));

    m_objectPrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, objectConstructor, DontEnum);
    m_functionPrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, functionConstructor, DontEnum);
    m_arrayPrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, arrayConstructor, DontEnum);
    m_booleanPrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, booleanConstructor, DontEnum);
    m_stringPrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, stringConstructor, DontEnum);
    m_numberPrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, numberConstructor, DontEnum);
    m_datePrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, dateConstructor, DontEnum);
    m_regExpPrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, m_regExpConstructor.get(), DontEnum);
    m_errorPrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, m_errorConstructor.get(), DontEnum);
    m_promisePrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, promiseConstructor, DontEnum);
    m_promiseResolverPrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, promiseResolverConstructor, DontEnum);

    putDirectWithoutTransition(exec->vm(), exec->propertyNames().Object, objectConstructor, DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().Function, functionConstructor, DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().Array, arrayConstructor, DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().Boolean, booleanConstructor, DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().String, stringConstructor, DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().Number, numberConstructor, DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().Date, dateConstructor, DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().RegExp, m_regExpConstructor.get(), DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().Error, m_errorConstructor.get(), DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().EvalError, m_evalErrorConstructor.get(), DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().RangeError, m_rangeErrorConstructor.get(), DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().ReferenceError, m_referenceErrorConstructor.get(), DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().SyntaxError, m_syntaxErrorConstructor.get(), DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().TypeError, m_typeErrorConstructor.get(), DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().URIError, m_URIErrorConstructor.get(), DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().Promise, promiseConstructor, DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().PromiseResolver, promiseResolverConstructor, DontEnum);

    m_evalFunction.set(exec->vm(), this, JSFunction::create(exec, this, 1, exec->propertyNames().eval.string(), globalFuncEval));
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().eval, m_evalFunction.get(), DontEnum);

    putDirectWithoutTransition(exec->vm(), exec->propertyNames().JSON, JSONObject::create(exec, this, JSONObject::createStructure(exec->vm(), this, m_objectPrototype.get())), DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().Math, MathObject::create(exec, this, MathObject::createStructure(exec->vm(), this, m_objectPrototype.get())), DontEnum);
    
    JSArrayBufferConstructor* arrayBufferConstructor = JSArrayBufferConstructor::create(this, JSArrayBufferConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_arrayBufferPrototype.get());
    FixedArray<InternalFunction*, NUMBER_OF_TYPED_ARRAY_TYPES> typedArrayConstructors;
    typedArrayConstructors[toIndex(TypeInt8)] = JSInt8ArrayConstructor::create(this, JSInt8ArrayConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_typedArrays[toIndex(TypeInt8)].prototype.get(), "Int8Array");
    typedArrayConstructors[toIndex(TypeInt16)] = JSInt16ArrayConstructor::create(this, JSInt16ArrayConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_typedArrays[toIndex(TypeInt16)].prototype.get(), "Int16Array");
    typedArrayConstructors[toIndex(TypeInt32)] = JSInt32ArrayConstructor::create(this, JSInt32ArrayConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_typedArrays[toIndex(TypeInt32)].prototype.get(), "Int32Array");
    typedArrayConstructors[toIndex(TypeUint8)] = JSUint8ArrayConstructor::create(this, JSUint8ArrayConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_typedArrays[toIndex(TypeUint8)].prototype.get(), "Uint8Array");
    typedArrayConstructors[toIndex(TypeUint8Clamped)] = JSUint8ClampedArrayConstructor::create(this, JSUint8ClampedArrayConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_typedArrays[toIndex(TypeUint8Clamped)].prototype.get(), "Uint8ClampedArray");
    typedArrayConstructors[toIndex(TypeUint16)] = JSUint16ArrayConstructor::create(this, JSUint16ArrayConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_typedArrays[toIndex(TypeUint16)].prototype.get(), "Uint16Array");
    typedArrayConstructors[toIndex(TypeUint32)] = JSUint32ArrayConstructor::create(this, JSUint32ArrayConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_typedArrays[toIndex(TypeUint32)].prototype.get(), "Uint32Array");
    typedArrayConstructors[toIndex(TypeFloat32)] = JSFloat32ArrayConstructor::create(this, JSFloat32ArrayConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_typedArrays[toIndex(TypeFloat32)].prototype.get(), "Float32Array");
    typedArrayConstructors[toIndex(TypeFloat64)] = JSFloat64ArrayConstructor::create(this, JSFloat64ArrayConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_typedArrays[toIndex(TypeFloat64)].prototype.get(), "Float64Array");
    typedArrayConstructors[toIndex(TypeDataView)] = JSDataViewConstructor::create(this, JSDataViewConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), m_typedArrays[toIndex(TypeDataView)].prototype.get(), "DataView");
    
    m_arrayBufferPrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, arrayBufferConstructor, DontEnum);
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().ArrayBuffer, arrayBufferConstructor, DontEnum);
    
    for (unsigned typedArrayIndex = NUMBER_OF_TYPED_ARRAY_TYPES; typedArrayIndex--;) {
        m_typedArrays[typedArrayIndex].prototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, typedArrayConstructors[typedArrayIndex], DontEnum);
        putDirectWithoutTransition(exec->vm(), Identifier(exec, typedArrayConstructors[typedArrayIndex]->name(exec)), typedArrayConstructors[typedArrayIndex], DontEnum);
    }

    GlobalPropertyInfo staticGlobals[] = {
        GlobalPropertyInfo(exec->propertyNames().NaN, jsNaN(), DontEnum | DontDelete | ReadOnly),
        GlobalPropertyInfo(exec->propertyNames().Infinity, jsNumber(std::numeric_limits<double>::infinity()), DontEnum | DontDelete | ReadOnly),
        GlobalPropertyInfo(exec->propertyNames().undefinedKeyword, jsUndefined(), DontEnum | DontDelete | ReadOnly)
    };
    addStaticGlobals(staticGlobals, WTF_ARRAY_LENGTH(staticGlobals));
    
    m_specialPointers[Special::CallFunction] = m_callFunction.get();
    m_specialPointers[Special::ApplyFunction] = m_applyFunction.get();
    m_specialPointers[Special::ObjectConstructor] = objectConstructor;
    m_specialPointers[Special::ArrayConstructor] = arrayConstructor;

    if (m_experimentsEnabled) {
        NamePrototype* privateNamePrototype = NamePrototype::create(exec, NamePrototype::createStructure(exec->vm(), this, m_objectPrototype.get()));
        m_privateNameStructure.set(exec->vm(), this, NameInstance::createStructure(exec->vm(), this, privateNamePrototype));

        JSCell* privateNameConstructor = NameConstructor::create(exec, this, NameConstructor::createStructure(exec->vm(), this, m_functionPrototype.get()), privateNamePrototype);
        privateNamePrototype->putDirectWithoutTransition(exec->vm(), exec->propertyNames().constructor, privateNameConstructor, DontEnum);
        putDirectWithoutTransition(exec->vm(), Identifier(exec, "Name"), privateNameConstructor, DontEnum);
    }

    resetPrototype(exec->vm(), prototype);
}

// Private namespace for helpers for JSGlobalObject::haveABadTime()
namespace {

class ObjectsWithBrokenIndexingFinder : public MarkedBlock::VoidFunctor {
public:
    ObjectsWithBrokenIndexingFinder(MarkedArgumentBuffer&, JSGlobalObject*);
    void operator()(JSCell*);

private:
    MarkedArgumentBuffer& m_foundObjects;
    JSGlobalObject* m_globalObject;
};

ObjectsWithBrokenIndexingFinder::ObjectsWithBrokenIndexingFinder(
    MarkedArgumentBuffer& foundObjects, JSGlobalObject* globalObject)
    : m_foundObjects(foundObjects)
    , m_globalObject(globalObject)
{
}

inline bool hasBrokenIndexing(JSObject* object)
{
    // This will change if we have more indexing types.
    IndexingType type = object->structure()->indexingType();
    // This could be made obviously more efficient, but isn't made so right now, because
    // we expect this to be an unlikely slow path anyway.
    return hasUndecided(type) || hasInt32(type) || hasDouble(type) || hasContiguous(type) || hasFastArrayStorage(type);
}

void ObjectsWithBrokenIndexingFinder::operator()(JSCell* cell)
{
    if (!cell->isObject())
        return;
    
    JSObject* object = asObject(cell);

    // Run this filter first, since it's cheap, and ought to filter out a lot of objects.
    if (!hasBrokenIndexing(object))
        return;
    
    // We only want to have a bad time in the affected global object, not in the entire
    // VM. But we have to be careful, since there may be objects that claim to belong to
    // a different global object that have prototypes from our global object.
    bool foundGlobalObject = false;
    for (JSObject* current = object; ;) {
        if (current->globalObject() == m_globalObject) {
            foundGlobalObject = true;
            break;
        }
        
        JSValue prototypeValue = current->prototype();
        if (prototypeValue.isNull())
            break;
        current = asObject(prototypeValue);
    }
    if (!foundGlobalObject)
        return;
    
    m_foundObjects.append(object);
}

} // end private namespace for helpers for JSGlobalObject::haveABadTime()

void JSGlobalObject::haveABadTime(VM& vm)
{
    ASSERT(&vm == &this->vm());
    
    if (isHavingABadTime())
        return;
    
    // Make sure that all allocations or indexed storage transitions that are inlining
    // the assumption that it's safe to transition to a non-SlowPut array storage don't
    // do so anymore.
    m_havingABadTimeWatchpoint->notifyWrite();
    ASSERT(isHavingABadTime()); // The watchpoint is what tells us that we're having a bad time.
    
    // Make sure that all JSArray allocations that load the appropriate structure from
    // this object now load a structure that uses SlowPut.
    for (unsigned i = 0; i < NumberOfIndexingShapes; ++i)
        m_arrayStructureForIndexingShapeDuringAllocation[i].set(vm, this, originalArrayStructureForIndexingType(ArrayWithSlowPutArrayStorage));
    
    // Make sure that all objects that have indexed storage switch to the slow kind of
    // indexed storage.
    MarkedArgumentBuffer foundObjects; // Use MarkedArgumentBuffer because switchToSlowPutArrayStorage() may GC.
    ObjectsWithBrokenIndexingFinder finder(foundObjects, this);
    vm.heap.objectSpace().forEachLiveCell(finder);
    while (!foundObjects.isEmpty()) {
        JSObject* object = asObject(foundObjects.last());
        foundObjects.removeLast();
        ASSERT(hasBrokenIndexing(object));
        object->switchToSlowPutArrayStorage(vm);
    }
}

bool JSGlobalObject::objectPrototypeIsSane()
{
    return !hasIndexedProperties(m_objectPrototype->structure()->indexingType())
        && m_objectPrototype->prototype().isNull();
}

bool JSGlobalObject::arrayPrototypeChainIsSane()
{
    return !hasIndexedProperties(m_arrayPrototype->structure()->indexingType())
        && m_arrayPrototype->prototype() == m_objectPrototype.get()
        && objectPrototypeIsSane();
}

bool JSGlobalObject::stringPrototypeChainIsSane()
{
    return !hasIndexedProperties(m_stringPrototype->structure()->indexingType())
        && m_stringPrototype->prototype() == m_objectPrototype.get()
        && objectPrototypeIsSane();
}

void JSGlobalObject::createThrowTypeError(ExecState* exec)
{
    JSFunction* thrower = JSFunction::create(exec, this, 0, String(), globalFuncThrowTypeError);
    GetterSetter* getterSetter = GetterSetter::create(exec);
    getterSetter->setGetter(exec->vm(), thrower);
    getterSetter->setSetter(exec->vm(), thrower);
    m_throwTypeErrorGetterSetter.set(exec->vm(), this, getterSetter);
}

// Set prototype, and also insert the object prototype at the end of the chain.
void JSGlobalObject::resetPrototype(VM& vm, JSValue prototype)
{
    setPrototype(vm, prototype);

    JSObject* oldLastInPrototypeChain = lastInPrototypeChain(this);
    JSObject* objectPrototype = m_objectPrototype.get();
    if (oldLastInPrototypeChain != objectPrototype)
        oldLastInPrototypeChain->setPrototype(vm, objectPrototype);
}

void JSGlobalObject::visitChildren(JSCell* cell, SlotVisitor& visitor)
{ 
    JSGlobalObject* thisObject = jsCast<JSGlobalObject*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    Base::visitChildren(thisObject, visitor);

    visitor.append(&thisObject->m_globalThis);

    visitor.append(&thisObject->m_regExpConstructor);
    visitor.append(&thisObject->m_errorConstructor);
    visitor.append(&thisObject->m_evalErrorConstructor);
    visitor.append(&thisObject->m_rangeErrorConstructor);
    visitor.append(&thisObject->m_referenceErrorConstructor);
    visitor.append(&thisObject->m_syntaxErrorConstructor);
    visitor.append(&thisObject->m_typeErrorConstructor);
    visitor.append(&thisObject->m_URIErrorConstructor);

    visitor.append(&thisObject->m_evalFunction);
    visitor.append(&thisObject->m_callFunction);
    visitor.append(&thisObject->m_applyFunction);
    visitor.append(&thisObject->m_throwTypeErrorGetterSetter);

    visitor.append(&thisObject->m_objectPrototype);
    visitor.append(&thisObject->m_functionPrototype);
    visitor.append(&thisObject->m_arrayPrototype);
    visitor.append(&thisObject->m_booleanPrototype);
    visitor.append(&thisObject->m_stringPrototype);
    visitor.append(&thisObject->m_numberPrototype);
    visitor.append(&thisObject->m_datePrototype);
    visitor.append(&thisObject->m_regExpPrototype);
    visitor.append(&thisObject->m_errorPrototype);
    visitor.append(&thisObject->m_promisePrototype);
    visitor.append(&thisObject->m_promiseResolverPrototype);

    visitor.append(&thisObject->m_withScopeStructure);
    visitor.append(&thisObject->m_strictEvalActivationStructure);
    visitor.append(&thisObject->m_activationStructure);
    visitor.append(&thisObject->m_nameScopeStructure);
    visitor.append(&thisObject->m_argumentsStructure);
    for (unsigned i = 0; i < NumberOfIndexingShapes; ++i)
        visitor.append(&thisObject->m_originalArrayStructureForIndexingShape[i]);
    for (unsigned i = 0; i < NumberOfIndexingShapes; ++i)
        visitor.append(&thisObject->m_arrayStructureForIndexingShapeDuringAllocation[i]);
    visitor.append(&thisObject->m_booleanObjectStructure);
    visitor.append(&thisObject->m_callbackConstructorStructure);
    visitor.append(&thisObject->m_callbackFunctionStructure);
    visitor.append(&thisObject->m_callbackObjectStructure);
#if JSC_OBJC_API_ENABLED
    visitor.append(&thisObject->m_objcCallbackFunctionStructure);
    visitor.append(&thisObject->m_objcWrapperObjectStructure);
#endif
    visitor.append(&thisObject->m_dateStructure);
    visitor.append(&thisObject->m_nullPrototypeObjectStructure);
    visitor.append(&thisObject->m_errorStructure);
    visitor.append(&thisObject->m_functionStructure);
    visitor.append(&thisObject->m_boundFunctionStructure);
    visitor.append(&thisObject->m_namedFunctionStructure);
    visitor.append(&thisObject->m_numberObjectStructure);
    visitor.append(&thisObject->m_privateNameStructure);
    visitor.append(&thisObject->m_regExpMatchesArrayStructure);
    visitor.append(&thisObject->m_regExpStructure);
    visitor.append(&thisObject->m_stringObjectStructure);
    visitor.append(&thisObject->m_internalFunctionStructure);
    visitor.append(&thisObject->m_promiseStructure);
    visitor.append(&thisObject->m_promiseResolverStructure);
    visitor.append(&thisObject->m_promiseCallbackStructure);
    visitor.append(&thisObject->m_promiseWrapperCallbackStructure);

    visitor.append(&thisObject->m_arrayBufferPrototype);
    visitor.append(&thisObject->m_arrayBufferStructure);
    
    for (unsigned i = NUMBER_OF_TYPED_ARRAY_TYPES; i--;) {
        visitor.append(&thisObject->m_typedArrays[i].prototype);
        visitor.append(&thisObject->m_typedArrays[i].structure);
    }
}

JSValue JSGlobalObject::toThis(JSCell*, ExecState* exec, ECMAMode ecmaMode)
{
    if (ecmaMode == StrictMode)
        return jsUndefined();
    return exec->globalThisValue();
}

ExecState* JSGlobalObject::globalExec()
{
    return CallFrame::create(m_globalCallFrame + JSStack::CallFrameHeaderSize);
}

void JSGlobalObject::addStaticGlobals(GlobalPropertyInfo* globals, int count)
{
    addRegisters(count);

    for (int i = 0; i < count; ++i) {
        GlobalPropertyInfo& global = globals[i];
        ASSERT(global.attributes & DontDelete);
        
        int index = symbolTable()->size();
        SymbolTableEntry newEntry(index, global.attributes);
        symbolTable()->add(global.identifier.impl(), newEntry);
        registerAt(index).set(vm(), this, global.value);
    }
}

bool JSGlobalObject::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSGlobalObject* thisObject = jsCast<JSGlobalObject*>(object);
    if (getStaticFunctionSlot<Base>(exec, ExecState::globalObjectTable(exec), thisObject, propertyName, slot))
        return true;
    return symbolTableGet(thisObject, propertyName, slot);
}

void JSGlobalObject::clearRareData(JSCell* cell)
{
    jsCast<JSGlobalObject*>(cell)->m_rareData.clear();
}

DynamicGlobalObjectScope::DynamicGlobalObjectScope(VM& vm, JSGlobalObject* dynamicGlobalObject)
    : m_dynamicGlobalObjectSlot(vm.dynamicGlobalObject)
    , m_savedDynamicGlobalObject(m_dynamicGlobalObjectSlot)
{
    if (!m_dynamicGlobalObjectSlot) {
#if ENABLE(ASSEMBLER)
        if (ExecutableAllocator::underMemoryPressure())
            vm.heap.deleteAllCompiledCode();
#endif

        m_dynamicGlobalObjectSlot = dynamicGlobalObject;

        // Reset the date cache between JS invocations to force the VM
        // to observe time zone changes.
        vm.resetDateCache();
    }
    // Clear the exception stack between entries
    vm.clearExceptionStack();
}

void slowValidateCell(JSGlobalObject* globalObject)
{
    RELEASE_ASSERT(globalObject->isGlobalObject());
    ASSERT_GC_OBJECT_INHERITS(globalObject, JSGlobalObject::info());
}

UnlinkedProgramCodeBlock* JSGlobalObject::createProgramCodeBlock(CallFrame* callFrame, ProgramExecutable* executable, JSObject** exception)
{
    ParserError error;
    JSParserStrictness strictness = executable->isStrictMode() ? JSParseStrict : JSParseNormal;
    DebuggerMode debuggerMode = hasDebugger() ? DebuggerOn : DebuggerOff;
    ProfilerMode profilerMode = hasProfiler() ? ProfilerOn : ProfilerOff;
    UnlinkedProgramCodeBlock* unlinkedCode = vm().codeCache()->getProgramCodeBlock(vm(), executable, executable->source(), strictness, debuggerMode, profilerMode, error);

    if (hasDebugger())
        debugger()->sourceParsed(callFrame, executable->source().provider(), error.m_line, error.m_message);

    if (error.m_type != ParserError::ErrorNone) {
        *exception = error.toErrorObject(this, executable->source());
        return 0;
    }
    
    return unlinkedCode;
}

UnlinkedEvalCodeBlock* JSGlobalObject::createEvalCodeBlock(CallFrame* callFrame, EvalExecutable* executable)
{
    ParserError error;
    JSParserStrictness strictness = executable->isStrictMode() ? JSParseStrict : JSParseNormal;
    DebuggerMode debuggerMode = hasDebugger() ? DebuggerOn : DebuggerOff;
    ProfilerMode profilerMode = hasProfiler() ? ProfilerOn : ProfilerOff;
    UnlinkedEvalCodeBlock* unlinkedCode = vm().codeCache()->getEvalCodeBlock(vm(), executable, executable->source(), strictness, debuggerMode, profilerMode, error);

    if (hasDebugger())
        debugger()->sourceParsed(callFrame, executable->source().provider(), error.m_line, error.m_message);

    if (error.m_type != ParserError::ErrorNone) {
        throwVMError(callFrame, error.toErrorObject(this, executable->source()));
        return 0;
    }

    return unlinkedCode;
}

} // namespace JSC
