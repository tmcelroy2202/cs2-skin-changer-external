@echo off
powershell.exe -ExecutionPolicy Bypass -NoProfile -File "%~dp0scripts\setup.ps1" %*
if %ERRORLEVEL% neq 0 (
    echo.
    echo Setup failed. See messages above.
    pause
)
