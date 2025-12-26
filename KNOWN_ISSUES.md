# Derakus Legends - Project Status & Known Issues

**Project**: Derakus Legends Android Client
**Repository**: https://github.com/Pavogani/derakus-legends-Android-Client
**Last Updated**: December 26, 2025

---

## Table of Contents
1. [Current Status](#current-status)
2. [Critical Issues](#critical-issues)
3. [Server Issues](#server-issues)
4. [Code TODOs](#code-todos)
5. [Future Improvements](#future-improvements)
6. [Resolved Issues](#resolved-issues)
7. [Server Configuration](#server-configuration)

---

## Current Status

### Working Features
- Android client builds and runs at 60 FPS
- All 5 server versions supported (7.72, 8.60, 10.98, 12.64, 14.12)
- Joystick control for movement (fixed position mode)
- Login screen with simplified mobile layout
- Asset extraction with progress dialog
- HTTP login for 14.12 Canary server
- Long press on action bar buttons for spell/item assignment

### Build Requirements
- Android SDK with NDK 29
- Gradle 8.x
- Java 21

---

## Critical Issues

*No critical issues at this time.*

---

## Server Issues (Ubuntu VM - 192.168.1.186)

### Duplicate Weapon ID 0 Warnings (Server 1098)
**Status:** Unresolved (Low Priority)
**Symptom:** `[Warning - Weapons::registerEvent] Duplicate registered item with id: 0` (8 times)
**Location:** Server startup
**Cause:** Likely TFS core issue or weapon scripts with missing item IDs
**Impact:** Low - server runs normally

### Duplicate Unique ID 1000 (Server 1098)
**Status:** Unresolved (Low Priority)
**Symptom:** Multiple `Duplicate unique id: 1000` warnings
**Location:** Map loading
**Cause:** Map items with duplicate unique IDs
**Fix Required:** Use Remere's Map Editor to fix unique ID conflicts

---

## Code TODOs

### High Priority (Client Core)

| File | Line | Description |
|------|------|-------------|
| `src/client/tile.cpp` | 242, 337 | Need refactoring |
| `src/framework/ui/uiwidget.cpp` | 1822 | Prevent setting already set properties |
| `src/framework/luaengine/luaobject.cpp` | 120 | Could be cached for more performance |

### Medium Priority (Features)

| File | Line | Description |
|------|------|-------------|
| `data/modules/gamelib/protocollogin.lua` | 170 | Prompt for 2FA token |
| `data/modules/game_bugreport/bugreport.lua` | 1 | Find another hotkey (Ctrl+Z reserved for undo) |
| `data/modules/game_shortcuts/shortcuts.lua` | 332 | Open spell selection UI |
| `data/modules/game_outfit/outfit.lua` | 714, 830 | Try changing square clipping size |
| `data/modules/game_ruleviolation/ruleviolation.lua` | 142 | Message unique ID for statementId |

### Low Priority (Polish)

| File | Line | Description |
|------|------|-------------|
| `src/client/statictext.cpp` | 57, 119 | Could be moved to Lua |
| `src/client/minimap.cpp` | 308, 383 | Implement compression flag with zlib |
| `src/client/mapio.cpp` | 257, 489 | Map name input, compression flag |
| `src/client/spritemanager.cpp` | 198 | Check for overwritten sprites |
| `src/framework/graphics/image.cpp` | 192 | FIXME: calculate mipmaps for 8x1, 4x1, 2x1 |

### Game Module TODOs

| Module | Description |
|--------|-------------|
| `game_cyclopedia/items` | Access market statistics from game_market module |
| `game_cyclopedia/bestiary` | Add sort by name, kills, percentage |
| `game_containers` | Implement weight sorting, quickloot sorting, nested container moves |
| `game_battle` | Optimize creature updates for large counts |
| `game_analyser` | Fix anchor bugs on login |

### Protocol TODOs

| File | Description |
|------|-------------|
| `protocolgameparse.cpp:1979` | Set creature as trapper |
| `protocolgameparse.cpp:2237` | processEditText with ItemPtr parameter |
| `protocolgameparse.cpp:4477` | Implement game news usage |
| `protocolgameparse.cpp:4563` | Implement items price usage |
| `protocolgameparse.cpp:4587` | Implement bestiary entry changed usage |

---

## Future Improvements

### High Priority
1. **Chat Keyboard Fix** - Ensure console TextEdit receives touch events on all devices
2. **Texture Compression** - Use ETC2 for GPU memory savings
3. **Protocol Refactoring** - Clean up tile.cpp refactoring TODOs

### Medium Priority
1. **FPS Limiting** - Cap at 30 FPS option for battery savings
2. **Network Optimization** - Mobile-friendly keep-alive settings
3. **Clipboard Support** - Implement copy/paste via Android ClipboardManager
4. **2FA Token UI** - Prompt for authenticator token during login
5. **Market Statistics** - Integrate with cyclopedia items

### Low Priority
1. **On-Demand Asset Loading** - Download protocol versions as needed
2. **Crash Reporting** - Firebase Crashlytics integration
3. **Analytics** - Usage tracking
4. **Minimap Compression** - Implement zlib compression for minimap saves

---

## Resolved Issues

### 14.12 Canary Server Connection (Android Client) - FIXED
**Date:** December 26, 2025
**Problem:** Client showed "Success (no Errors)" popup, returned to login screen
**Root Cause:** Password in database didn't match user's expected password (SHA1 hash mismatch)
**Solution:**
1. Configured HTTP login in init.lua (host: `ot.derakusproductions.com/login.php`, port: 80)
2. Updated account password hash in MySQL database

**Config:**
```lua
["Derakus Legends 14.12"] = {
    ["host"] = "ot.derakusproductions.com/login.php",
    ["port"] = 80,
    ["protocol"] = 1412,
    ["httpLogin"] = true
},
```

### First Items Script Error (Server 1098) - FIXED
**Problem:** `getContainer` nil error on line 53
**Root Cause:** Wrong backpack ID (2854 instead of 1988)
**Solution:** Rewrote first_items.lua with correct item IDs:
- Backpack ID: 1988 (not 2854)
- ITEM_BAG: 1987

### sendSupplyUsed Error (Server 1098) - FIXED
**Problem:** `sendSupplyUsed` method doesn't exist in TFS 1.x
**Files Fixed:**
- `/home/deraku/server-1098/data/actions/scripts/others/food.lua`
- `/home/deraku/server-1098/data/actions/scripts/others/enchanting.lua`
**Solution:** Commented out sendSupplyUsed calls

### Missing Monster Files (Server 1098) - FIXED
**Problem:** 31 monsters referenced in map but missing XML definitions
**Solution:** Created XML files and registered in monsters.xml:
- Mountain Troll, Minotaur Bruiser, Minotaur Poacher, Minotaur Occultist
- Juvenile Cyclops, Brittle Skeleton, Crazed Dwarf, Dawn Scorpion
- Troll-trained Salamander, Salamander Trainer, Lesser Fire Devil
- Muglex Clan (Scavenger, Assassin, Footman), Woodling
- Scar Tribe (Warrior, Shaman), Instable Breach Brood, Instable Sparkion
- Breach Brood, Dread Intruder, Reality Reaver, Spark of Destruction
- Charger, Outburst, Greed, Deaththrower, Demon Summoner
- Overcharged Energy Elemental, Trailemaw, Training Monk

### RSA Decrypt Error (Android) - FIXED
**Problem:** RSA decryption failing during login
**Solution:** Fixed in previous development session

### Unzipper Path Separator (Android) - FIXED
**Problem:** Asset extraction failing on Android
**File:** `src/framework/core/unzipper.cpp`
**Solution:** Normalize `\` to `/` during extraction

### Joystick Not Working (Android) - FIXED
**Problem:** Virtual joystick not responding to touch
**Files Fixed:**
- `data/modules/game_joystick/joystick.lua`
- `data/modules/game_joystick/joystick.otui`

### Streaming Asset Extraction (Android) - FIXED
**Problem:** 422MB malloc failing on low-memory devices
**Solution:** Stream extraction in 8MB chunks instead of loading entire file

---

## Server Configuration

### Port Mapping
| Server | Protocol | Login Port | Game Port | Screen Session |
|--------|----------|-----------|-----------|----------------|
| Canary 14.12 | HTTP | 80 (login.php) | 7172 | canary |
| TFS 7.72 | Binary | 7271 | 7272 | retro |
| TFS 8.60 | Binary | 7371 | 7372 | gold |
| TFS 10.98 | Binary | 7471 | 7472 | modern |
| OTBR 12.64 | Binary | 7571 | 7572 | legacy |

### SSH Access
```bash
ssh deraku@192.168.1.186
```

### Database Credentials (Canary)
- Host: 127.0.0.1
- User: deraku
- Database: derakus-legends
- Port: 3306

### Screen Commands
```bash
# List screens
screen -ls

# Attach to server
screen -r canary

# Detach
Ctrl+A, D
```

---

## Quick Reference

### Build Android APK
```cmd
cd c:\Users\epols\otclient\android
.\gradlew.bat assembleDebug
adb install -r app\build\outputs\apk\debug\app-debug.apk
```

### Rebuild data.zip
```cmd
powershell -ExecutionPolicy Bypass -File android\rebuild_data_zip.ps1
```

### View Logs
```cmd
adb logcat -s OTClientMobile:V AndroidRuntime:E
```

---

*This document consolidates all project issues, TODOs, and status information.*
