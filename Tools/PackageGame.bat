@echo off
rem Windows packaging test. Close the editor first.
rem Usage: PackageGame.bat            (Development build, with logs)
rem        PackageGame.bat Shipping   (release build)
rem Output: Packaged\Windows\CatchTheTail.exe
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Development
set ROOT=%~dp0..
set UAT=C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat

call "%UAT%" BuildCookRun -project="%ROOT%\CatchTheTail.uproject" -noP4 -platform=Win64 -clientconfig=%CONFIG% -build -cook -stage -pak -iostore -package -archive -archivedirectory="%ROOT%\Packaged" -utf8output
if errorlevel 1 goto failed

rem Steam test app id (480 = Spacewar). Replace the number once you get a real AppID.
echo 480> "%ROOT%\Packaged\Windows\steam_appid.txt"
if exist "%ROOT%\Packaged\Windows\CatchTheTail\Binaries\Win64" echo 480> "%ROOT%\Packaged\Windows\CatchTheTail\Binaries\Win64\steam_appid.txt"
echo Done: %ROOT%\Packaged\Windows\CatchTheTail.exe
pause
exit /b 0

:failed
echo Packaging failed. Check the Error lines in the log above.
pause
exit /b 1
