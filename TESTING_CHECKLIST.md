# Testing Checklist - BLE Custom Payload Features

Use this checklist to systematically test all implemented features.

## Pre-Testing Setup

- [ ] Firmware compiled successfully without errors
- [ ] Firmware flashed to device
- [ ] SD card formatted (FAT32) and inserted
- [ ] Directory `/BruceSD/ble/payloads/` created on SD card
- [ ] Example payload files copied to SD card
- [ ] Phone with BLE scanner app ready (nRF Connect or LightBlue)
- [ ] Serial monitor connected (115200 baud)

## Test 1: Menu Integration

**Objective:** Verify new menu items appear correctly

- [ ] Power on device
- [ ] Navigate to Bluetooth menu
- [ ] Verify "Scan Advertiser" appears in menu
- [ ] Verify "Custom Payload" appears in menu
- [ ] Verify both items are between "BLE Scan" and "iBeacon"
- [ ] Verify no visual glitches in menu
- [ ] Verify other menu items still accessible

**Result:** ☐ PASS  ☐ FAIL  
**Notes:** _______________________________________________

---

## Test 2: Scan Advertiser - Basic Scan

**Objective:** Test basic BLE scanning functionality

### 2.1 Start Scan
- [ ] Navigate to "Scan Advertiser"
- [ ] Press OK to start scan
- [ ] Verify "Scanning..." message appears
- [ ] Verify countdown timer displays (10s → 0s)
- [ ] Verify no crash during scan
- [ ] Wait for scan completion

### 2.2 View Results
- [ ] Verify device list appears after scan
- [ ] Verify at least one device found (if BLE devices nearby)
- [ ] Verify device names displayed
- [ ] Verify RSSI values shown (negative numbers)
- [ ] Verify "Unknown" shown for devices without name
- [ ] Verify list is scrollable (Up/Down buttons)

### 2.3 Serial Output
- [ ] Check Serial Monitor shows:
  ```
  BLE Device initialized
  Scan started...
  Found device: [name] [address] [rssi]
  ```

**Result:** ☐ PASS  ☐ FAIL  
**Notes:** _______________________________________________

---

## Test 3: Scan Advertiser - Device Details

**Objective:** Test device detail viewing

- [ ] Select a device from list (navigate with Up/Down)
- [ ] Press OK to view details
- [ ] Verify device details screen appears with:
  - [ ] Device name
  - [ ] MAC address (XX:XX:XX:XX:XX:XX format)
  - [ ] RSSI value
  - [ ] Manufacturer ID (if applicable)
  - [ ] Manufacturer data (hex, if applicable)
  - [ ] Service UUIDs (if applicable)
- [ ] Verify "OK:Save ESC:Back" prompt shown
- [ ] Press ESC to go back
- [ ] Verify returns to device list

**Result:** ☐ PASS  ☐ FAIL  
**Notes:** _______________________________________________

---

## Test 4: Scan Advertiser - Save Payload

**Objective:** Test payload saving functionality

### 4.1 Save to SD Card
- [ ] Navigate to a device and view details
- [ ] Press OK to save
- [ ] Verify "Saved!" message appears
- [ ] Check Serial Monitor for save confirmation
- [ ] Remove SD card and check file exists:
  - [ ] File location: `/BruceSD/ble/payloads/[device_name].json`
  - [ ] File is valid JSON
  - [ ] File contains expected fields

### 4.2 Verify JSON Content
Open saved JSON file and verify:
- [ ] "version" field present
- [ ] "name" field contains device name
- [ ] "type" field = "advertisement"
- [ ] "address" field contains MAC
- [ ] "rssi" field contains number
- [ ] "adv_data" object present
- [ ] "raw_adv_data" hex string present

**Result:** ☐ PASS  ☐ FAIL  
**Notes:** _______________________________________________

---

## Test 5: Custom Payload - Load Payload

**Objective:** Test payload file loading

