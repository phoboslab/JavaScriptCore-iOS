all:
    touch "%ConfigurationBuildDir%\buildfailed"
    bash build-LLIntDesiredOffsets.sh "%ConfigurationBuildDir%" "$(WEBKIT_LIBRARIES)" "%PlatformArchitecture%"

    -del "%ConfigurationBuildDir%\buildfailed"

clean:
    -del "%ConfigurationBuildDir%\buildfailed"
    -del /s /q "%ConfigurationBuildDir%\obj%PlatformArchitecture%\JavaScriptCore\DerivedSources\LLIntDesiredOffsets.h"
