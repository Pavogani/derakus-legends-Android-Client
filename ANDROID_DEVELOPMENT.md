# OTClient Android Development Documentation

**Project**: Derakus Legends Android Client
**Repository**: https://github.com/Pavogani/derakus-legends-Android-Client
**Last Updated**: December 25, 2025

---

## Table of Contents

1. [Build Status](#build-status)
2. [Quick Start](#quick-start)
3. [Build Configuration](#build-configuration)
4. [Fixes Applied](#fixes-applied)
5. [Mobile UI Improvements](#mobile-ui-improvements)
6. [Troubleshooting](#troubleshooting)
7. [Future Improvements](#future-improvements)

---

## Build Status

**Current Status**: Working - Joystick functional, login working

### Completed Features
- App launches and runs at 60 FPS
- Joystick control for movement (fixed position mode)
- Login screen with simplified mobile layout
- Asset extraction with progress dialog
- Splash screen on launch
- Long press on action bar buttons to assign spells/items/hotkeys

### Known Issues
- None currently - testing chat keyboard fix

---

## Quick Start

### Prerequisites
- Android SDK with NDK 29
- Gradle 8.x
- ADB for device deployment
- Device with USB debugging enabled

### Build Commands

```cmd
# Navigate to project
cd c:\Users\epols\otclient

# Rebuild data.zip (if data folder changed)
powershell -ExecutionPolicy Bypass -File android\rebuild_data_zip.ps1

# Build Debug APK
cd android
.\gradlew.bat assembleDebug

# Install to connected device
adb install -r app\build\outputs\apk\debug\app-debug.apk

# View logs
adb logcat -s OTClientMobile:V AndroidRuntime:E
```

### First Launch
1. App shows splash screen (~1 second)
2. "First Time Setup" dialog with progress bar (~30-60 seconds)
3. Login screen appears
4. Select server from dropdown, enter credentials

---

## Build Configuration

### Key Files
- `android/app/build.gradle.kts` - Android build config
- `android/app/src/main/AndroidManifest.xml` - App manifest
- `CMakeLists.txt` - Native build config
- `src/framework/platform/androidmanager.cpp` - Android JNI bridge

### Build Settings
```kotlin
// build.gradle.kts
android {
    compileSdk = 35
    ndkVersion = "29.0.13599879 rc2"

    defaultConfig {
        minSdk = 21
        targetSdk = 35

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a")
        }

        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++23", "-flto")
                arguments += listOf("-DANDROID_STL=c++_shared")
            }
        }
    }
}
```

### Data Asset
- `android/app/src/main/assets/data.zip` - Game data (~620MB)
- Contains: Sprites, modules, configurations for protocols 7.72, 8.60, 10.98, 12.64, 14.12

---

## Fixes Applied

### 1. Path Separator Fix (unzipper.cpp)
**Issue**: Files extracted with Windows backslashes failed on Android
**Fix**: Normalize `\` to `/` during extraction
```cpp
for (size_t j = 0; filename[j] != '\0'; ++j) {
    if (filename[j] == '\\') filename[j] = '/';
}
```

### 2. Joystick Fixed Mode
**Issue**: Joystick not showing in game
**Fix**:
- Retry positioning when parent dimensions are 0
- Hide touchZone in fixed mode (was blocking console)
- Connect touch events directly to keypad widget

### 3. Mobile Login Screen
**Issue**: Login screen cluttered with technical fields
**Fix**:
- Hidden server/port/version fields (auto-populated from server selector)
- Larger touch targets for mobile (`$mobile:` styles)
- Simplified layout

### 4. Streaming Asset Extraction
**Issue**: 422MB malloc failing on low-memory devices
**Fix**: Stream extraction in 8MB chunks instead of loading entire file

### 5. Large Heap Enabled
```xml
<application android:largeHeap="true" ...>
```

---

## Mobile UI Improvements

### OTUI Mobile Styles
Use `$mobile:` selector for platform-specific styles:

```yaml
TextEdit
  id: consoleTextEdit
  height: 18
  $mobile:
    height: 36
    margin-bottom: 6
```

### Joystick Module
**File**: `data/modules/game_joystick/`

Features:
- Fixed position mode (bottom-left corner)
- 8-directional movement
- Visual direction indicators
- Haptic feedback support
- Configurable size, opacity, dead zone

Configuration in `joystick.lua`:
```lua
local settings = {
    opacity = 0.8,
    size = 150,
    deadZone = 0.15,
    sensitivity = 1.0,
    leftHanded = false,
    hapticEnabled = true,
    floatingMode = false  -- Fixed mode for mobile
}
```

---

## Troubleshooting

### Black Screen on Launch
1. Check logcat for errors: `adb logcat -s OTClientMobile:V`
2. Verify data.zip extracted: `adb shell ls /data/data/com.derakus.legends/files/game_data/`
3. Clear app data and retry: `adb shell pm clear com.derakus.legends`

### Touch Not Working
1. Check widget `phantom` property (should be `false` for interactive widgets)
2. Check z-order with `:raise()` calls
3. Verify widget is visible and positioned correctly

### Keyboard Not Appearing
1. Widget must be `focusable: true`
2. Widget must be `editable: true` (for TextEdit)
3. No overlay blocking touch events

### Build Errors
```cmd
# Clean build
cd android
.\gradlew.bat clean
.\gradlew.bat assembleDebug
```

### Device Connection
```cmd
# Check device
adb devices

# Restart ADB
adb kill-server
adb start-server
```

---

## Future Improvements

### High Priority
1. **Chat Keyboard Fix** - Ensure console TextEdit receives touch events
2. **Hotkey Bar Long Press** - Allow setting hotkeys via long press
3. **Texture Compression** - Use ETC2 for GPU memory savings

### Medium Priority
1. **FPS Limiting** - Cap at 30 FPS for battery savings
2. **Network Optimization** - Mobile-friendly keep-alive settings
3. **Clipboard Support** - Implement copy/paste via Android ClipboardManager

### Low Priority
1. **On-Demand Asset Loading** - Download protocol versions as needed
2. **Crash Reporting** - Firebase Crashlytics integration
3. **Analytics** - Usage tracking

---

## File Structure

```
otclient/
├── android/
│   ├── app/
│   │   ├── src/main/
│   │   │   ├── assets/data.zip      # Game data
│   │   │   ├── java/.../MainActivity.kt
│   │   │   ├── res/values/themes.xml
│   │   │   └── AndroidManifest.xml
│   │   └── build.gradle.kts
│   ├── rebuild_data_zip.ps1
│   └── gradlew.bat
├── data/
│   ├── modules/
│   │   ├── game_joystick/           # Mobile joystick
│   │   ├── game_console/            # Chat console
│   │   └── client_entergame/        # Login screen
│   └── init.lua                     # Server configuration
├── src/
│   └── framework/
│       └── platform/
│           ├── androidmanager.cpp   # JNI bridge
│           └── androidwindow.cpp    # EGL/OpenGL
└── CMakeLists.txt
```

---

## Commands Reference

```cmd
# Build
cd android && .\gradlew.bat assembleDebug

# Install
adb install -r android\app\build\outputs\apk\debug\app-debug.apk

# Logs
adb logcat -s OTClientMobile:V

# Clear data
adb shell pm clear com.derakus.legends

# Launch
adb shell am start -n com.derakus.legends/.MainActivity

# Force stop
adb shell am force-stop com.derakus.legends

# Check extracted files
adb shell ls /data/data/com.derakus.legends/files/game_data/
```

---

## Contributors

- **Deraku** - Original OTClient Android port
- **Claude Code** - Mobile UI improvements and bug fixes

---

*This document consolidates previous development notes from December 23-25, 2025.*
