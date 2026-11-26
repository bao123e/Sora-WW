@echo off
cd /d "%~dp0"
set "WUWA_PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if exist "%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe" set "WUWA_PS=%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe"
"%WUWA_PS%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-Runtime.ps1"
echo.
pause
