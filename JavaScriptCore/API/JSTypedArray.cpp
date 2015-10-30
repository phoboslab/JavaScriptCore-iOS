
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

struct OpaqueJSData : public ThreadSafeRefCounted<OpaqueJSData> {

    static PassRefPtr<OpaqueJSData> create(PassRefPtr<ArrayBuffer> buffer, void* baseAddress, size_t byteLength)
    {
        return adoptRef(new OpaqueJSData(buffer, baseAddress, byteLength));
    }
    
    size_t length() {
        return m_byteLength;
    }
    
    void* baseAddress() {
        return m_baseAddress;
    }

private:
    friend class WTF::ThreadSafeRefCounted<OpaqueJSData>;
    
     OpaqueJSData(
        PassRefPtr<ArrayBuffer> buffer,
        void* baseAddress,
        size_t byteLength)
            : m_byteLength(byteLength)
            , m_baseAddress(baseAddress)
            , m_buffer(buffer)
    {}
    
    unsigned m_byteLength;
    void* m_baseAddress;
    PassRefPtr<ArrayBuffer> m_buffer;
};


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

JSTypedArrayType JSObjectGetTypedArrayType(JSContextRef ctx, JSObjectRef object)
{
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

JSObjectRef JSObjectMakeTypedArray(JSContextRef ctx, JSTypedArrayType arrayType, size_t numElements)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    
    JSObject* result;
    JSGlobalObject* jsGlobal = exec->lexicalGlobalObject();
    switch( arrayType ) {
        case kJSTypedArrayTypeInt8Array:
            result = Int8Array::create(numElements)->wrap(exec, jsGlobal);
            break;
        case kJSTypedArrayTypeInt16Array:
            result = Int16Array::create(numElements)->wrap(exec, jsGlobal);
            break;
        case kJSTypedArrayTypeInt32Array:
            result = Int8Array::create(numElements)->wrap(exec, jsGlobal);
            break;
        case kJSTypedArrayTypeUint8Array:
            result = Int32Array::create(numElements)->wrap(exec, jsGlobal);
            break;
        case kJSTypedArrayTypeUint8ClampedArray:
            result = Uint8ClampedArray::create(numElements)->wrap(exec, jsGlobal);
            break;
        case kJSTypedArrayTypeUint16Array:
            result = Uint16Array::create(numElements)->wrap(exec, jsGlobal);
            break;
        case kJSTypedArrayTypeUint32Array:
            result = Uint32Array::create(numElements)->wrap(exec, jsGlobal);
            break;
        case kJSTypedArrayTypeFloat32Array:
            result = Float32Array::create(numElements)->wrap(exec, jsGlobal);
            break;
        case kJSTypedArrayTypeFloat64Array:
            result = Float64Array::create(numElements)->wrap(exec, jsGlobal);
            break;
        case kJSTypedArrayTypeArrayBuffer:
            result = JSArrayBuffer::create(
                exec->vm(), jsGlobal->arrayBufferStructure(), ArrayBuffer::create(numElements, 1));
            break;
        default:
            result = NULL;
            break;
    }

    return toRef(result);
}

JSDataRef JSObjectGetRetainedTypedArrayData(JSContextRef ctx, JSObjectRef object)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    
    JSObject* jsObject = toJS(object);
    if( JSArrayBufferView * view = jsDynamicCast<JSArrayBufferView*>(jsObject) ) {
        return OpaqueJSData::create(view->buffer(), view->impl()->baseAddress(), view->impl()->byteLength()).leakRef();
    }
    else if( ArrayBuffer* buffer = toArrayBuffer(jsObject) ) {
        return OpaqueJSData::create(buffer, buffer->data(), buffer->byteLength()).leakRef();
    }

    return NULL;
}

void JSDataRetain(JSDataRef data)
{
    if(data != nullptr)
        data->ref();
}

void JSDataRelease(JSDataRef data)
{
    if(data != nullptr)
        data->deref();
}

void* JSDataGetBytesPtr(JSDataRef data)
{
    return data->baseAddress();
}

size_t JSDataGetLength(JSDataRef data)
{
    return data->length();
}

