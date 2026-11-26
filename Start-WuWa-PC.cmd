@echo off
cd /d "%~dp0"
if not exist "%~dp0WuWa-PC.exe" goto missing
start "" /wait "%~dp0WuWa-PC.exe" --self-test
if errorlevel 1 goto failed
start "" "%~dp0WuWa-PC.exe"
exit /b 0
:missing
echo WuWa-PC.exe is missing. Extract the whole ZIP into one folder.
pause
exit /b 1
:failed
echo Windows self-test failed. The menu was not started. No game action was sent.
echo Run Check-Windows.cmd and send the displayed result code.
pause
exit /b 1
