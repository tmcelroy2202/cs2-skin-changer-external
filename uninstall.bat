@echo off
powershell.exe -ExecutionPolicy Bypass -NoProfile -File "%~dp0scripts\uninstall.ps1" %*
if %ERRORLEVEL% neq 0 (
    echo.
    pause
)
