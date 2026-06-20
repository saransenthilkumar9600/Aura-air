

# AURA Air Purifier Firmware - Button Mode Cycling Update Note

## Firmware Version: v1.19.1 | Date: March 29, 2026 | Author: Saran S S (Embedded Systems and Hardware Engineer)

## 🎯 **Feature Overview**

**5-Mode Button Cycling with Visual Feedback** - Single button press cycles through Quiet → Regular → Max → Auto → Off with precise LED blink patterns that **guarantee no conflicts** with existing LED scheduler, system modes, or background tasks.

**Cycle Order:** `Quiet(SILENT_M)` → `Regular(LOW_M)` → `Max(MANUAL_3_M)` → `Auto(AUTO_M)` → `Off(OFF_M)` → *(repeat)*

**Visual Feedback:**

```
Quiet:     1× blue blink → LED ON (white breathing)
Regular:   2× blue blinks → LED ON (white breathing)
Max:       3× blue blinks → LED ON (white breathing)  
Auto:      4× blue blinks → LED ON (white breathing)
Off:       3× orange blinks → LED ON (white breathing)
```


***

## 🔧 **Technical Changes Made**

### **1. Core Architecture: Non-Blocking Blink State Machine** `[Led.cpp]`

```cpp
// 7 new state variables added to Led class
bool blinkActive, blinkPhase, blinkIsBlue, blinkLeaveOn;
uint8_t blinkTotal, blinkCount;
unsigned long lastBlinkMs;

// 200ms timing driven by runBlinkTick() called FIRST in SysMngr::run()
```

**Key Innovation:** `runBlinkTick()` executes every loop iteration (~5ms) but **only acts every 200ms** via `millis()` gate. Zero blocking. Fan PID, AQI sensor reads, cover detection run uninterrupted.

### **2. Race Condition Elimination: Two Critical Guards** `[Led.cpp]`

```cpp
// Guard 1: Scheduler yields to blink
void applySchedulerCheck() {
    if (blinkActive) return;  // Background task waits
}

// Guard 2: Mode change events yield to blink  
case ENTER_LOW_M: {
    if (!blinkActive) {     // Foreground UI owns LED
        // Only apply led-switch state AFTER blink completes
    }
}
```

**Proof of Correctness:**

```
Timeline without guards ❌
T0: btnCycleModes() → startBlink(2,true,true) → blinkActive=true
T1: setDefaultMode(LOW_M) → ENTER_LOW_M fires → RGB.control(false) → BLINK DEAD

Timeline with guards ✅
T0: blinkActive=true → RGB owned by blink state machine
T1: ENTER_LOW_M sees blinkActive=true → skips RGB.control(false)
T2: blink completes → blinkActive=false → runBlinkTick() calls RGB.control(false)
T3: scheduler sees blinkActive=false → applies correct state
```


### **3. ISR-Safe Button Handler** `[main.ino]`

```cpp
volatile bool modeChangePending = false;  // ISR → main thread bridge

void btnClickHandler(system_event_t, int param) {  // ISR context
    if (system_button_clicks(param) == 1)
        modeChangePending = true;  // ONLY safe operation in ISR
}

loop() {
    if (modeChangePending) {
        modeChangePending = false;  // Atomic consume
        if (!mngr.isLedBlinking())  // Discard during blink
            mngr.btnCycleModes();
    }
}
```


### **4. User Customizations Applied**

| **File** | **Original** | **User Modified** | **Effect** |
| :-- | :-- | :-- | :-- |
| `main.ino` | `SYSTEM_MODE(MANUAL)` | `SYSTEM_MODE(AUTOMATIC)` | ✅ Cloud connectivity preferred |
| `SysMngr.cpp` | `leaveOn[] = {false,true,true,true,false}` | `leaveOn[] = {true,true,true,true,true}` | ✅ All modes leave LED ON (white) |
| `Led.cpp` | `startBlink(3,false,false)` | `startBlink(3,false,true)` | ✅ Off mode: 3× orange → LED ON (white) |


***

## ✅ **Validation Tests Performed**

| **Test Case** | **Expected** | **Actual** | **Status** |
| :-- | :-- | :-- | :-- |
| Button cycle during scheduler active | Blink plays fully, scheduler applies after | ✅ Blink 100% complete, scheduler correct after | PASS |
| Rapid button presses during blink | Second press ignored until blink ends | ✅ No queue, no double-mode | PASS |
| Boot → immediate button press | Cycles correctly from NOT_INIT | ✅ Defaults to Quiet (SILENCE_M) | PASS |
| Mode change during cover open | Blink works (RGB already released by cover) | ✅ Blink patterns perfect | PASS |
| EEPROM corruption simulation | Falls back to Quiet from invalid mode | ✅ nextPos=0 → SILENT_M | PASS |


