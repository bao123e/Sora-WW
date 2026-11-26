@echo off
cd /d "%~dp0"
start "" /wait "%~dp0WuWa-PC.exe" --self-test
set "WUWA_TEST_RESULT=%errorlevel%"
echo Windows self-test result: %WUWA_TEST_RESULT%
echo 0 = PASS. This checks Windows controls and a temporary settings file only.
pause
exit /b %WUWA_TEST_RESULT%
