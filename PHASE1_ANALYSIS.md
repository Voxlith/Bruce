# Phase 1 Analysis: BAD NRF (MouseJack) Implementation

**Date**: 2025-01-15
**Status**: ✅ COMPLETED
**Target Device**: T-Embed CC1101 Plus with nRF24L01+

---

## 1. Existing Architecture Analysis

### 1.1 NRF24 Menu Structure

**Location**: `src/core/menu_items/NRF24.cpp`

Current menu items:
- Information (`nrf_info`)
- Spectrum (`nrf_spectrum`)
- NRF Jammer (`nrf_jammer`)
- CH Jammer (`nrf_channel_jammer`)
- CH hopper (`nrf_channel_hopper`)
- Config pins (for M5Stick devices)

**Menu Implementation Pattern**:
```cpp
void NRF24Menu::optionsMenu() {
    options.clear();
    options.push_back({"Menu Item", callback_function});
    // ... more options
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "NRF24");
}
```

**Integration Point**: Add "BAD NRF" entry after existing options, before config menu.

### 1.2 Existing NRF24 Functions Analysis

#### nrf_spectrum() - `/src/modules/NRF24/nrf_spectrum.cpp`
- **Key Learning**: Already implements promiscuous mode for spectrum analysis
- **Configuration Used**:
  ```cpp
  NRFradio.setAutoAck(false);
  NRFradio.disableCRC();
  NRFradio.setAddressWidth(2);
  NRFradio.setDataRate(RF24_1MBPS);
  ```
- **Scanning Pattern**: Iterates channels 0-79, uses `testRPD()` for carrier detection
- **Display**: Real-time visualization with TFT graphics

#### nrf_jammer() - `/src/modules/NRF24/nrf_jammer.cpp`
- **Key Learning**: Shows channel targeting patterns
- **Configuration**:
  ```cpp
  NRFradio.setPALevel(RF24_PA_MAX);
  NRFradio.setDataRate(RF24_2MBPS);
  NRFradio.setAddressWidth(5);
  NRFradio.setPayloadSize(2);
  ```
- **Mode Switching**: Interactive menu with different channel sets
- **Display Pattern**: Status updates with mode selection UI

#### nrf_common.cpp - `/src/modules/NRF24/nrf_common.cpp`
- **Initialization**: `nrf_start()` handles SPI bus detection and setup
- **SPI Bus Logic**: Automatically detects shared SPI with SD Card, TFT, or CC1101
- **Global Radio Object**: `RF24 NRFradio(NRF24_CE_PIN, NRF24_SS_PIN)`

**✅ CONCLUSION**: All necessary RF24 initialization patterns exist. Can reuse `nrf_start()` and similar display patterns.

---

## 2. RF24 Library v1.4.11 Capabilities

**Library**: `nrf24/RF24 @ 1.4.11`
**Location**: `.pio/libdeps/lilygo-t-embed-cc1101/RF24/`

### 2.1 Promiscuous Mode Support

**✅ VERIFIED**: RF24 v1.4.11 supports all required features for MouseJack:

| Feature | Function | Support |
|---------|----------|---------|
| Disable Auto-ACK | `setAutoAck(false)` | ✅ Yes |
| Disable CRC | `disableCRC()` | ✅ Yes |
| Variable Address Width | `setAddressWidth(2-5)` | ✅ Yes |
| 2Mbps Data Rate | `setDataRate(RF24_2MBPS)` | ✅ Yes |
| Max Power | `setPALevel(RF24_PA_MAX)` | ✅ Yes |
| Payload Size | `setPayloadSize(32)` | ✅ Yes |
| Channel Switching | `setChannel(0-125)` | ✅ Yes |
| Raw Packet Read | `read()` | ✅ Yes |
| Raw Packet Write | `write()` | ✅ Yes |

### 2.2 Recommended Configuration for MouseJack

```cpp
// Promiscuous scanning mode
radio.setAutoAck(false);
radio.setPALevel(RF24_PA_MAX);
radio.setDataRate(RF24_2MBPS);  // Logitech uses 2Mbps
radio.setPayloadSize(32);
radio.disableCRC();             // For promiscuous mode
radio.setAddressWidth(2);       // Reduced for wider packet capture

// For injection mode
radio.setAutoAck(false);
radio.setDataRate(RF24_2MBPS);
radio.setAddressWidth(5);       // Standard Logitech address width
radio.setPayloadSize(10);       // Logitech keyboard packet size
```

**⚠️ LIMITATION**: nRF24L01+ range is limited (~10m without PA/LNA). T-Embed has integrated module without external PA/LNA.

---

## 3. BadUSB Ducky Script Parser Analysis

**Location**: `src/modules/badusb_ble/ducky_typer.cpp`