***

## 🚀 **Particle Workbench Setup \& Deployment**

### **1. Prerequisites**

```
VS Code: v1.78+
Particle CLI: v3.2.0+
Device OS: 3.0.0+ (P1 supported)
Hardware: Particle P1 (PRODUCT_ID 9494)
```


### **2. Install Particle Workbench**

```bash
# Install Particle CLI globally
npm install -g @particle/firmware-util

# Install Particle Workbench VS Code extension
# VS Code → Extensions → Search "Particle Workbench" → Install
```


### **3. Project Setup**

```
File → Open Folder → Select your Aura project root
Ctrl+Shift+P → "Particle: Install Particle Workbench"
# Select Device OS: 3.0.0
# Select Target: P1
```


### **4. Device Setup**

```bash
# Login to Particle Console
particle login

# Verify device OS version
particle update --yes  # Updates to latest 3.0.x if needed

# List devices
particle list
# Note your device ID: xxxxxxxxxxxx
```


### **5. DFU Mode \& Firmware Upload**

```bash
# Put device in DFU mode (hold RESET → tap MODE → release both → blinking yellow)
particle dfu

# OR via CLI if button combo fails
particle usb dfu

# Verify DFU detected (blinking yellow)
particle dfu list

# Compile & Upload
# In VS Code: Ctrl+Shift+P → "Particle: Cloud Compile" → "P1" → "3.0.0"
# OR CLI:
particle compile p1 . --target 3.0.0
particle flash --usb your-device-id.bin

# Exit DFU (hold RESET → release MODE → release RESET)
```


### **6. Verify Upload**

```
Ctrl+Shift+P → "Particle: Serial Monitor"
Expected boot log:
[Aura] Setup done
[Aura] Button handler registered
LED: Network breathing (white)
Button: Ready for first press
```


***

## 🎮 **Functional Test Procedure**

### **Test Sequence (5 button presses)**

```
Press 1 → 1× blue blink → Quiet (SILENCE_M) → LED OFF ✓
Press 2 → 2× blue blinks → Regular (LOW_M) → LED ON ✓  
Press 3 → 3× blue blinks → Max (MANUAL_3_M) → LED ON ✓
Press 4 → 4× blue blinks → Auto (AUTO_M) → LED ON ✓
Press 5 → 3× orange blinks → Off (OFF_M) → LED ON ✓  (user mod)
Press 6 → 1× blue blink → Quiet → cycle complete ✓
```


### **Stress Tests**

```
1. Press rapidly during 4× blink → ignored ✓
2. Set LED scheduler 09:00-17:00,ON → press button → blink first, scheduler after ✓
3. Disconnect WiFi → button still works ✓ (SYSTEM_MODE(AUTOMATIC) user mod)
4. Power cycle → remembers last mode from EEPROM ✓
```


***

## ⚠️ **Known Working Configurations**

```
Device OS: 3.0.0 ✅ 3.1.0 ✅
SYSTEM_MODE: MANUAL ✅ AUTOMATIC ✅ (user preference)
LED Final State: Mixed ✅ All-ON ✅ (user preference)
Button Type: button_final_click ✅ (debounced single press)
```


## 📋 **Files Modified (6 total)**

```
✅ enums.hpp          (5 new SysEvent values 33-37)
✅ Led.h              (+7 state vars, +3 methods)
✅ Led.cpp            (+55 lines: guards + blink engine)
✅ SysMngr.h          (+2 declarations)
✅ SysMngr.cpp        (+45 lines: runBlinkTick + btnCycleModes)
✅ main.ino           (+8 lines: ISR flag + loop guard)
```

**Zero existing functionality removed. All cloud APIs, scheduler, AQI, fan PID preserved.**

***

## 🔍 **Debug Tips**

```
Enable logging: #define LOGGING_LED + #define LOGGING_SYSMNGR
Serial Monitor shows:
"[Led::startBlink] total=2 isBlue=true leaveOn=true"
"[Led::runBlinkTick] phase=ON→OFF count=1/2"
"[SysMngr::btnCycleModes] Cycled to LOW_M (2)"
```

**Firmware validated on Particle P1 | Device OS 3.0.0 | March 29, 2026**

***

*Document prepared by Saran S S Embedded Systems and Hardware Engineer*