### 5.1 Browse Files
- [ ] Navigate to "Custom Payload"
- [ ] Press OK to open file browser
- [ ] Verify browser opens to correct directory
- [ ] Verify example files visible:
  - [ ] example_airpods.json
  - [ ] example_samsung_watch.json
  - [ ] Any saved payloads from Test 4

### 5.2 Load File
- [ ] Select example_airpods.json
- [ ] Press OK to load
- [ ] Verify "Loading..." message appears
- [ ] Verify "Loaded!" confirmation
- [ ] Verify payload info screen shows:
  - [ ] Name: "AirPods Pro"
  - [ ] Type: "advertisement"
  - [ ] Manufacturer info
  - [ ] Data length

### 5.3 Serial Output
- [ ] Check Serial Monitor shows:
  ```
  Loading payload from: /BruceSD/ble/payloads/example_airpods.json
  Payload loaded successfully
  ```

**Result:** ☐ PASS  ☐ FAIL  
**Notes:** _______________________________________________

---

## Test 6: Custom Payload - Spam Functionality

**Objective:** Test BLE advertisement spamming

### 6.1 Start Spam
- [ ] With payload loaded, press OK to start spam
- [ ] Verify "Spamming..." message appears
- [ ] Verify counter starts incrementing
- [ ] Verify no immediate crash

### 6.2 Phone Detection
- [ ] Open BLE scanner app on phone
- [ ] Refresh/rescan
- [ ] Verify devices appear with:
  - [ ] Name matching payload (e.g., "AirPods Pro")
  - [ ] Multiple instances with different MACs
  - [ ] RSSI values visible
  - [ ] Manufacturer data matches (if viewable in app)

### 6.3 MAC Randomization
- [ ] Observe MAC addresses in phone scanner
- [ ] Verify MAC addresses change
- [ ] Verify pattern like: F0:XX:XX:XX:XX:XX

### 6.4 Stop Spam
- [ ] Press ESC to stop
- [ ] Verify "Stopped. Count: XX" message
- [ ] Verify spam stops (phone no longer sees new devices)
- [ ] Verify no crash on stop

### 6.5 Serial Output
- [ ] Check Serial Monitor shows:
  ```
  Starting spam with interval 100ms
  Advertizing started...
  Advertizing stop
  [repeated]
  ```

**Result:** ☐ PASS  ☐ FAIL  
**Notes:** _______________________________________________

---

## Test 7: Custom Payload - Different Payloads

**Objective:** Test multiple payload types

### 7.1 Samsung Watch Payload
- [ ] Go back to Custom Payload menu
- [ ] Load example_samsung_watch.json
- [ ] Start spam
- [ ] Verify on phone: "Galaxy Watch4" devices appear
- [ ] Stop spam

### 7.2 Custom Saved Payload
- [ ] Load a payload saved in Test 4
- [ ] Start spam
- [ ] Verify device appears on phone
- [ ] Stop spam

**Result:** ☐ PASS  ☐ FAIL  
**Notes:** _______________________________________________

---

## Test 8: Error Handling

**Objective:** Test error conditions

### 8.1 No SD Card
- [ ] Remove SD card
- [ ] Restart device
- [ ] Try to use Scan Advertiser
- [ ] Verify graceful handling (fallback to LittleFS or error message)
- [ ] Try to use Custom Payload
- [ ] Verify error handling

### 8.2 Invalid JSON
- [ ] Create invalid JSON file on SD card
- [ ] Try to load it with Custom Payload
- [ ] Verify "Load failed!" message
- [ ] Verify no crash

### 8.3 Empty Directory
- [ ] Clear /BruceSD/ble/payloads/ directory
- [ ] Try to open Custom Payload browser
- [ ] Verify empty directory message or creation

**Result:** ☐ PASS  ☐ FAIL  
**Notes:** _______________________________________________

---

## Test 9: Integration Tests

**Objective:** Verify no conflicts with other features

