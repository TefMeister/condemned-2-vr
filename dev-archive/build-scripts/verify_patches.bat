@echo off
setlocal
set LOGDIR=C:\Users\Tefa\AppData\Local\Temp\claude\D--Program-Files--x86--Steam-steamapps-common\433c5e9a-8719-4657-add8-0d486cd69aab\scratchpad
set GAME=E:\condemned-2-vr\src\Condemned2Recomp
set SDK=E:\condemned-2-vr\src\rexglue-sdk

call "D:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat" > "%LOGDIR%\vcv3.log" 2>&1
set "PATH=C:\Program Files\CMake\bin;C:\Users\Tefa\AppData\Local\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe;%PATH%"

cd /d "%GAME%"
if exist out\build\win-amd64-release rmdir /s /q out\build\win-amd64-release

echo ### CONFIGURE - NO manual -march, NO manual -I. Only the two patches + the MSVC target.
cmake --preset win-amd64-release -DREXSDK_DIR=%SDK% -DCMAKE_C_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_CXX_COMPILER_TARGET=x86_64-pc-windows-msvc > "%LOGDIR%\vcfg.txt" 2>&1
if errorlevel 1 ( echo CONFIGURE FAILED & powershell -NoProfile -Command "Select-String -Path '%LOGDIR%\vcfg.txt' -Pattern 'rror' | Select-Object -First 10 | ForEach-Object { $_.Line }" & exit /b 1 )
echo CONFIGURE OK

echo.
echo ### codegen
cmake --build out\build\win-amd64-release --target condemned2recomp_codegen > "%LOGDIR%\vcodegen.txt" 2>&1
echo codegen errorlevel: %errorlevel%

echo.
echo ### build all
cmake --build out\build\win-amd64-release > "%LOGDIR%\vbuild.txt" 2>&1
echo build errorlevel: %errorlevel%
powershell -NoProfile -Command "Select-String -Path '%LOGDIR%\vbuild.txt' -Pattern 'error:|error [A-Z]+[0-9]+|FAILED' | Select-Object -First 8 | ForEach-Object { $_.Line }"
powershell -NoProfile -Command "Get-Content '%LOGDIR%\vbuild.txt' -Tail 3"

echo.
echo ### PRODUCED:
dir /b "%GAME%\out\build\win-amd64-release\*.exe" 2>nul
echo ### FINISHED