### 3.1 Parser Architecture

**Key Components**:
1. **Command Types**: `DuckyCommandType` enum (Print, Cmd, Delay, Comment, Loop, Combination)
2. **Command Tables**: 
   - `duckyCmds[]` - Command definitions (STRING, DELAY, CTRL, etc.)
   - `duckyComb[]` - Key combinations (CTRL-ALT, GUI-SHIFT, etc.)
3. **Main Parser**: `key_input(FS fs, String bad_script, HIDInterface *hid)`

### 3.2 Parsing Flow

```
1. Open file → Read line by line
2. Parse command → Split at first space
3. Match command in duckyCmds[] array
4. Process based on type:
   - Print → hid->print(Argument)
   - Delay → delay(ms)
   - Cmd → hid->press(key)
   - Combination → press multiple keys
5. Release all keys → hid->releaseAll()
6. Repeat if REPEAT command
```

### 3.3 HID Interface Abstraction

**Key Interface**: `HIDInterface` base class in `lib/Bad_Usb_Lib/Bad_Usb_Lib.h`

Virtual methods:
- `press(uint8_t k)` - Press key
- `release(uint8_t k)` - Release key
- `releaseAll()` - Release all keys
- `write(uint8_t k)` - Write character
- `print(String)` - Print string

**Key Codes**: Defined in `lib/Bad_Usb_Lib/keys.h`
- Modifiers: `KEY_LEFT_CTRL`, `KEY_LEFT_SHIFT`, `KEY_LEFT_ALT`, `KEY_LEFT_GUI`
- Special keys: `KEY_RETURN`, `KEY_ESC`, `KEY_TAB`, `KEY_DELETE`, etc.
- Function keys: `KEY_F1` through `KEY_F12`

### 3.4 Reusability for BAD NRF

**✅ STRATEGY**: Create NRF-specific HID interface that:
1. Extends `HIDInterface` base class
2. Converts HID key codes to Logitech packet format
3. Sends packets via nRF24L01+ radio
4. Reuses existing parser: `key_input()` function

**Example Structure**:
```cpp
class NRF24HIDInterface : public HIDInterface {
    RF24* radio;
    uint8_t target_address[5];
    uint8_t target_channel;
    
    virtual size_t press(uint8_t k) {
        // Convert HID keycode to Logitech format
        // Send via radio
    }
    // ... implement other methods
};
```

---

## 4. File Storage Structure

### 4.1 Current SD Card Organization

**Base Path**: `/sd_files/` (repository example structure)

Existing directories:
- `/BadUSB and BlueDucky/` - Ducky scripts (`.txt` files)
- `/infrared/` - IR codes
- `/nfc/` - NFC/RFID tags
- `/wifi/` - WiFi configs
- `/themes/` - UI themes

### 4.2 Ducky Script Access

**File Browser**: `loopSD(FS &fs, bool filePicker, String allowed_ext, String rootPath)`
- Supports SD Card and LittleFS
- Filters by extension
- Returns full file path
- Used across multiple modules

**Current BadUSB Usage**:
```cpp
bad_script = loopSD(*fs, true);  // No filter, starts at root
```

### 4.3 Proposed BAD NRF Storage

**Directory Structure**:
```
/badnrf/
├── targets/              # Saved target devices (JSON)
│   ├── office_pc.json
│   ├── lab_mouse.json
│   └── ...
└── payloads/             # OR reuse /BadUSB and BlueDucky/
    ├── rickroll.txt
    ├── reverse_shell.txt
    └── ...
```

**Target JSON Format** (from IMPLEMENTATION.md):
```json
{
  "version": "1.0",
  "name": "Bureau PC",
  "type": "logitech_unifying",
  "address": "BB:29:0A:F1:C8",
  "channel": 32,
  "data_rate": "2MBPS",
  "discovered_at": "2025-01-15T10:30:00Z",
  "vulnerable": true,
  "notes": "Optional user notes"
}
```

**✅ RECOMMENDATION**: 
- **Payloads**: Reuse existing `/BadUSB and BlueDucky/` directory
- **Targets**: Create new `/badnrf/targets/` directory
- Use `loopSD(*fs, true, "JSON", "/badnrf/targets")` for target selection

---

## 5. Logitech Unifying Protocol (Technical)

### 5.1 Protocol Basics

**Frequency**: 2.400 - 2.483 GHz (ISM band)
**Channels**: 0-79 (nRF24 mapping) → Logitech uses channels 2-83 with hopping
**Data Rate**: 2 Mbps
**Address**: 5 bytes
**Packet Size**: 10-22 bytes (keyboard), 5 bytes (keepalive)

### 5.2 Channel Hopping Pattern

