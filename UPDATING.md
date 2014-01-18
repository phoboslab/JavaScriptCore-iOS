Steps taken to create JavaScriptCore static iOS library from webkit sources:
---

- Remove JavaScriptCore and WTF folders
$ rm -rf WTF
$ rm -rf JavaScriptCore

- Check out JavaScriptCore / WTF from webkit.org
$ svn co https://svn.webkit.org/repository/webkit/tags/Safari-538.12.1/Source/WTF
$ svn co https://svn.webkit.org/repository/webkit/tags/Safari-538.12.1/Source/JavaScriptCore

- Open WTF.xcodeproj
- Select "WTF" project, go to "Info" tab
- Under "Configurations": for each config of "WTF" (Debug, Release, ...) change the config file to "WTF-iOS-Static"



- Add JavaScriptCore-iOS-Static.xcconfig and ToolExecutable-iOS-Static.xcconfig to JavaScriptCore

- Duplicate target JSCLLIntOffsetsExtractor
- Rename to "JSCLLIntOffsetsExtractor iOS"
- Rename scheme to "JSCLLIntOffsetsExtractor iOS"
- Change the PRODUCT_NAME setting back to "JSCLLIntOffsetsExtractor"
- Select "JavaScriptCore" project, go to "Info" tab
- Under "Configurations": for each config (Debug, Release, ...) change the config file to "ToolExecutable-iOS-Static"
- Close Xcode
- Open project.pbxproj in an editor
- In the "JSCLLIntOffsetsExtractor iOS" target, replace "com.apple.product-type.tool" with "com.apple.product-type.library.static"

- Duplicate target "Derived Sources"
- Rename to "Derived Sources iOS"
- Rename scheme to "Derived Sources iOS"
- Under "Build Phases" > "Target Dependencies":
	- Remove "JSCLLIntOffsetsExtractor"
	- Add "JSCLLIntOffsetsExtractor iOS"
- In the "Generate Derived Sources" script, change last line to:
	/usr/bin/env ruby JavaScriptCore/offlineasm/asm.rb JavaScriptCore/llint/LowLevelInterpreter.asm ${BUILT_PRODUCTS_DIR}/libJSCLLIntOffsetsExtractor.a LLIntAssembly.h || exit 1


- Duplicate target JavaScriptCore
- Rename to "JavaScriptCore iOS"
- Rename the scheme to "JavaScriptCore iOS"
- Remove "JavaScriptCore-copy-Info.plist" from project
- Remove build settings INFOPLIST_FILE, PRODUCT_NAME and INSTALL_PATH from the new target
- Under "Build Phases" > "Target Dependencies":
	- Remove "llmvmForJSC"
	- Remove "Derived Sources"
	- Add "Derived Sources iOS"
- Select "JavaScriptCore" project, go to "Info" tab
- Under "Configurations": for each config (Debug, Release, ...) change the config file to "JavaScriptCore-iOS-Static"
- Close Xcode
- Open project.pbxproj in an editor
- In the "JavaScriptCore iOS" target, replace "com.apple.product-type.framework" with "com.apple.product-type.library.static"

- Try building the projects from the JavaScriptCore.xcworkspace. There are probably some things that don't compile right away, because of small bugs, probably related to feature #defines. See phoboslabs' patches: https://github.com/phoboslab/JavaScriptCore-iOS/commits/master

- Run `python make.py`

- Grab a cup of coffee while the project builds.... :)

Thoughts for improvement:
---
- It's probably simpler and faster not to make duplicates, but just set the xcconfig files on the existing targets
- The step to mod the script for "Generate Derived Sources" can probably be avoided by mucking with the build settings EXECUTABLE_PREFIX (`lib`) and EXECUTABLE_EXTENSION (`a`) to make the output file name match what the script expects.
- Can the product type be set from an xcconfig file? Perhaps using the PRODUCT_TYPE and/or PACKAGE_TYPE build settings?
- For some reason the WTF headers get included in the JavaScriptCore.framework. Probably something I'm missing.
