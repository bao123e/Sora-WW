@echo off
cd /d "%~dp0"
set "SORA_GAME_PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if exist "%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe" set "SORA_GAME_PS=%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe"
"%SORA_GAME_PS%" -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Read-Gameplay.ps1"
echo.
pause