Logitech devices hop between 25-32 channels:
- **Hop Interval**: ~1-5ms
- **Pattern**: Pseudo-random based on device pairing
- **Common Channels**: 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 53, 56, 59, 62, 65, 68, 71, 74, 77, 80

**Scanning Strategy**:
```cpp
// Scan with step 3 (covers Logitech range)
for (uint8_t channel = 2; channel <= 83; channel += 3) {
    radio.setChannel(channel);
    // Listen for packets...
}
```

### 5.3 Packet Format (Unencrypted Keyboard)

**Keystroke Packet** (10 bytes):
```
Byte 0:    Device type (0x00 = keepalive, 0x04 = keyboard, 0x02 = mouse)
Byte 1:    Modifiers (Ctrl=0x01, Shift=0x02, Alt=0x04, GUI=0x08)
Byte 2:    Reserved (0x00)
Byte 3-8:  Key codes (up to 6 simultaneous keys)
Byte 9:    Checksum (XOR of bytes 0-8)
```

**Example - Press 'A' key**:
```
04 00 00 04 00 00 00 00 00 00  // Press A (keycode 0x04)
04 00 00 00 00 00 00 00 00 04  // Release (empty keys)
```

**Example - CTRL+ALT+DELETE**:
```
04 05 00 4C 00 00 00 00 00 45  // Modifiers=0x05 (Ctrl+Alt), Key=0x4C (Del)
```

### 5.4 Device Detection Patterns

**Logitech Preamble**: `0xAA` prefix in some models
**Microsoft Preamble**: `0x55` prefix

**Address Patterns**:
- Logitech Unifying: Often starts with `0xBB`, `0xC2`, `0xCA`
- Generic 2.4GHz: Variable

### 5.5 Vulnerable Devices

**Known Vulnerable** (pre-2016 firmware):
- Logitech Unifying Receiver C-U0007
- Logitech Unifying Receiver C-U0008
- Some Logitech non-Unifying (C-U0010)
- Microsoft Wireless Keyboard 2000/3000
- Generic wireless keyboards/mice

**Vulnerability**: No encryption or weak encryption on keystroke packets, accepts packets without proper authentication.

### 5.6 MouseJack Attack Vector

1. **Passive Scan**: Detect dongle address and channel
2. **Fingerprint**: Identify dongle type (Logitech/MS/Generic)
3. **Test Injection**: Send benign test packet (e.g., Shift x5)
4. **Payload Delivery**: If successful, inject Ducky script keystrokes

---

## 6. SPI Bus Compatibility

### 6.1 T-Embed CC1101 Plus Configuration

**NRF24 Pins**:
- CE: GPIO 10
- CS: GPIO 9  
- SCK: Shared with TFT/SD
- MOSI: Shared with TFT/SD
- MISO: Shared with TFT/SD

**SPI Detection Logic** (from `nrf_common.cpp`):
```cpp
if (bruceConfigPins.NRF24_bus.mosi == (gpio_num_t)TFT_MOSI) {
    NRFSPI = &tft.getSPIinstance();
} else if (bruceConfigPins.NRF24_bus.mosi == bruceConfigPins.SDCARD_bus.mosi) {
    NRFSPI = &sdcardSPI;
} else {
    NRFSPI = &SPI;
}
```

**✅ COMPATIBILITY**: Existing `nrf_start()` handles all SPI bus detection automatically. No additional configuration needed.

---

## 7. Implementation Recommendations

### 7.1 Module Structure

**New Files**:
```
src/modules/nrf/
├── bad_nrf.h          # Declarations
└── bad_nrf.cpp        # Implementation
```

**Header Include** in `NRF24.cpp`:
```cpp
#include "modules/nrf/bad_nrf.h"
```

### 7.2 Menu Integration

Add to `NRF24Menu::optionsMenu()`:
```cpp
options.push_back({"BAD NRF", badnrf_menu});
```

Create submenu:
```cpp
void badnrf_menu() {
    options = {
        {"Scan Devices",  badnrf_scan},
        {"Saved Targets", badnrf_targets},
        {"Test Target",   badnrf_test},
        {"Run Payload",   badnrf_payload},
        {"Back",          [=]() { /* return */ }}
    };
    loopOptions(options, false, true, "BAD NRF");
}
```

### 7.3 Core Data Structures

```cpp
struct NRFDevice {
    uint8_t address[5];
    uint8_t channel;
    int8_t rssi;
    String type;  // "logitech", "microsoft", "generic"
    bool vulnerable;
    uint32_t last_seen;
};

class NRF24HIDInterface : public HIDInterface {
    RF24* radio;
    NRFDevice* target;
    // Implement HID interface methods
};
```

### 7.4 Key Functions to Implement

