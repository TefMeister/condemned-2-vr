@echo off
setlocal
set LOGDIR=C:\Users\Tefa\AppData\Local\Temp\claude\D--Program-Files--x86--Steam-steamapps-common\433c5e9a-8719-4657-add8-0d486cd69aab\scratchpad
set GAME=E:\condemned-2-vr\src\Condemned2Recomp
set SDK=E:\condemned-2-vr\src\rexglue-sdk

if not exist "%GAME%\assets\default.xex" ( echo MISSING default.xex & exit /b 1 )
echo assets OK

call "D:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat" > "%LOGDIR%\vcvars2.log" 2>&1
if errorlevel 1 ( echo VCVARS FAILED & exit /b 1 )
set "PATH=C:\Program Files\CMake\bin;C:\Users\Tefa\AppData\Local\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe;%PATH%"

cd /d "%GAME%"
if exist out\build\win-amd64-release rmdir /s /q out\build\win-amd64-release

echo.
echo ### CONFIGURE (clang -^> MSVC ABI, -march=x86-64-v2 so SSSE3/SSE4.2 are allowed but AVX2 is not)
set "TP=%SDK%\thirdparty"
set "INCS=-I%TP%/imgui -I%TP%/fmt/include -I%TP%/simde -I%TP%/simde/simde -I%TP%/spdlog/include -I%TP%/renderdoc -I%TP%/sdl3/include -I%TP%/dxc/include -I%TP%/tomlplusplus/include"
cmake --preset win-amd64-release -DREXSDK_DIR=%SDK% -DCMAKE_C_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_CXX_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_C_FLAGS="-march=x86-64-v2 %INCS%" -DCMAKE_CXX_FLAGS="-march=x86-64-v2 %INCS%" > "%LOGDIR%\gcfg.txt" 2>&1
if errorlevel 1 (
  echo ### CONFIGURE FAILED
  powershell -NoProfile -Command "Select-String -Path '%LOGDIR%\gcfg.txt' -Pattern 'error|Error' | Select-Object -First 15 | ForEach-Object { $_.Line }"
  exit /b 1
)
echo ### CONFIGURE OK

echo.
echo ### STEP 1/2: codegen - translate default.xex into C++ (the long one)
cmake --build out\build\win-amd64-release --target condemned2recomp_codegen > "%LOGDIR%\gcodegen.txt" 2>&1
echo codegen errorlevel: %errorlevel%
powershell -NoProfile -Command "Select-String -Path '%LOGDIR%\gcodegen.txt' -Pattern 'error:|error [A-Z]+[0-9]+|FAILED' | Select-Object -First 8 | ForEach-Object { $_.Line }"
powershell -NoProfile -Command "Get-Content '%LOGDIR%\gcodegen.txt' -Tail 5"

echo.
echo ### STEP 2/2: build everything
cmake --build out\build\win-amd64-release > "%LOGDIR%\gbuild.txt" 2>&1
echo build errorlevel: %errorlevel%
powershell -NoProfile -Command "Select-String -Path '%LOGDIR%\gbuild.txt' -Pattern 'error:|error [A-Z]+[0-9]+|FAILED' | Select-Object -First 8 | ForEach-Object { $_.Line }"
powershell -NoProfile -Command "Get-Content '%LOGDIR%\gbuild.txt' -Tail 4"

echo.
echo ### PRODUCED:
dir /s /b "%GAME%\out\*.exe" 2>nul | findstr /i condemned
echo ### FINISHED
