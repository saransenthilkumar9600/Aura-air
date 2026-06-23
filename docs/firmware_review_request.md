Subject: Firmware Review Request - v1.19 LED & Hotfix Branch Changes

Hi,

Please review the firmware changes made on the `v1.19_led_and_hotfix` branch. The changes address several bugs found in production devices, including EEPROM read failures, stuck PM2.5 sensor readings, and cloud variable registration issues.

I've attached the ZPH02 sensor datasheet and the Particle P1 datasheet for reference. Please verify the implementation against both datasheets, specifically:

- **ZPH02 datasheet**: Verify the serial protocol implementation (baud rate, frame format, byte positions, checksum calculation) matches the datasheet. Confirm the PM2.5 value calculation formula `(byte[3] + 0.01 * byte[4]) / 0.1` is correct per the datasheet. Check if a warmup period is required and if we need to handle it.

- **Particle P1 datasheet**: Verify that the EEPROM address ranges used (0-901 for JSON data, 1000-1009 for recovery counters, 2044 for reinit flag) are within the P1's 2048-byte EEPROM. Confirm that `Particle.variable()` must be registered in `setup()` before the cloud handshake when using `SYSTEM_THREAD(ENABLED)`. Confirm the `Particle.publish` data size limit (622 bytes).

Below is a summary table of all changes made. Please inspect each one for correctness and potential regressions.

---

## Summary of Changes

| # | File | Change | Reason |
|---|------|--------|--------|
| 1 | `EepromMngr.cpp` | `char schedNum[6]` changed to `char schedNum[8]` (4 locations in `initEeprom()`) | Buffer overflow — `"sched1\0"` needs 7 bytes, old buffer was only 6 |
| 2 | `EepromMngr.cpp` | Removed `panic-reset` and `user-reset` JSON sections from both `#if` and `#else` branches of `initEeprom()` | Serialized JSON (~691 bytes) exceeded Particle.publish 622-byte data limit |
| 3 | `EepromMngr.h` | Added dedicated EEPROM addresses: `EEPROM_PANIC_COUNTER_ADDRS` (1000), `EEPROM_PANIC_LAST_EVT_ADDRS` (1001), `EEPROM_RESET_COUNTER_ADDRS` (1005), `EEPROM_RESET_LAST_EVT_ADDRS` (1006) | Recovery counters moved out of JSON document to dedicated raw EEPROM addresses |
| 4 | `RecoverMngr.cpp` | Replaced `EepromMngr::get/set("panic-reset",...)` and `EepromMngr::get/set("user-reset",...)` with raw `EEPROM.read/write/get/put` at the dedicated addresses | Recovery counters no longer stored in the JSON to reduce serialized size |
| 5 | `EepromMngr.h` | `EEPROM_REINIT_FLAG_ADDRS` changed from `2043` to `2044` | Force EEPROM reinitialization on firmware upgrade — old flag at 2043 was already set by previous firmware, so the new structure was never applied |
| 6 | `EepromMngr.cpp` | Added `EepromMngr::initEeprom()` call inside `clearEeprom()` after `EEPROM.clear()` | EEPROM structure is now rebuilt immediately after clear without requiring a device reboot |
| 7 | `ConnectivityMngr.h` | Added `void registerCloudVariables()` as a public method | Expose cloud variable registration so it can be called early during setup |
| 8 | `ConnectivityMngr.cpp` | Moved `Particle.variable("SSID",...)` and `Particle.variable("EEPROMContent",...)` from `connectivityProcedure()` into new `registerCloudVariables()` method | Cloud variables must be registered before cloud handshake completes when using `SYSTEM_THREAD(ENABLED)` |
| 9 | `SysMngr.h` | Added `void registerCloudVariables()` as a public method | Pass-through to ConnectivityMngr for calling from main setup |
| 10 | `SysMngr.cpp` | Added `SysMngr::registerCloudVariables()` implementation calling `this->connMngr.registerCloudVariables()` | Pass-through to ConnectivityMngr |
| 11 | `main.cpp` | Added `mngr.registerCloudVariables()` in `setup()` after other `Particle.variable` calls | Register cloud variables during setup before the cloud connection is established |
| 12 | `ZPH02.h` | Replaced `counter` member with `packetStartMs` (`unsigned long`). Added `ZPH02_PACKET_TIMEOUT_MS 200` define | Non-blocking millis()-based timeout replaces broken poll counter |
| 13 | `ZPH02.cpp` | Replaced blocking `while(index < 9)` + `counter > 5` loop with non-blocking `while(Serial1.available())` drain. Added millis()-based stale-packet timeout. State and index now reset after every completed packet | Bug 1: poll counter fired in ~167ns at 120MHz, 6000x faster than UART bytes arrive at 9600 baud — packets could never complete. Bug 2: `state` persisted across calls but `index` was reset to 0, permanently desynchronizing the state machine |
| 14 | `ZPH02.cpp` | Removed `random(1, random(8))` noise injection on PM2.5 and `random(0, 7)` noise on PM10 | Fake random noise corrupted real sensor readings. Combined with bugs 1+2, the initial noisy value (typically 5) persisted indefinitely. Confirmed by data from 11 devices: 10 devices showed pm2.5=5 for 99.9% of all readings |
| 15 | `AqiAnalyzer.h` | `PUBLISH_DATA_INTERVAL` changed from `180000` (3 minutes) to `10000` (10 seconds) | Faster publish rate for testing and validation of sensor fixes |

---

## Key Areas to Verify

1. **EEPROM address layout**: Confirm no overlap between the JSON data blob (addresses 2-901), recovery counters (1000-1009), and reinit flag (2044).

2. **ZPH02 non-blocking state machine**: Verify that `while(Serial1.available())` correctly drains the 64-byte RX FIFO without missing bytes or blocking the app thread. Confirm the 200ms stale-packet timeout is appropriate for the ZPH02's packet rate.

3. **PM2.5 calculation**: Verify `round((byte[3] + 0.01 * byte[4]) / 0.1)` against the ZPH02 datasheet. Confirm the checksum algorithm (two's complement of sum of bytes 1-7 compared to byte 8).

4. **EEPROM migration path**: On OTA update from old firmware, the reinit flag at address 2044 will be 255 (unwritten), triggering `saveData()` then `initEeprom()`. Verify that scheduler data, active mode, LED settings, and timezone are preserved correctly through this migration. Verify that the old `panic-reset` and `user-reset` JSON keys in the old EEPROM do not cause issues during `saveData()`.

5. **Publish rate**: The 10-second publish interval is for testing only. Confirm this does not hit Particle cloud rate limits (1 publish/second with burst of 4). This should be reverted to production interval after validation.

Please let me know if you have any questions or need additional context.

Best regards