1. **Scanner**:
   - `void badnrf_scan()` - Main scanning loop
   - `bool detectDevice(uint8_t* packet, NRFDevice& device)` - Packet analysis
   - `void displayDevices(Vector<NRFDevice>& devices)` - UI display

2. **Target Management**:
   - `void badnrf_save_target(NRFDevice& device)` - Save to JSON
   - `NRFDevice badnrf_load_target(String filepath)` - Load from JSON
   - `void badnrf_targets()` - Target browser menu

3. **Injection**:
   - `bool badnrf_test_target(NRFDevice& target)` - Vulnerability test
   - `void badnrf_send_keystroke(uint8_t key, uint8_t modifiers)` - Single key
   - `void badnrf_run_payload(String script_path, NRFDevice& target)` - Full script

4. **HID Conversion**:
   - `uint8_t hid_to_logitech(uint8_t hid_key)` - Keycode mapping
   - `void create_keyboard_packet(uint8_t* buffer, uint8_t key, uint8_t modifiers)` - Packet builder
   - `uint8_t calculate_checksum(uint8_t* packet, uint8_t len)` - Checksum

### 7.5 Reuse Existing Code

**FROM**: `ducky_typer.cpp`
- Reuse: `key_input()` with custom NRF HID interface
- Reuse: Command tables `duckyCmds[]`, `duckyComb[]`

**FROM**: `nrf_spectrum.cpp`
- Reuse: Promiscuous mode configuration
- Reuse: Channel scanning pattern
- Adapt: Display visualization

**FROM**: `nrf_jammer.cpp`
- Reuse: Display layout and UI patterns
- Reuse: Continuous operation with button checks

---

## 8. Risk and Limitations

### 8.1 Technical Limitations

| Limitation | Impact | Mitigation |
|------------|--------|------------|
| nRF24L01+ range (~10m) | Short-range attacks only | User awareness |
| No external PA/LNA | Reduced effectiveness | Use closer proximity |
| Channel hopping detection | May miss fast-hopping devices | Longer scan time |
| Encrypted dongles | Recent Logitech immune | Display "Not vulnerable" |
| SPI bus conflicts | Potential interference | Use existing SPI mgmt |

### 8.2 Legal and Ethical Considerations

**⚠️ CRITICAL**: 
- Feature is ILLEGAL to use without authorization
- Must display prominent disclaimer (similar to `nrf_info()`)
- Only for authorized penetration testing
- User is responsible for compliance with local laws

**Recommended Disclaimer**:
```cpp
void badnrf_disclaimer() {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("_WARNING_", tftWidth / 2, 10, 1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setCursor(15, 33);
    padprintln("BAD NRF (MouseJack) is for AUTHORIZED SECURITY TESTING ONLY.");
    padprintln("");
    padprintln("Unauthorized use is ILLEGAL and you can face CRIMINAL PROSECUTION.");
    // ... continue
}
```

### 8.3 Success Probability

**Expected Results**:
- ✅ **High Success**: Pre-2016 Logitech Unifying dongles
- ⚠️ **Medium Success**: Microsoft wireless keyboards (2000/3000 series)
- ❌ **Low Success**: Modern Logitech (post-2016), encrypted devices
- ❌ **No Success**: Bluetooth devices, wired keyboards

---

## 9. Phase 1 Completion Summary

### ✅ Analysis Complete

**What We Learned**:
1. ✅ NRF24 menu structure and integration points identified
2. ✅ Existing NRF functions provide reusable patterns
3. ✅ RF24 v1.4.11 supports all required MouseJack features
4. ✅ BadUSB Ducky parser is fully reusable with HID abstraction
5. ✅ File storage structure defined (reuse BadUSB, add /badnrf/targets/)
6. ✅ Logitech Unifying protocol documented
7. ✅ SPI bus compatibility confirmed (auto-detection works)

**Risks Identified**:
- ⚠️ Range limitation (~10m)
- ⚠️ Legal/ethical considerations require strong disclaimers
- ⚠️ Modern devices are likely immune

**Dependencies Verified**:
- ✅ RF24 v1.4.11 installed and functional
- ✅ HIDInterface abstraction exists
- ✅ File system utilities available
- ✅ Display and input handling patterns established

---

## 10. Next Steps (Phase 2)

**Ready to Proceed**: Create module files and implement scanner

**Phase 2 Tasks**:
1. Create `src/modules/nrf/bad_nrf.h` and `.cpp`
2. Implement device scanner with promiscuous mode
3. Implement device detection and fingerprinting
4. Add real-time display during scan
5. Test scanning on actual wireless devices

**Estimated Complexity**: Medium-High
**Estimated Time**: 4-6 hours for scanner implementation

---

**Analysis Completed**: 2025-01-15
**Analyst**: Claude (OpenCode)
**Status**: ✅ READY FOR PHASE 2
