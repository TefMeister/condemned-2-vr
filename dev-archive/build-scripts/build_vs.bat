@echo off
setlocal
set LOGDIR=C:\Users\Tefa\AppData\Local\Temp\claude\D--Program-Files--x86--Steam-steamapps-common\433c5e9a-8719-4657-add8-0d486cd69aab\scratchpad

if not exist "D:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat" (
  echo VCVARS NOT FOUND
  exit /b 1
)

echo ### Entering the Visual Studio build environment
call "D:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat" > "%LOGDIR%\vcvars.log" 2>&1
if errorlevel 1 ( echo VCVARS FAILED & type "%LOGDIR%\vcvars.log" & exit /b 1 )
echo INCLUDE head: %INCLUDE:~0,70%
echo LIB head:     %LIB:~0,70%

set "PATH=C:\Program Files\CMake\bin;C:\Users\Tefa\AppData\Local\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe;%PATH%"

cd /d E:\condemned-2-vr\src\rexglue-sdk
if exist out\build\win-amd64 rmdir /s /q out\build\win-amd64

echo.
echo ### CONFIGURE (clang -^> MSVC ABI, -march=x86-64-v2)
cmake --preset win-amd64 -DCMAKE_C_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_CXX_COMPILER_TARGET=x86_64-pc-windows-msvc > "%LOGDIR%\cfg.txt" 2>&1
if errorlevel 1 (
  echo ### CONFIGURE FAILED
  powershell -NoProfile -Command "Select-String -Path '%LOGDIR%\cfg.txt' -Pattern 'error|Error' | Select-Object -First 12 | ForEach-Object { $_.Line }"
  exit /b 1
)
echo ### CONFIGURE OK

echo.
echo ### BUILD Release
cmake --build out\build\win-amd64 --config Release > "%LOGDIR%\bld.txt" 2>&1
echo build errorlevel: %errorlevel%
echo --- first failures, if any ---
powershell -NoProfile -Command "Select-String -Path '%LOGDIR%\bld.txt' -Pattern 'error:|error [A-Z]+[0-9]+|FAILED' | Select-Object -First 12 | ForEach-Object { $_.Line }"

echo.
echo ### PRODUCED:
dir /s /b out\build\win-amd64\*.dll out\build\win-amd64\*.exe 2>nul | findstr /v /i "CompilerId"
echo ### FINISHED
