# Build and Test Guide - BLE Custom Payload Features

## Quick Start

This guide will help you build and test the new BLE Custom Payload Scanner & Spammer features.

## Prerequisites

- PlatformIO installed
- Bruce firmware repository cloned
- ESP32 board with BLE support
- (Optional) SD card for payload storage
- Smartphone with BLE scanner app (e.g., nRF Connect, LightBlue)

## Step 1: Verify Files

Ensure all files are present:

```bash
# Core implementation files
ls -l src/modules/ble/ble_scanner.*
ls -l src/modules/ble/ble_custom_spam.*

# Modified menu file
ls -l src/core/menu_items/BleMenu.cpp

# Example payloads
ls -l sd_files/ble/payloads/
```

## Step 2: Build the Firmware

### For T-Embed CC1101 (target board):
```bash
cd /Users/matthieu/workspace-perso/Bruce
pio run -e lilygo-t-embed-cc1101
```

### For other boards, check available environments:
```bash
pio run --list-targets
# Then build for your board:
pio run -e [your-board-env]
```

### Expected Build Output:
```
Building in release mode
...
Linking .pio/build/[board]/firmware.elf
Building .pio/build/[board]/firmware.bin
RAM:   [====      ]  XX.X% (used XXXXX bytes)
Flash: [=====     ]  XX.X% (used XXXXX bytes)
SUCCESS
```

## Step 3: Flash the Firmware

### Using PlatformIO:
```bash
pio run -e [your-board] -t upload
```

