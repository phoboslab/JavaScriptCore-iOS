# JavaScriptCore iOS

The JavaScriptCore library is part of the [WebKit project](http://www.webkit.org/) and thus Open Source. However, in the sources you get from the [WebKit SVN](https://svn.webkit.org/repository/webkit/trunk), the XCode project files are curiously missing an iOS compile target. You can't compile it for iOS. The sources you get from [opensource.apple.com](http://opensource.apple.com/release/ios-601/) are missing the project files altogether. You can't compile it all. That's quite the Open Source spirit, Apple!

This repo aims to re-produce the missing iOS targets while staying on a somewhat up-to-date version.

Currently, the [Safari 538.4 tag](https://svn.webkit.org/repository/webkit/tags/Safari-538.4/) is used as the basis. With the current settings, the WTF and JavaScriptCore libraries can be compiled for armv7 and x86 (for the iOS simulator). It will be compiled without Unicode collation support, because Apple claims [ICU](http://site.icu-project.org/) is a private framework on iOS. It should be AppStore compatible this way.

This version of JSC deprecates the `typed-arrays` branch of this repository. The updated 538.4 tag now supports Typed Arrays natively, without any hacks.

Note however, that the source code of JSC was still modified from the 538.4 tag:

- It includes some API methods to work with Typed Arrays in native code. Have a look at the `API/JSTypedArray.h`, it declares three new API functions. The documentation for these functions can be found in this header file as well.
- `Number.MIN_VALUE` was fixed to return `DBL_MIN` instead of `0` when the CPU has not enabled support denormal numbers.

## Binaries

A compiled version of the `libJavaScriptCore.a` for armv7 and the Simulator can be found in the [source tree](https://github.com/phoboslab/Ejecta/tree/master/Source/lib) of the [Ejecta project](https://github.com/phoboslab/Ejecta).

## How to Compile

1. Create a `Build/` directory in the top level of this repo. The combined `libWTF.a` and `JavaScriptCore.a` will be placed here

2. Open the `WTF/WTF.xcodeproj` file and compile the "WTF iOS" target once for "iOS Device" and once for "iPhone Simulator". Compile the release version (Shift+Command+I).
3. Compile the "Combine iOS lib" target. This will execute a script that will combine the device and simulator libraries into one and place it in your `Build/` directory.

4. Open the `JavaScriptCore/JavaScriptCore.xcodeproj` file and compile the "JavaScriptCore iOS" target once for "iOS Device" and once for "iPhone Simulator". Compile the release version (Shift+Command+I).
5. Compile the "Combine iOS lib" target. This will execute a script that will combine the device and simulator libraries into one and place it in your `Build/` directory.
