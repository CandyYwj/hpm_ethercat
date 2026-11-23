@echo off
REM Author: BearKing
setlocal

powershell -ExecutionPolicy Bypass -File "%~dp0download_firmware.ps1" %*
exit /b %ERRORLEVEL%
