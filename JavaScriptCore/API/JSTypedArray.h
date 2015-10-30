#ifndef JSTypedArray_h
#define JSTypedArray_h

#include <JavaScriptCore/JSValueRef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
@enum JSType
@abstract     A constant identifying the Typed Array type of a JSValue.
@constant     kJSTypedArrayTypeNone                 Not a Typed Array.
@constant     kJSTypedArrayTypeInt8Array            Int8Array
@constant     kJSTypedArrayTypeInt16Array           Int16Array
@constant     kJSTypedArrayTypeInt32Array           Int32Array
@constant     kJSTypedArrayTypeUint8Array           Int8Array
@constant     kJSTypedArrayTypeUint8ClampedArray    Int8ClampedArray
@constant     kJSTypedArrayTypeUint16Array          Uint16Array
@constant     kJSTypedArrayTypeUint32Array          Uint32Array
@constant     kJSTypedArrayTypeFloat32Array         Float32Array
@constant     kJSTypedArrayTypeFloat64Array         Float64Array
@constant     kJSTypedArrayTypeArrayBuffer          ArrayBuffer
*/
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
    kJSTypedArrayTypeArrayBuffer
} JSTypedArrayType;

/*!
@function
@abstract           Returns a JavaScript value's Typed Array type
@param ctx          The execution context to use.
@param value        The JSValue whose Typed Array type you want to obtain.
@result             A value of type JSTypedArrayType that identifies value's Typed Array type, or kJSTypedArrayTypeNone if the object is not a Typed Array.
*/
JS_EXPORT JSTypedArrayType JSObjectGetTypedArrayType(JSContextRef ctx, JSObjectRef object);

/*!
@function
@abstract           Creates a JavaScript Typed Array with the given number of elements
@param ctx          The execution context to use.
@param arrayType    A value of type JSTypedArrayType identifying the type of array you want to create
@param numElements  The number of elements for the array.
@result             A JSObjectRef that is a Typed Array or NULL if there was an error
*/
JS_EXPORT JSObjectRef JSObjectMakeTypedArray(JSContextRef ctx, JSTypedArrayType arrayType, size_t numElements);

/*! @typedef JSDataRef A Typed Array data buffer. */
typedef struct OpaqueJSData* JSDataRef;

/*!
@function
@abstract           Returns a retained JSDataRef that encapsulates a pointer to a Typed Array's data in memory
@param ctx          The execution context to use.
@param value        The JSObjectRef whose Typed Array type data pointer you want to obtain.
@result             A JSDataRef or NULL if the JSObjectRef is not a Typed Array. The return value is automatically retained and has to be released again with JSDataRelease
*/
JS_EXPORT JSDataRef JSObjectGetRetainedTypedArrayData(JSContextRef ctx, JSObjectRef object);

/*!
@function
@abstract         Retains a JavaScript data object.
@param data       The JSData to retain.
@result           A JSDataRef that is the same as data.
*/
JS_EXPORT JSDataRef JSDataRetain(JSDataRef data);

/*!
@function
@abstract         Releases a JavaScript data object.
@param data       The JSData to release.
*/
JS_EXPORT void JSDataRelease(JSDataRef data);

/*!
@function
@abstract         Returns a pointer to the data buffer that serves as the backing store for a JavaScript data object.
@param data       The JSData whose backing store you want to access.
@result           A pointer to the raw data buffer that serves as data's backing store, which will be deallocated when the data is deallocated.
*/
JS_EXPORT void * JSDataGetBytesPtr(JSDataRef data);

/*!
@function
@abstract         Returns the number of bytes in a JavaScript data object.
@param data       The JSData whose length (in bytes) you want to know.
@result           The number of bytes stored in the data object.
*/
JS_EXPORT size_t JSDataGetLength(JSDataRef data);




#ifdef __cplusplus
}
#endif

#endif /* JSTypedArray_h */
