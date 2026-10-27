@echo off
rem Packages BochaNet for distribution on Windows (build + cook + pak + archive).
rem
rem   build-game.bat [Shipping^|Development]
rem
rem Close the Unreal Editor first. The engine path can be overridden with UE_ROOT.
rem Output: Packaged\Windows\ (the folder ready to zip and share).

setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Shipping"
if not defined UE_ROOT set "UE_ROOT=C:\Program Files\Epic Games\UE_5.8"
set "REPO=%~dp0"
if "%REPO:~-1%"=="\" set "REPO=%REPO:~0,-1%"
set "PROJECT=%REPO%\BochaNet\BochaNet.uproject"
set "OUT=%REPO%\Packaged"
set "MAPS=/Game/ThirdPerson/Lvl_ThirdPerson+/Game/Bocha/Maps/Lvl_Cancha"

if /i not "%CONFIG%"=="Shipping" if /i not "%CONFIG%"=="Development" (
	echo Usage: %~nx0 [Shipping^|Development] 1>&2
	exit /b 1
)
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" (
	echo Unreal Engine not found at %UE_ROOT% ^(set UE_ROOT^). 1>&2
	exit /b 1
)
tasklist /fi "imagename eq UnrealEditor.exe" | find /i "UnrealEditor.exe" >nul
if not errorlevel 1 (
	echo Close the Unreal Editor before packaging. 1>&2
	exit /b 1
)

rem The cooker loads the project's editor modules, so they must be up to date.
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" BochaNetEditor Win64 Development ^
	-project="%PROJECT%" -waitmutex
if errorlevel 1 exit /b 1

call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
	-project="%PROJECT%" -platform=Win64 -clientconfig=%CONFIG% ^
	-map=%MAPS% -build -cook -stage -pak -package -archive -archivedirectory="%OUT%" ^
	-prereqs -nocompileeditor -unattended -utf8output -nop4
if errorlevel 1 exit /b 1

echo.
echo Done: %OUT%\Windows
