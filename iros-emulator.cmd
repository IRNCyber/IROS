@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0iros-emulator.ps1" %*
endlocal

