@echo off
setlocal
rem Dev-PC rebuild of Condemned 2 on the shared SDK after the 2026-10-11 E: -> D: move (fresh configure).
set LOGDIR=C:\Users\Tefa\AppData\Local\Temp\claude\D--Program-Files--x86--Steam-steamapps-common\2679f761-c3e6-49f9-a555-ea18de416130\scratchpad
set LLVM=C:\Users\Tefa\AppData\Local\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260616-ucrt-x86_64\bin
set GAME=D:\condemned-2-vr\src\Condemned2Recomp
set SDK=D:\condemned-2-vr\src\rexglue-sdk
set "TP=%SDK%\thirdparty"
set "INCS=-I%TP%/imgui -I%TP%/fmt/include -I%TP%/simde -I%TP%/simde/simde -I%TP%/spdlog/include -I%TP%/renderdoc -I%TP%/sdl3/include -I%TP%/dxc/include -I%TP%/tomlplusplus/include"
call "D:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat" > "%LOGDIR%\vcv3.log" 2>&1
set "PATH=C:\Program Files\CMake\bin;C:\Users\Tefa\AppData\Local\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe;%PATH%"
cd /d "%GAME%"
if exist out\build\win-amd64-release\build.ninja goto configured
cmake --preset win-amd64-release -DREXSDK_DIR=%SDK% -DCMAKE_C_COMPILER=%LLVM%\clang.exe -DCMAKE_CXX_COMPILER=%LLVM%\clang++.exe -DCMAKE_LINKER=%LLVM%\lld-link.exe -DCMAKE_C_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_CXX_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_C_FLAGS="-march=x86-64-v2 %INCS%" -DCMAKE_CXX_FLAGS="-march=x86-64-v2 %INCS%" > "%LOGDIR%\c2-cfg.txt" 2>&1
if errorlevel 1 ( echo C2 CONFIGURE FAILED & powershell -NoProfile -Command "Get-Content '%LOGDIR%\c2-cfg.txt' -Tail 15" & exit /b 1 )
:configured
cmake --build out\build\win-amd64-release --target condemned2recomp_codegen > "%LOGDIR%\c2-codegen.txt" 2>&1
echo C2 codegen errorlevel: %errorlevel%
cmake --build out\build\win-amd64-release > "%LOGDIR%\c2-build.txt" 2>&1
echo C2 build errorlevel: %errorlevel%
powershell -NoProfile -Command "Select-String -Path '%LOGDIR%\c2-build.txt' -Pattern 'error:|FAILED' | Select-Object -First 10 | ForEach-Object { $_.Line }"
powershell -NoProfile -Command "Get-Content '%LOGDIR%\c2-build.txt' -Tail 3"
dir /b out\build\win-amd64-release\*.exe out\build\win-amd64-release\*.dll 2>nul
