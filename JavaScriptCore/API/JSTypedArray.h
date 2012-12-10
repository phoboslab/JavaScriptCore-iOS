#ifndef JSTypedArray_h
#define JSTypedArray_h

#include <JavaScriptCore/JSValueRef.h>

typedef enum {
    kJSTypedArrayTypeNone,
    kJSTypedArrayTypeInt8Array,
    kJSTypedArrayTypeInt16Array,
    kJSTypedArrayTypeInt32Array,
    kJSTypedArrayTypeUint8Array,
    kJSTypedArrayTypeUint8ClampedArray,
    kJSTypedArrayTypeUint16Array,
    kJSTypedArrayTypeUint32Array,
    kJSTypedArrayTypeFloat32Array,
    kJSTypedArrayTypeFloat64Array,
    kJSTypedArrayTypeLast
} JSTypedArrayType;


#ifdef __cplusplus
extern "C" {
#endif

JS_EXPORT JSTypedArrayType JSTypedArrayGetType(JSContextRef ctx, JSValueRef array);
JS_EXPORT JSValueRef JSTypedArrayMake(JSContextRef ctx, JSTypedArrayType arrayClass, size_t numElements);
JS_EXPORT void * JSTypedArrayGetDataPtr(JSContextRef ctx, JSValueRef array, size_t * byteLength);

#ifdef __cplusplus
}
#endif

#endif /* JSTypedArray_h */
