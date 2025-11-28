# Xiaomi BLE Payloads - Test Collection

## 📋 Overview

This directory contains BLE advertisement payloads for Xiaomi devices. These payloads are for **TESTING PURPOSES** with Bruce firmware's Custom Payload feature and Xiaomi Spam module.

**⚠️ IMPORTANT:** Only **1 payload is REAL** (captured from actual device). The rest are **SYNTHETIC** (generated based on Xiaomi MiBeacon protocol structure).

---

## ✅ Payload Status

| File | Device Name | Status | OUI | Company ID |
|------|-------------|--------|-----|------------|
| `redmi_buds_4.json` | Redmi Buds 4 | ✅ **REAL** | 7C:C9:5E | 0x038F |
| `redmi_buds_3_pro.json` | Redmi Buds 3 Pro | 🔧 SYNTHETIC | 7C:C9:5E | 0x038F |
| `mi_true_wireless_earbuds_2s.json` | Mi True Wireless 2S | 🔧 SYNTHETIC | 7C:C9:5E | 0x038F |
| `xiaomi_buds_4_pro.json` | Xiaomi Buds 4 Pro | 🔧 SYNTHETIC | 58:2D:34 | 0x038F |
| `mi_band_7.json` | Mi Band 7 | 🔧 SYNTHETIC | 64:B4:73 | 0x038F |
| `mi_band_8.json` | Mi Band 8 | 🔧 SYNTHETIC | 64:B4:73 | 0x038F |
| `redmi_watch_3.json` | Redmi Watch 3 | 🔧 SYNTHETIC | 74:23:44 | 0x038F |
| `xiaomi_smart_band_7_pro.json` | Smart Band 7 Pro | 🔧 SYNTHETIC | 64:B4:73 | 0x038F |

---

## 🧪 How to Test Payloads

### **Method 1: Using Custom Payload Feature**

1. Copy payload file to SD card: `/sd_files/ble/payloads/`
2. Bruce → Bluetooth → Custom Payload
3. Select payload file
4. Enable "Random MAC"
5. Start spam
6. **Test with BLE scanner app** (nRF Connect, BLE Scanner)
7. **Report results!**

### **Method 2: Using Xiaomi Spam Module**

1. Payloads are embedded in `ble_spam_xiaomi.cpp`
2. Bruce → Bluetooth → Xiaomi Spam
3. Random MAC is **ALWAYS ON**
4. Cycles through all payloads automatically
5. Monitor with BLE scanner

---

## 📊 Xiaomi BLE Technical Info

### **Company ID**
- **0x038F** = Xiaomi Inc. (official Bluetooth SIG)

### **MAC OUIs (Organizationally Unique Identifiers)**

| OUI | Assigned To | Typical Devices |
|-----|-------------|-----------------|
| `7C:C9:5E` | Xiaomi Technology | Earbuds, IoT devices |
| `58:2D:34` | Xiaomi Communications | Smartphones, tablets |
| `F0:B4:29` | Xiaomi Inc. | Routers, IoT |
| `64:B4:73` | Xiaomi Inc. | Mi Band, wearables |
| `74:23:44` | Xiaomi Inc. | General devices |
| `78:11:DC` | Xiaomi Inc. | IoT devices |

### **MiBeacon Protocol Structure**

```
┌────────────────────────────────────────────┐
│ FLAGS (0x02 0x01 0x06/0x1A)                │
├────────────────────────────────────────────┤
│ MANUFACTURER DATA                          │
│ ├─ Length (1 byte)                         │
│ ├─ Type: 0xFF (manufacturer)               │
│ ├─ Company ID: 0x8F 0x03 (LE)             │
│ ├─ Frame Control (2 bytes)                 │
│ ├─ Product ID (2 bytes)                    │
│ ├─ Frame Counter (1 byte)                  │
│ ├─ MAC Address (6 bytes, scrambled/LE)     │
│ ├─ Capability (optional)                   │
│ └─ Object Data (variable, may be encrypted)│
└────────────────────────────────────────────┘
```

