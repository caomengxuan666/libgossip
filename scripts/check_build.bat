@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
ninja -C C:\Users\cmx\repo\libgossip\build\msvc-debug 2>&1
