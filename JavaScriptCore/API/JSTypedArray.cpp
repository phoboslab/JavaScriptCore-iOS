
#include "config.h"

#include "JSTypedArray.h"

#include "JSObjectRef.h"
#include "APICast.h"
#include "InitializeThreading.h"
#include "JSCallbackObject.h"
#include "JSClassRef.h"
#include "JSGlobalObject.h"

#include "JSArrayBuffer.h"
#include "JSFloat32Array.h"
#include "JSFloat64Array.h"
#include "JSInt8Array.h"
#include "JSInt16Array.h"
#include "JSInt32Array.h"
#include "JSUint8ClampedArray.h"
#include "JSUint8Array.h"
#include "JSUint16Array.h"
#include "JSUint32Array.h"


using namespace JSC;
using namespace WebCore;

// Better be safe than sorry!
const JSTypedArrayType TypedArrayTypes[] = {
    [TypedArrayNone] = kJSTypedArrayTypeNone,
    [TypedArrayInt8] = kJSTypedArrayTypeInt8Array,
    [TypedArrayInt16] = kJSTypedArrayTypeInt16Array,
    [TypedArrayInt32] = kJSTypedArrayTypeInt32Array,
    [TypedArrayUint8] = kJSTypedArrayTypeUint8Array,
    [TypedArrayUint8Clamped] = kJSTypedArrayTypeUint8ClampedArray,
    [TypedArrayUint16] = kJSTypedArrayTypeUint16Array,
    [TypedArrayUint32] = kJSTypedArrayTypeUint32Array,
    [TypedArrayFloat32] = kJSTypedArrayTypeFloat32Array,
    [TypedArrayFloat64] = kJSTypedArrayTypeFloat64Array
};

const int kJSTypedArrayTypeLast = kJSTypedArrayTypeFloat64Array;


template <typename ArrayType>JSObject * CreateTypedArray(JSC::ExecState* exec, size_t length) {
    RefPtr<ArrayType> array = ArrayType::create(length);
    if( !array.get() ) {
        return NULL;
    }
    return asObject(toJS(exec, exec->lexicalGlobalObject(), array.get()));
}

typedef JSObject*(*CreateTypedArrayFuncPtr)(JSC::ExecState*, size_t);
const CreateTypedArrayFuncPtr CreateTypedArrayFunc[] = {
    [kJSTypedArrayTypeNone] = NULL,
    [kJSTypedArrayTypeInt8Array] = CreateTypedArray<Int8Array>,
    [kJSTypedArrayTypeInt16Array] = CreateTypedArray<Int16Array>,
    [kJSTypedArrayTypeInt32Array] = CreateTypedArray<Int32Array>,
    [kJSTypedArrayTypeUint8Array] = CreateTypedArray<Uint8Array>,
    [kJSTypedArrayTypeUint8ClampedArray] = CreateTypedArray<Uint8ClampedArray>,
    [kJSTypedArrayTypeUint16Array] = CreateTypedArray<Uint16Array>,
    [kJSTypedArrayTypeUint32Array] = CreateTypedArray<Uint32Array>,
    [kJSTypedArrayTypeFloat32Array] = CreateTypedArray<Float32Array>,
    [kJSTypedArrayTypeFloat64Array] = CreateTypedArray<Float64Array>
};




JSTypedArrayType JSTypedArrayGetType(JSContextRef ctx, JSValueRef value) {
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);
    if( JSObject* object = jsValue.getObject() ) {
        return TypedArrayTypes[object->classInfo()->typedArrayStorageType];
    }
    return kJSTypedArrayTypeNone;
}

JSValueRef JSTypedArrayMake(JSContextRef ctx, JSTypedArrayType arrayType, size_t numElements) {
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    
    JSObject* result;
    if( arrayType > kJSTypedArrayTypeNone && arrayType <= kJSTypedArrayTypeLast ) {
        result = CreateTypedArrayFunc[arrayType]( exec, numElements );
    }

    return toRef(result);
}

void * JSTypedArrayGetDataPtr(JSContextRef ctx, JSValueRef value, size_t * byteLength) {
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    
    JSValue jsValue = toJS(exec, value);
    if( ArrayBufferView * view = toArrayBufferView(jsValue) ) {
        if( byteLength ) {
            *byteLength = view->byteLength();
        }
        return view->baseAddress();
    }
    
    if( byteLength ) {
        *byteLength = 0;
    }
    return NULL;
}