### **Product IDs (from synthetic payloads)**

- `0x0112` - Earbuds type 1 (Redmi Buds 4 - REAL)
- `0x0113` - Earbuds type 2
- `0x0114` - Earbuds type 3
- `0x0115` - Earbuds type 4
- `0x04E1` - Mi Band 7
- `0x04E2` - Mi Band 8
- `0x0A3C` - Watch series
- `0x0821` - Smart Band series

---

## 📝 How to Capture REAL Payloads

Want to add your own real Xiaomi device payloads? Here's how:

### **Using Bruce Firmware:**

1. **Put device in pairing mode:**
   - Earbuds: Open charging case near Bruce
   - Wearables: Go to Settings → Bluetooth → Make discoverable

2. **Scan with Bruce:**
   ```
   Bruce Menu
   └─ Bluetooth
      └─ BLE Scan
         └─ Scan Advertiser
   ```

3. **Find your device:**
   - Look for your device name or MAC starting with Xiaomi OUI
   - Note the advertisement data

4. **Save the payload:**
   - Bruce automatically saves to `/sd_files/ble/payloads/`
   - Or manually create JSON file

5. **Test it:**
   - Load with Custom Payload
   - Enable Random MAC
   - Verify it works!

### **Using nRF Connect (smartphone):**

1. Install nRF Connect app (Android/iOS)
2. Put Xiaomi device in pairing mode
3. Scan for devices
4. Find your device → Tap "RAW"
5. Note the **Advertisement Data** and **Scan Response**
6. Create JSON file manually using the template below

### **JSON Template:**

```json
{
  "version": "1.0",
  "name": "Your Device Name",
  "type": "advertisement",
  "address": "XX:XX:XX:YY:YY:YY",
  "rssi": -35,
  "status": "REAL - Captured",
  "notes": "Captured from real device on DATE",
  "raw_adv_data": "HEX_STRING_FROM_SCANNER",
  "raw_scan_rsp": "HEX_STRING_IF_AVAILABLE"
}
```

---

## 🎯 Testing Checklist

For each synthetic payload, test and report:

- [ ] **Detected by BLE scanner?** (Yes/No)
- [ ] **Correct Company ID visible?** (0x038F)
- [ ] **Random MAC working?** (MAC changes each broadcast)
- [ ] **Device name appears?** (if applicable)
- [ ] **Any errors on Bruce screen?**

### **Reporting Results:**

Please report your test results! For each payload:

```
Payload: redmi_buds_3_pro.json
Status: ✅ WORKS / ❌ DOESN'T WORK
Notes: [your observations]
Scanner used: nRF Connect / BLE Scanner / etc.
```

---

## 🔧 Modifying Payloads

If you want to experiment with synthetic payloads:

### **Change MAC Address:**
- Edit the `"address"` field
- Make sure to use a valid Xiaomi OUI
- MAC must be encoded in Little-Endian in `raw_adv_data`

### **Change Product ID:**
- Bytes 8-9 in manufacturer data (after 0x8F 0x03)
- Try different values to see what happens

### **Add Scan Response:**
- Some devices broadcast additional data
- Add hex string to `"raw_scan_rsp"` field

---

## ⚠️ Disclaimer

- These payloads are for **educational/testing purposes only**
- Synthetic payloads may **NOT work** on real Xiaomi devices
- Use responsibly and legally
- Test in controlled environment
- **DO NOT use for malicious purposes**

---

## 📚 References

- [Bluetooth SIG Company IDs](https://www.bluetooth.com/specifications/assigned-numbers/)
- [Xiaomi MiBeacon Protocol](https://github.com/pvvx/ATC_MiThermometer)
- [Bruce Firmware](https://github.com/pr3y/Bruce)
- [nRF Connect App](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile)

---

## 📬 Contribution

Found a real payload or tested synthetic ones? Share your results!

1. Test the payloads
2. Document what works/doesn't work
3. Capture real payloads from your devices
4. Report back with results

**Last Updated:** 2024-11-28
