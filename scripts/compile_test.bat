@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cl /nologo /TP /DLIBGOSSIP_BUILD /IC:\Users\cmx\repo\libgossip\third_party /IC:\Users\cmx\repo\libgossip\third_party\asio\asio\include /IC:\Users\cmx\repo\libgossip\third_party\json\single_include /IC:\Users\cmx\repo\libgossip\include /IC:\Users\cmx\repo\libgossip\build\msvc-debug\include /DWIN32 /D_WINDOWS /GR /EHsc /Zi /Ob0 /Od /RTC1 -std:c++17 -MDd /utf-8 /W4 /c C:\Users\cmx\repo\libgossip\src\net\serializer_factory.cpp 2>&1
