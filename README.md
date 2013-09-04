# About this fork

The goal is to bring the new Objective-C to Javascript bridge to be featured in iOS 7 to iOS 5 and 6. That means that the same code will transition seamlessly to iOS 7 when the time to drop older iOS versions comes.
Some refactoring was involved but nothing major mostly just making sure the project doesn't use undocumented frameworks/functions.

Some good examples on how to use it here:

http://www.steamclock.com/blog/2013/05/apple-objective-c-javascript-bridge/

The most basic example:

	@protocol NativeObjectExport <JSExport>
	-(void)log:(NSString*)string;
	@end

	@interface NativeObject : NSObject <NativeObjectExport>
	@end

	@implementation NativeObject

	-(void)log:(NSString*)string {
	    NSLog(@"js: %@", string);
	}

	@end
	
	JSContext* context = [[JSContext alloc] init];
	context[@"nativeObject"] = [[NativeObject alloc] init];
	[context evaluateScript:@"nativeObject.log(\"Hello Javascript\")"];



A pull request was submitted to merge this into the main repo.

Thanks!

# JavaScriptCore iOS

The JavaScriptCore library is part of the [WebKit project](http://www.webkit.org/) and thus Open Source. However, in the sources you get from the [WebKit SVN](https://svn.webkit.org/repository/webkit/trunk), the XCode project files are curiously missing an iOS compile target. You can't compile it for iOS. The sources you get from [opensource.apple.com](http://opensource.apple.com/release/ios-601/) are missing the project files altogether. You can't compile it all. That's quite the Open Source spirit, Apple!

This repo aims to re-produce the missing iOS targets while staying on a somewhat up-to-date version.

Currently, the [Safari 6.0.2 release](https://svn.webkit.org/repository/webkit/releases/Apple/Safari%206.0.2/) is used as the basis. With the current settings, the WTF and JavaScriptCore libraries can be compiled for armv7 and x86 (for the iOS simulator). It will be compiled without Unicode collation support, because Apple claims [ICU](http://site.icu-project.org/) is a private framework on iOS. It should be AppStore compatible this way.

## Binaries

A compiled version of the `libJavaScriptCore.a` for armv7 and the Simulator can be found in the [source tree](https://github.com/phoboslab/Ejecta/tree/master/Source/lib) of the [Ejecta project](https://github.com/phoboslab/Ejecta).

## How to Compile

1. Create a `Build/` directory in the top level of this repo. The combined `libWTF.a` and `JavaScriptCore.a` will be placed here

2. Open the `WTF/WTF.xcodeproj` file and compile the "WTF iOS" target once for "iOS Device" and once for "iPhone Simulator". Compile the release version (Shift+Command+I).
3. Compile the "Combine iOS lib" target. This will execute a script that will combine the device and simulator libraries into one and place it in your `Build/` directory.

4. Open the `JavaScriptCore/JavaScriptCore.xcodeproj` file and compile the "JavaScriptCore iOS" target once for "iOS Device" and once for "iPhone Simulator". Compile the release version (Shift+Command+I).
5. Compile the "Combine iOS lib" target. This will execute a script that will combine the device and simulator libraries into one and place it in your `Build/` directory.
