Pod::Spec.new do |s|
  s.name           = "JavaScriptCore-iOS"
  s.version        = "0.0.1"
  s.summary        = "Apple's JavaScript Engine, with modified project files for iOS."
  s.description    = "Apple's JavaScript Engine, with modified project files for iOS. Also includes JavaScript's Typed Arrays which are normally a part of WebKit, not of JavaScriptCore."
  s.homepage       = "https://github.com/phoboslab/JavaScriptCore-iOS"
  s.authors        = "WebKit Team"
  s.license        = { :type => 'LGPL', :file => 'JavaScriptCore/COPYING.LIB' }
  s.source         = { :git => "https://github.com/phoboslab/JavaScriptCore-iOS.git", :tag => "#{s.version}" }
  s.platform       = :ios, '5.0'
  s.source_files   = 'JavaScriptCore/API/*.h'
  s.header_dir     = 'JavaScriptCore'
  s.preserve_paths = 'Build/libJavaScriptCore.a'
  s.libraries      = 'stdc++', 'icucore', 'JavaScriptCore'
  s.requires_arc   = false
  s.xcconfig       =  { 'LIBRARY_SEARCH_PATHS' => '"$(PODS_ROOT)/JavaScriptCore-iOS/Build"' }


  def s.pre_install(pod, target_definition)
    Dir.chdir(pod.root) do
      system <<CMD
mkdir Build

xcodebuild -project WTF/WTF.xcodeproj -alltargets clean
xcodebuild -project WTF/WTF.xcodeproj -target "WTF iOS" -configuration Release -sdk iphoneos
xcodebuild -project WTF/WTF.xcodeproj -target "WTF iOS" -configuration Release -sdk iphonesimulator -arch i386
xcodebuild -project WTF/WTF.xcodeproj -target "Combine iOS lib" -configuration Release

xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -alltargets clean
xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -target "JavaScriptCore iOS" -configuration Release -sdk iphoneos
xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -target "JavaScriptCore iOS" -configuration Release -sdk iphonesimulator -arch i386
xcodebuild -project JavaScriptCore/JavaScriptCore.xcodeproj -target "Combine iOS lib" -configuration Release

echo "DONE!"
CMD
    end
  end
end

