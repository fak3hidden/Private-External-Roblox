@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
title UCR Launcher

REM =============================================================
REM  UCR Launcher - run the external overlay or inject the internal
REM  DLL. Double-click me. Missing binaries are built automatically
REM  if Visual Studio / VS Build Tools is installed.
REM =============================================================

set "EXT_PROJ=undetected-external-main\UCRobloxExternal\UCRobloxExternal\UCRobloxExternal.vcxproj"
set "INT_SLN=internal\Internal.sln"

:menu
echo.
echo ============================================================
echo   UCR Launcher
echo ============================================================
echo   1. EXTERNAL  overlay: ESP, aimbot, movement, etc.
echo   2. INTERNAL  inject UCRCore.dll into a running Roblox
echo   3. UPDATE    pull the latest code and replace files
echo   4. QUIT
echo.
set "choice="
set /p "choice=Pick 1-4: "

if "%choice%"=="1" goto external
if "%choice%"=="2" goto internal
if "%choice%"=="3" goto update
if "%choice%"=="4" goto end
echo [!] Invalid option.
goto menu


REM ------------------- EXTERNAL --------------------------------
:external
set "EXE="
for %%P in (
  "undetected-external-main\UCRobloxExternal\UCRobloxExternal\bin\x64\Release\UCRobloxExternal.exe"
  "undetected-external-main\UCRobloxExternal\UCRobloxExternal\bin\x64\Debug\UCRobloxExternal.exe"
) do if not defined EXE if exist %%P set "EXE=%%~fP"

if not defined EXE (
    echo [!] External not built yet - building now.
    call :build "%EXT_PROJ%"
    for %%P in (
      "undetected-external-main\UCRobloxExternal\UCRobloxExternal\bin\x64\Release\UCRobloxExternal.exe"
      "undetected-external-main\UCRobloxExternal\UCRobloxExternal\bin\x64\Debug\UCRobloxExternal.exe"
    ) do if not defined EXE if exist %%P set "EXE=%%~fP"
)

if not defined EXE (
    echo [ERR] Could not build the external. Open the project in Visual Studio and build Release x64.
    goto menu
)

echo [+] Launching EXTERNAL...
"!EXE!"
echo [*] External closed.
goto menu


REM ------------------- INTERNAL --------------------------------
:internal
call :scan_internal
if defined INJ if defined DLL goto internal_run

echo [!] Internal binaries not built yet - building now.
call :build "%INT_SLN%"
call :scan_internal

if not defined INJ (
    echo [ERR] Injector.exe not found after build. Open internal\Internal.sln and build Release x64.
    goto menu
)
if not defined DLL (
    echo [ERR] UCRCore.dll not found after build. Open internal\Internal.sln and build Release x64.
    goto menu
)

:internal_run
echo [+] Launching INTERNAL injector...
echo     Make sure Roblox is already running first.
"!INJ!" "!DLL!"
echo [*] Injector finished - read the output above.
goto menu


REM ------------------- UPDATE ---------------------------------
:update
if exist "update.cmd" (
    call "update.cmd"
) else (
    echo [*] update.cmd not found - running git pull instead.
    git pull
)
goto menu


REM ------------------- HELPERS --------------------------------
:scan_internal
set "INJ="
for %%P in (
  "internal\Injector\bin\x64\Release\Injector.exe"
  "internal\Injector\bin\x64\Debug\Injector.exe"
) do if not defined INJ if exist %%P set "INJ=%%~fP"

set "DLL="
for %%P in (
  "internal\Core\bin\x64\Release\UCRCore.dll"
  "internal\Core\bin\x64\Debug\UCRCore.dll"
) do if not defined DLL if exist %%P set "DLL=%%~fP"
goto :eof


:build
set "PROJ=%~1"
set "MSBUILD="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "delims=" %%I in ('"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe 2^>nul') do if not defined MSBUILD set "MSBUILD=%%I"
)
if not defined MSBUILD (
    echo [ERR] MSBuild not found - install Visual Studio with the Desktop C++ workload, or VS Build Tools.
    goto :eof
)
echo [*] Building "%PROJ%" - Release x64...
"!MSBUILD!" "%PROJ%" /p:Configuration=Release /p:Platform=x64 /m /nologo /v:minimal
if errorlevel 1 echo [ERR] Build failed - see the errors above.
goto :eof


REM ------------------- EXIT -----------------------------------
:end
echo Bye.
endlocal