### Using esptool (manual):
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash 0x0 .pio/build/[board]/firmware.bin
```

## Step 4: Prepare SD Card (Optional but Recommended)

1. Format SD card as FAT32
2. Create directory structure:
   ```
   /BruceSD/
   └── ble/
       └── payloads/
   ```
3. Copy example payloads:
   ```bash
   cp sd_files/ble/payloads/*.json /path/to/sdcard/BruceSD/ble/payloads/
   ```

## Step 5: First Boot Test

1. Insert SD card into device
2. Power on device
3. Navigate to Bluetooth menu
4. Verify new menu items appear:
   - "Scan Advertiser"
   - "Custom Payload"

## Step 6: Test Scan Advertiser

### Test Procedure:
1. Ensure you have BLE devices nearby (phone, watch, earbuds, etc.)
2. Navigate to: Bluetooth Menu → Scan Advertiser
3. Press OK to start scan
4. Wait for 10-second scan to complete
5. Verify devices appear in list
6. Navigate to a device using Up/Down
7. Press OK to view details
8. Verify displayed information:
   - Device name
   - MAC address
   - RSSI value
   - Manufacturer data (if present)
   - Service UUIDs (if present)
9. Press OK again to save payload
10. Check Serial Monitor for save confirmation

### Expected Serial Output:
```
SDCARD mounted successfully
BLE Device initialized
Scan started...
Found device: [name] [address] [rssi]
Payload saved to: /BruceSD/ble/payloads/[device_name].json
```

### Troubleshooting Scan:
- **No devices found:** Move closer to BLE devices, ensure BLE is enabled
- **Crash during scan:** Check available heap memory (reduce scan duration if needed)
- **Save failed:** Verify SD card is mounted, check directory exists

## Step 7: Test Custom Payload Spammer

### Test Procedure:
1. Navigate to: Bluetooth Menu → Custom Payload
2. Press OK to open file browser
3. Navigate to /BruceSD/ble/payloads/
4. Select example_airpods.json (or any captured payload)
5. Press OK to load
6. Review payload information displayed
7. Press OK to start spamming
8. Open BLE scanner app on phone
9. Verify advertisements appear with changing MAC addresses
10. Press ESC to stop spamming

### Expected Behavior:
- Phone BLE scanner shows multiple "AirPods Pro" devices
- MAC addresses change with each advertisement
- Device counter increments on Bruce display
- Advertisements continue until ESC pressed

### Expected Serial Output:
```
Loading payload from: /BruceSD/ble/payloads/example_airpods.json
Payload loaded successfully
Starting spam with interval 100ms
Advertizing started...
Advertizing stop
[repeated]
```

### Troubleshooting Spam:
- **Load failed:** Check JSON syntax, verify file exists
- **Not visible on phone:** Ensure phone BLE is on, try refresh
- **Crashes during spam:** Reduce spam rate, check memory

## Step 8: Verify Phone Detection

### Using nRF Connect (Android/iOS):
1. Open nRF Connect app
2. Refresh scan
3. Look for device names matching your payload
4. Notice MAC addresses changing (0-2 seconds apart)
5. Tap device to see raw advertisement data
6. Verify manufacturer data matches payload

### Using LightBlue (iOS):
1. Open LightBlue app
2. Enable continuous scan
3. Look for payload device names
4. Observe RSSI and advertisement data

## Step 9: Create Custom Payload

### Method 1: Capture from Real Device
1. Turn on your BLE device (e.g., AirPods, watch)
2. Use Scan Advertiser to capture
3. Save payload
4. Test with Custom Payload spammer

### Method 2: Manual JSON Creation
1. Create new JSON file based on format:
   ```json
   {
     "version": "1.0",
     "name": "My Custom Device",
     "type": "advertisement",
     "adv_data": {
       "flags": "0x06",
       "complete_name": "Custom BLE Device",
       "manufacturer_data": {
         "company_id": "0xFFFF",
         "data": "0102030405"
       }
     }
   }
   ```
2. Save to SD card: /BruceSD/ble/payloads/custom.json
3. Load and spam

## Step 10: Advanced Testing

### Performance Test:
1. Monitor heap memory during scan:
   - Add `Serial.println(ESP.getFreeHeap());` in code
   - Watch for memory leaks
2. Long-duration spam test:
   - Spam for 5+ minutes
   - Verify no crashes or slowdown
   - Check memory remains stable

### Compatibility Test:
1. Test with various BLE devices:
   - Apple devices (iPhone, AirPods, Apple Watch)
   - Android devices
   - Wearables (Fitbit, Samsung Watch)
   - IoT devices (smart bulbs, sensors)
2. Verify capture and replay works for each

### Stress Test:
1. Scan in area with many (20+) BLE devices
2. Verify list handling and no crashes
3. Test rapid start/stop of spam
4. Test switching between scanner and spammer quickly

## Debug Mode

To enable detailed debugging:

1. Open `src/modules/ble/ble_scanner.cpp`
2. Add at top: `#define DEBUG_BLE_SCANNER`
3. Uncomment existing Serial.println() statements
4. Rebuild and monitor Serial output

## Common Issues and Solutions

### Issue: Compile Error - "NimBLE not found"
**Solution:** Verify platformio.ini includes:
```ini
lib_deps =
    h2zero/NimBLE-Arduino@^1.4.3
```

### Issue: Compile Error - "ArduinoJson not found"
**Solution:** Add to platformio.ini:
```ini
lib_deps =
    bblanchon/ArduinoJson
```

### Issue: Menu items not appearing
**Solution:** 
- Check board environment doesn't define LITE_VERSION
- Verify BleMenu.cpp was modified correctly
- Rebuild clean: `pio run -e [board] -t clean && pio run -e [board]`

### Issue: SD card not detected
**Solution:**
- Check SD card format (FAT32)
- Verify SD card pins in board configuration
- Try LittleFS as alternative (auto-fallback implemented)

### Issue: Payload save fails
**Solution:**
- Create directory manually: /BruceSD/ble/payloads/
- Check SD card has free space
- Verify write permissions

### Issue: Spam not visible on phone
**Solution:**
- Check phone BLE is enabled
- Move phone closer to device
- Try different BLE scanner app
- Verify payload JSON is valid

## Serial Monitor Commands

Monitor output at 115200 baud:
```bash
pio device monitor -b 115200
```

Key messages to watch for:
- "SDCARD mounted successfully"
- "BLE Device initialized"
- "Scan started..."
- "Payload saved to: ..."
- "Loading payload from: ..."
- "Advertizing started..."

## Success Criteria

✅ **Scanner Working:**
- Finds nearby BLE devices
- Displays device information
- Saves payloads to JSON
- No crashes during scan

✅ **Spammer Working:**
- Loads payload files
- Displays payload info
- Starts/stops spam cleanly
- Visible on phone BLE scanner
- MAC randomization working

✅ **Integration Working:**
- Menu items appear
- No conflicts with other features
- Memory stable
- Clean navigation

## Performance Benchmarks

Expected metrics:
- Scan duration: 10 seconds
- Devices found: 5-20 (typical)
- Save time: < 1 second
- Spam rate: ~10 advertisements/second
- Memory usage: < 50KB additional
- Battery impact: Moderate (BLE active)

## Next Steps After Testing

1. ✅ Verify all tests pass
2. Document any issues found
3. Adjust parameters if needed (scan duration, spam interval)
4. Test with your specific use cases
5. Share results and feedback

## Support

If you encounter issues:
1. Check Serial Monitor output
2. Review NEW_IMPLEMENTATION.md for technical details
3. Check BLE_PAYLOAD_README.md for feature documentation
4. Open issue on GitHub with:
   - Board type
   - Build output
   - Serial monitor log
   - Steps to reproduce

---

**Test Date:** ___________
**Board:** ___________
**Result:** ___________
**Notes:** ___________
