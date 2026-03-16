@echo off
chcp 65001 >nul
echo ============================================================
echo    Future Tech Box 2.0 - Build and Upload Script
echo ============================================================
echo.

cd /d "%~dp0led_breathing_project"

echo [Step 1/3] Checking dependencies...
echo.

echo [Step 2/3] Compiling...
echo This may take 1-5 minutes on first run (downloading Arduino framework)
echo.
pio run
if errorlevel 1 (
    echo.
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)

echo.
echo [Step 3/3] Uploading to board...
echo.
pio run -t upload
if errorlevel 1 (
    echo.
    echo [ERROR] Upload failed!
    echo Please check:
    echo   1. USB cable is connected properly
    echo   2. Try holding BOOT button while uploading
    pause
    exit /b 1
)

echo.
echo ============================================================
echo    SUCCESS! Program uploaded to board.
echo ============================================================
echo.
echo Press any key to open serial monitor (optional)...
pause >nul
pio device monitor
