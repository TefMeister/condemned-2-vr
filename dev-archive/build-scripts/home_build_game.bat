@echo off
setlocal
rem Developer PC (PC1) build of Condemned 2 VR (2026-11-12). The tree: D:\condemned-2-build\src\Condemned2Recomp = upstream
rem Condemned2Recomp 0.1.0 + this repo's port-project files (CMakeLists.txt, src\*), its assets\ a junction to the game copy
rem C:\NonSteam\condemned-2\app\Assets. The SDK is The Darkness's (D:\the-darkness-build\rexglue-sdk, v0.10.0 + its patches:
rem the headset calls Condemned 2 uses AND the motion controllers). No wipe: CMake reconfigures itself when needed.
set LOGDIR=D:\condemned-2-build\logs
set GAME=D:\condemned-2-build\src\Condemned2Recomp
set SDK=D:\the-darkness-build\rexglue-sdk
if not exist "%GAME%\assets\default.xex" ( echo MISSING default.xex & exit /b 1 )
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" > "%LOGDIR%\vcvars-game.log" 2>&1
if errorlevel 1 ( echo VCVARS FAILED & exit /b 1 )
set "PATH=C:\Program Files\LLVM\bin;C:\Program Files\CMake\bin;%LOCALAPPDATA%\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe;%PATH%"
cd /d "%GAME%"
set "TP=%SDK%\thirdparty"
set "INCS=-I%TP%/imgui -I%TP%/fmt/include -I%TP%/simde -I%TP%/simde/simde -I%TP%/spdlog/include -I%TP%/renderdoc -I%TP%/sdl3/include -I%TP%/dxc/include -I%TP%/tomlplusplus/include"
if exist out\build\win-amd64-release\build.ninja goto built_cfg
echo ### CONFIGURE
cmake --preset win-amd64-release -DREXSDK_DIR=%SDK% -DCMAKE_C_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_CXX_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_C_FLAGS="-march=x86-64-v2 %INCS%" -DCMAKE_CXX_FLAGS="-march=x86-64-v2 %INCS%" > "%LOGDIR%\game-cfg.txt" 2>&1
if errorlevel 1 ( echo CONFIGURE FAILED & exit /b 1 )
echo CONFIGURE OK
:built_cfg
echo ### CODEGEN
cmake --build out\build\win-amd64-release --target condemned2recomp_codegen > "%LOGDIR%\game-codegen.txt" 2>&1
echo codegen errorlevel: %errorlevel%
echo ### BUILD
cmake --build out\build\win-amd64-release > "%LOGDIR%\game-build.txt" 2>&1
echo build errorlevel: %errorlevel%
dir /b out\build\win-amd64-release\*.exe 2>nul
echo ### FINISHED
