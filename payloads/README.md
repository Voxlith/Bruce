# 🧪 BLE Custom Payload - Test Files

## 📋 Overview

This folder contains **8 test JSON files** to identify the correct MAC address randomization pattern for Xiaomi BLE devices.

**Base MAC:** `7c:c9:5e:7a:75:9d`  
**Test MAC:** `7c:c9:5e:AA:BB:CC`

---

## 📁 Test Files

### **TEST1: `test1_mac_position_15-20.json`**
**Hypothèse:** MAC at position 15-20 (partial inverted format)
- **Modified bytes:** Position 15-17 in raw_adv_data
- **Change:** `9D757A` → `CCBBAA`
- **Priority:** MEDIUM

---

### **TEST2: `test2_mac_little_endian_full.json`**
**Hypothèse:** Full MAC in little-endian at position 15-20
- **Modified bytes:** Position 15-20 in raw_adv_data
- **Change:** `9D757AC97C5E` → `CCBBAA5EC97C`
- **Priority:** HIGH
- **Note:** Standard BLE little-endian format

---

### **TEST3: `test3_mac_position_21-26.json`**
**Hypothèse:** MAC at position 21-26
- **Modified bytes:** Position 21-23 in raw_adv_data
- **Change:** `9D757A` → `CCBBAA`
- **Priority:** LOW

---

### **TEST4: `test4_mac_scattered_all.json`**
**Hypothèse:** MAC fragmented across all positions
- **Modified bytes:** ALL occurrences of `9D757A` in raw_adv_data
- **Change:** Multiple replacements throughout payload
- **Priority:** MEDIUM
- **Note:** Tests proprietary Xiaomi fragmentation

---

### **TEST5: `test5_no_mac_change.json`** ⭐
**Hypothèse:** BLE MAC alone is sufficient (no raw_adv_data modification)
- **Modified bytes:** NONE in raw_adv_data
- **Change:** Only `address` field changed
- **Priority:** **HIGHEST**
- **Note:** User's 60% hypothesis - simplest approach

---

### **TEST6: `test6_mac_big_endian.json`**
**Hypothèse:** MAC in big-endian (normal order) at position 15-20
- **Modified bytes:** Position 15-20 in raw_adv_data
- **Change:** `9D757AC97C5E` → `7CC95EAABBCC`
- **Priority:** MEDIUM

---

### **TEST7: `test7_only_last_3bytes.json`**
**Hypothèse:** Only last 3 MAC bytes (randomized part) in raw_adv_data
- **Modified bytes:** All occurrences of `9D757A` (last 3 bytes LE)
- **Change:** `9D757A` → `CCBBAA`
- **Priority:** HIGH
- **Note:** Matches OUI randomization logic

---

### **TEST8: `test8_manufacturer_data_only.json`**
**Hypothèse:** MAC only in manufacturer_data field
- **Modified bytes:** `manufacturer_data.data` field
- **Change:** Modified manufacturer data, raw_adv_data unchanged
- **Priority:** LOW

---

## 🎯 Testing Order (Recommended)

1. **TEST5** - No raw_adv_data change (60% hypothesis)
2. **TEST2** - Little-endian full (standard BLE)
3. **TEST7** - Last 3 bytes only (OUI logic)
4. **TEST4** - Scattered MAC (Xiaomi pattern)
5. **TEST1** - Position 15-20 partial
6. **TEST6** - Big-endian
7. **TEST3** - Position 21-26
8. **TEST8** - Manufacturer data only

---

## ✅ Success Criteria

For each test, verify:

| Check | Description |
|-------|-------------|
| ✅ **Phone Detection** | "Xiaomi Earbuds" appears in Bluetooth scan |
| ✅ **Name Display** | Matches `complete_name` in JSON (TEST1-TEST8) |
| ✅ **Serial Logs** | BLE MAC changes? Raw data changes? |
| ✅ **Persistence** | Detection remains after multiple spam cycles |

---

## 📊 Raw Data Analysis

**Original raw_adv_data structure:**
```
Position:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21...
Hex:      02 01 1A 1B FF 8F 03 16 01 12 C1 4A E4 E4 26 9D 75 7A C9 7C 5E 9D...
                                                       ^^^^^^^^^^^ Suspected pattern
```

**MAC address:** `7c:c9:5e:7a:75:9d`  
**Little-endian:** `9D 75 7A 5E C9 7C`  
**Found at pos 15-20:** `9D 75 7A C9 7C 5E` ← **INVERTED!**

**Observation:** Bytes 18-20 are inverted (`C9 7C 5E` instead of `5E C9 7C`)  
**Conclusion:** Proprietary Xiaomi format

---

## 🔧 How to Use

### **With Random MAC ON:**
1. Load test file via BLE Custom Payload menu
2. **Toggle:** `[✓] Random MAC` (enabled)
3. **Start spam** - MAC will randomize each cycle
4. **Observe:** Does phone detect device?

### **With Random MAC OFF:**
1. Load test file
2. **Toggle:** `[ ] Random MAC` (disabled)
3. **Start spam** - Uses fixed MAC `7c:c9:5e:AA:BB:CC`
4. **Observe:** Does phone detect device?

---

## 📝 Notes

- **Default:** Random MAC is **ENABLED**
- **OUI preserved:** First 3 bytes (`7c:c9:5e`) kept when randomizing
- **Cycle interval:** 100ms between advertisements
- **TX power:** Maximum for best range
- **Deinit/Init:** BLE stack restarted each cycle to apply new MAC

---

## 🐛 Expected Results

### **If TEST5 works:**
→ Hypothesis confirmed: BLE MAC alone is sufficient  
→ No need to modify raw_adv_data  
→ Simplify implementation

### **If TEST2/TEST7 works:**
→ Standard BLE little-endian format  
→ Implement proper LE MAC replacement

### **If TEST4 works:**
→ Xiaomi uses fragmented MAC  
→ Implement custom fragmentation logic

### **If none work:**
→ MAC might be encrypted/hashed in payload  
→ Consider alternative approaches (different device type)

---

**Good luck testing! 🚀**
