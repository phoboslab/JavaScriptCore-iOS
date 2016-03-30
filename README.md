### This Project has been discontinued – Mar. 2016

JSC is now a public library on iOS and tvOS. [Ejecta](https://github.com/phoboslab/Ejecta) has switched to the JSC library provided by the System. It still has some limitations (no native Typed Array API) as of 2016.03.30, but they are mostly workaroundable. 

Also, compiling it for tvOS requires bitcode compilation now, which this fork of JSC does not support. You can read up on the [fascinating details in my blog](http://phoboslab.org/log/2015/11/the-absolute-worst-way-to-read-typed-array-data-with-javascriptcore).

# JavaScriptCore iOS

The JavaScriptCore library is part of the [WebKit project](http://www.webkit.org/) and thus Open Source. However, in the sources you get from the [WebKit SVN](https://svn.webkit.org/repository/webkit/trunk), the XCode project files are curiously missing an iOS compile target. The sources you get from [opensource.apple.com](http://opensource.apple.com/release/ios-601/) are missing the project files altogether. You can't compile it at all. That's quite the Open Source spirit, Apple!

This repo aims to re-produce the missing iOS targets while staying on a somewhat up-to-date version.

Currently, the [Safari-538.11.1 tag](https://svn.webkit.org/repository/webkit/tags/Safari-538.11.1/) is used as the basis. With the current settings, the WTF and JavaScriptCore libraries can be compiled for armv7, armv7s, arm64 and x86 (for the iOS simulator). It will be compiled without Unicode collation support, because Apple claims [ICU](http://site.icu-project.org/) is a private framework on iOS. It should be AppStore compatible this way.

This version of JSC deprecates the `typed-arrays` branch of this repository. JSC now supports Typed Arrays natively, without any hacks.

Note however, that the source code of JSC was still modified:

- It includes some API methods to work with Typed Arrays in native code. Have a look at the `API/JSTypedArray.h`, it declares three new API functions. The documentation for these functions can be found in this header file as well.
- `Number.MIN_VALUE` was fixed to return `DBL_MIN` instead of `0` when the CPU has not enabled support denormal numbers.

## Binaries

A compiled version of the `libJavaScriptCore.a` for armv7, armv7s, arm64 and the Simulator can be found in the [source tree](https://github.com/phoboslab/Ejecta/tree/master/Source/lib) of the [Ejecta project](https://github.com/phoboslab/Ejecta).

## How to Compile

1. Run `python make.py`.
2. Get coffee! Building this takes a while ;P

You can do `python make.py --help` for more options.
