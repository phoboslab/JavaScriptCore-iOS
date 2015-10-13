
#include "config.h"

#include "JSTypedArray.h"

#include <wtf/RefPtr.h>

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

#include "TypedArrayInlines.h"

using namespace JSC;

// Better be safe than sorry!
const JSTypedArrayType TypedArrayTypes[] = {
    [NotTypedArray] = kJSTypedArrayTypeNone,
    [TypeInt8] = kJSTypedArrayTypeInt8Array,
    [TypeUint8] = kJSTypedArrayTypeUint8Array,
    [TypeUint8Clamped] = kJSTypedArrayTypeUint8ClampedArray,
    [TypeInt16] = kJSTypedArrayTypeInt16Array,
    [TypeUint16] = kJSTypedArrayTypeUint16Array,
    [TypeInt32] = kJSTypedArrayTypeInt32Array,
    [TypeUint32] = kJSTypedArrayTypeUint32Array,
    [TypeFloat32] = kJSTypedArrayTypeFloat32Array,
    [TypeFloat64] = kJSTypedArrayTypeFloat64Array,
    /* not a TypedArray */ kJSTypedArrayTypeArrayBuffer
};

const int kJSTypedArrayTypeLast = kJSTypedArrayTypeArrayBuffer;


template <typename ArrayType>JSObject * CreateTypedArray(JSC::ExecState* exec, size_t length) {
    return ArrayType::create(length)->wrap(exec, exec->lexicalGlobalObject());
}

template <typename BufferType>JSObject * CreateArrayBuffer(JSC::ExecState* exec, size_t length) {
    RefPtr<BufferType> buffer = BufferType::create(length, 1);
    if( !buffer ) {
        return NULL;
    }
    
    JSArrayBuffer* result = JSArrayBuffer::create(
        exec->vm(), exec->lexicalGlobalObject()->arrayBufferStructure(), buffer);
    return result;
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
    [kJSTypedArrayTypeFloat64Array] = CreateTypedArray<Float64Array>,
    [kJSTypedArrayTypeArrayBuffer] = CreateArrayBuffer<ArrayBuffer>,
};




JSTypedArrayType JSObjectGetTypedArrayType(JSContextRef ctx, JSObjectRef object) {
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSObject* jsObject = toJS(object);
    JSTypedArrayType type = kJSTypedArrayTypeNone;
    if( jsObject->inherits(JSArrayBufferView::info()) ) {
        type = TypedArrayTypes[jsObject->classInfo()->typedArrayStorageType];
    }
    else if( jsObject->inherits(JSArrayBuffer::info()) ) {
        type = kJSTypedArrayTypeArrayBuffer;
    }
    return type;
}

JSObjectRef JSObjectMakeTypedArray(JSContextRef ctx, JSTypedArrayType arrayType, size_t numElements) {
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    
    JSObject* result = NULL;
    if( arrayType > kJSTypedArrayTypeNone && arrayType <= kJSTypedArrayTypeLast ) {
        result = CreateTypedArrayFunc[arrayType]( exec, numElements );
    }

    return toRef(result);
}

void* JSObjectGetTypedArrayDataPtr(JSContextRef ctx, JSObjectRef object, size_t* byteLength) {
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    
    JSObject* jsObject = toJS(object);
    if( JSArrayBufferView * view = jsDynamicCast<JSArrayBufferView*>(jsObject) ) {
        if( byteLength ) {
            *byteLength = view->impl()->byteLength();
        }
        return view->impl()->baseAddress();
    }
    else if( ArrayBuffer* buffer = toArrayBuffer(jsObject) ) {
        if( byteLength ) {
            *byteLength = buffer->byteLength();
        }
        return buffer->data();
    }
    
    if( byteLength ) {
        *byteLength = 0;
    }
    return NULL;
}
