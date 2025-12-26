# Derakus Legends - Known Issues & TODOs

## Critical Issues

### 1. 14.12 Canary Server Connection (Android Client)
**Status:** Unresolved
**Symptom:** Client shows "Success (no Errors)" popup when attempting to connect to 14.12 Canary server
**Server:** ot.derakusproductions.com:7171 (Canary)
**Current Config:**
```lua
["Derakus Legends 14.12"] = {
    ["host"] = "ot.derakusproductions.com/login.php",
    ["port"] = 80,
    ["protocol"] = 1412,
    ["httpLogin"] = true
},
```
**Investigation Notes:**
- Binary protocol on port 7171 was tried first - connection drops after establishing
- HTTP login on port 80 with login.php was configured - still showing "Success (no Errors)"
- The login.php endpoint is responding correctly (tested with curl)
- RSA keys match between client and server
- Network connectivity confirmed (ports 7171, 80 accessible)

**Possible Causes:**
- MyAAC login.php response format not matching what OTClient expects
- Missing characters/worlds in login response
- JSON parsing issue in httplogin.cpp

**Next Steps:**
- Debug the actual HTTP response from login.php
- Check if account exists in Canary database
- Verify login.php returns proper playdata structure

---

## Server Issues (Ubuntu VM - 192.168.1.186)

### 2. Duplicate Weapon ID 0 Warnings (Server 1098)
**Status:** Unresolved (Low Priority)
**Symptom:** `[Warning - Weapons::registerEvent] Duplicate registered item with id: 0` (8 times)
**Location:** Server startup
**Cause:** Likely TFS core issue or weapon scripts with missing item IDs
**Impact:** Low - server runs normally

### 3. Duplicate Unique ID 1000 (Server 1098)
**Status:** Unresolved (Low Priority)
**Symptom:** Multiple `Duplicate unique id: 1000` warnings
**Location:** Map loading
**Cause:** Map items with duplicate unique IDs
**Fix Required:** Use Remere's Map Editor to fix unique ID conflicts

---

## Resolved Issues (For Reference)

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
**Solution:** Fixed in previous session

### Unzipper Issues (Android) - FIXED
**Problem:** Asset extraction failing on Android
**File:** `src/framework/core/unzipper.cpp`
**Solution:** Fixed path handling for Android assets

### Joystick Not Working (Android) - FIXED
**Problem:** Virtual joystick not responding to touch
**Files Fixed:**
- `data/modules/game_joystick/joystick.lua`
- `data/modules/game_joystick/joystick.otui`

---

## Configuration Reference

### Server Port Mapping
| Server | Login Port | Game Port | Screen Session |
|--------|-----------|-----------|----------------|
| Canary 14.12 | 7171 | 7172 | canary |
| TFS 7.72 | 7271 | 7272 | retro |
| TFS 8.60 | 7371 | 7372 | gold |
| TFS 10.98 | 7471 | 7472 | modern |
| OTBR 12.64 | 7571 | 7572 | legacy |

### SSH Access
```
ssh deraku@192.168.1.186
```

### Database Credentials (Canary)
- Host: 127.0.0.1
- User: deraku
- Database: derakus-legends
- Port: 3306

---

## Future Improvements

1. **Multi-Server Support:** Test all 5 server versions work correctly from Android
2. **HTTP Login Debugging:** Add better error logging for HTTP login failures
3. **Map Editor Fixes:** Use Remere's to fix unique ID duplicates
4. **TFS Core Investigation:** Track down weapon ID 0 warnings source
5. **Account Creation:** Ensure accounts exist in all server databases

---

*Last Updated: December 26, 2025*