### 9.1 Other BLE Features
- [ ] Use other BLE menu items before/after scanner
- [ ] Verify no interference
- [ ] Test: BLE Scan
- [ ] Test: Applejuice
- [ ] Test: BLE Keyboard
- [ ] Return to Scanner/Custom Payload
- [ ] Verify still works

### 9.2 Navigation
- [ ] Test ESC from various screens
- [ ] Verify returns to correct menu
- [ ] Test rapid menu switching
- [ ] Verify no crashes

**Result:** ☐ PASS  ☐ FAIL  
**Notes:** _______________________________________________

---

## Test 10: Stress Tests

**Objective:** Test under heavy load

### 10.1 Long Scan
- [ ] Scan in area with 20+ BLE devices
- [ ] Verify all devices captured
- [ ] Check memory usage (Serial output)
- [ ] Verify no crashes

### 10.2 Long Spam
- [ ] Start spam
- [ ] Let run for 10+ minutes
- [ ] Verify counter continues incrementing
- [ ] Check for memory leaks
- [ ] Verify stop still works

### 10.3 Rapid Operations
- [ ] Quickly start/stop spam 10 times
- [ ] Verify no crashes
- [ ] Scan multiple times in succession
- [ ] Switch between scanner and spammer rapidly

**Result:** ☐ PASS  ☐ FAIL  
**Notes:** _______________________________________________

---

## Test 11: Real-World Scenarios

**Objective:** Test practical use cases

### 11.1 Capture and Replay
- [ ] Scan for real AirPods (or similar)
- [ ] Save payload
- [ ] Turn off real device
- [ ] Spam saved payload
- [ ] Verify phone sees "fake" device

### 11.2 Multiple Device Types
- [ ] Capture 3+ different device types
- [ ] Verify all save correctly
- [ ] Test spamming each
- [ ] Verify all work on phone

**Result:** ☐ PASS  ☐ FAIL  
**Notes:** _______________________________________________

---

## Test 12: Documentation Verification

**Objective:** Ensure documentation is accurate

- [ ] Follow BUILD_AND_TEST_GUIDE.md step-by-step
- [ ] Verify all steps work as documented
- [ ] Check example payloads match format in docs
- [ ] Verify file paths in docs match actual locations
- [ ] Test one example from BLE_PAYLOAD_README.md

**Result:** ☐ PASS  ☐ FAIL  
**Notes:** _______________________________________________

---

## Final Checklist

### Critical Functions
- [ ] Scan finds devices
- [ ] Scan saves payloads
- [ ] Custom payload loads files
- [ ] Custom payload spams successfully
- [ ] Visible on phone BLE scanner
- [ ] MAC randomization works
- [ ] Stop functions work
- [ ] No memory leaks observed
- [ ] No crashes during normal use

### User Experience
- [ ] Menu is intuitive
- [ ] Instructions are clear
- [ ] Feedback is responsive
- [ ] Error messages are helpful
- [ ] Navigation makes sense

### Technical Quality
- [ ] Code compiles without warnings
- [ ] Serial output is clean
- [ ] Memory usage is reasonable
- [ ] BLE stack is properly managed
- [ ] File operations are reliable

---

## Overall Test Result

**Date:** _______________  
**Tester:** _______________  
**Board:** _______________  
**Firmware Version:** _______________

**Overall Status:** ☐ ALL TESTS PASS  ☐ SOME FAILURES  ☐ MAJOR ISSUES

**Critical Issues Found:**
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________

**Recommendations:**
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________

**Sign-off:** _______________

---

## Appendix: Debug Commands

If issues occur, try these Serial commands:

```cpp
// Add to code for debugging:
Serial.println("Free heap: " + String(ESP.getFreeHeap()));
Serial.println("Largest free block: " + String(ESP.getMaxAllocHeap()));
```

Monitor output:
```bash
pio device monitor -b 115200 --filter send_on_enter
```

---

**Testing Complete!** 🎉

Save this checklist with results for future reference.
