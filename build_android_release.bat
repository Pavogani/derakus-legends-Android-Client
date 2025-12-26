@echo off
setlocal

echo ========================================
echo  Building Android Release
echo ========================================

REM Step 1: Rebuild data.zip
echo.
echo [1/2] Rebuilding data.zip...
cd /d "%~dp0"

REM Remove old data.zip
if exist "android\app\src\main\assets\data.zip" del "android\app\src\main\assets\data.zip"

REM Create new data.zip from data folder
powershell -Command "Compress-Archive -Path 'data\*' -DestinationPath 'android\app\src\main\assets\data.zip' -Force"

if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to create data.zip
    exit /b 1
)
echo data.zip created successfully!

REM Step 2: Build release APK
echo.
echo [2/2] Building release APK...
cd android

call gradlew.bat assembleRelease

if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build release APK
    exit /b 1
)

echo.
echo ========================================
echo  Build Complete!
echo ========================================
echo APK location: android\app\build\outputs\apk\release\
echo.

endlocal
