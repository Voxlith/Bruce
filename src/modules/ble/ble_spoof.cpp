/**
 * BLE Audio Spoof + HID Injection Module for Bruce Firmware
 * Implementation file
 */

#include "ble_spoof.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include "lib/Bad_Usb_Lib/keys.h"

// HID Report IDs
#define KEYBOARD_ID 0x01
#define MEDIA_KEYS_ID 0x02

// HID Report Descriptor for Keyboard (simplified version)
static const uint8_t hidReportDescriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, KEYBOARD_ID,             //   REPORT_ID (1)
    0x05, 0x07,                    //   USAGE_PAGE (Key Codes)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Key Codes)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};

// Global instance
BleSpoofModule bleSpoofModule;

// ============================================================================
// BleSpoofScanCallbacks Implementation
// ============================================================================

BleSpoofScanCallbacks::BleSpoofScanCallbacks() {
    devices.clear();
}

void BleSpoofScanCallbacks::onResult(NimBLEAdvertisedDevice *advertisedDevice) {
    BleDeviceProfile profile;
    
    // Basic information
    profile.name = advertisedDevice->getName().c_str();
    profile.address = advertisedDevice->getAddress().toString().c_str();
    profile.addressType = advertisedDevice->getAddressType();
    profile.rssi = advertisedDevice->getRSSI();
    
    // Appearance
    if (advertisedDevice->haveAppearance()) {
        profile.appearance = advertisedDevice->getAppearance();
    }
    
    // TX Power
    if (advertisedDevice->haveTXPower()) {
        profile.txPower = advertisedDevice->getTXPower();
    }
    
    // Manufacturer Data
    if (advertisedDevice->haveManufacturerData()) {
        std::string mfgData = advertisedDevice->getManufacturerData();
        profile.manufacturerData = payloadToHex((const uint8_t*)mfgData.data(), mfgData.length());
    }
    
    // Service UUIDs
    if (advertisedDevice->haveServiceUUID()) {
        // Get all service UUIDs (NimBLE may have multiple)
        // Note: getServiceUUID() returns the first one, we need to get all
        // For now, we'll just get the first one
        profile.serviceUUIDs.push_back(advertisedDevice->getServiceUUID().toString().c_str());
    }
    
    // Complete payload (raw advertisement data)
    profile.payloadLength = advertisedDevice->getPayloadLength();
    profile.payload = payloadToHex(advertisedDevice->getPayload(), profile.payloadLength);
    
    // Add to list
    devices.push_back(profile);
    
    Serial.printf("BLE Device found: %s (%s) RSSI: %d\n", 
                  profile.name.c_str(), 
                  profile.address.c_str(), 
                  profile.rssi);
}

String BleSpoofScanCallbacks::payloadToHex(const uint8_t *payload, size_t length) {
    String hex = "";
    for (size_t i = 0; i < length; i++) {
        char buf[3];
        sprintf(buf, "%02X", payload[i]);
        hex += buf;
    }
    return hex;
}

// ============================================================================
// BleSpoofModule Implementation
// ============================================================================

BleSpoofModule::BleSpoofModule() {
    scanCallbacks = nullptr;
    pBLEScan = nullptr;
    isScanning = false;
    spoofingActive = false;
    pServer = nullptr;
    pAdvertising = nullptr;
    // HID Phase 4
    hidEnabled = false;
    pHID = nullptr;
    pInputKeyboard = nullptr;
    pOutputKeyboard = nullptr;
    batteryLevel = 100;
}

BleSpoofModule::~BleSpoofModule() {
    if (scanCallbacks) {
        delete scanCallbacks;
        scanCallbacks = nullptr;
    }
    stopSpoofing();
}

bool BleSpoofModule::startScan(int duration) {
    Serial.println("[BLE Spoof] Starting scan...");
    
    // Initialize BLE if not already done
    if (!NimBLEDevice::getInitialized()) {
        NimBLEDevice::init("");
    }
    
    // Create scan callbacks if not exist
    if (!scanCallbacks) {
        scanCallbacks = new BleSpoofScanCallbacks();
    }
    
    // Clear previous results
    scanCallbacks->clearDevices();
    
    // Get scan instance
    pBLEScan = NimBLEDevice::getScan();
    if (!pBLEScan) {
        Serial.println("[BLE Spoof] Failed to get scan instance");
        return false;
    }
    
    // Configure scan
    pBLEScan->setAdvertisedDeviceCallbacks(scanCallbacks, false);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    // Start scan
    isScanning = true;
    NimBLEScanResults results = pBLEScan->start(duration, false);
    isScanning = false;
    
    Serial.printf("[BLE Spoof] Scan complete. Found %d devices\n", scanCallbacks->getDevices().size());
    
    return true;
}

void BleSpoofModule::stopScan() {
    if (pBLEScan && isScanning) {
        pBLEScan->stop();
        isScanning = false;
        Serial.println("[BLE Spoof] Scan stopped");
    }
}

std::vector<BleDeviceProfile> BleSpoofModule::getScannedDevices() const {
    if (scanCallbacks) {
        return scanCallbacks->getDevices();
    }
    return std::vector<BleDeviceProfile>();
}

void BleSpoofModule::clearScannedDevices() {
    if (scanCallbacks) {
        scanCallbacks->clearDevices();
    }
    if (pBLEScan) {
        pBLEScan->clearResults();
    }
}

bool BleSpoofModule::saveProfile(const BleDeviceProfile &profile, FS &fs, const String &filename) {
    // Ensure directory exists
    ensureDirectoryExists(fs, "/ble_spoof");
    ensureDirectoryExists(fs, "/ble_spoof/profiles");
    
    // Create full path
    String filepath = "/ble_spoof/profiles/" + filename;
    if (!filepath.endsWith(".json")) {
        filepath += ".json";
    }
    
    // Create JSON document
    JsonDocument doc;
    if (!profileToJson(profile, doc)) {
        Serial.println("[BLE Spoof] Failed to convert profile to JSON");
        return false;
    }
    
    // Open file for writing
    File file = fs.open(filepath, FILE_WRITE);
    if (!file) {
        Serial.printf("[BLE Spoof] Failed to open file for writing: %s\n", filepath.c_str());
        return false;
    }
    
    // Serialize JSON to file
    if (serializeJsonPretty(doc, file) == 0) {
        Serial.println("[BLE Spoof] Failed to write JSON to file");
        file.close();
        return false;
    }
    
    file.close();
    Serial.printf("[BLE Spoof] Profile saved to: %s\n", filepath.c_str());
    return true;
}

bool BleSpoofModule::loadProfile(BleDeviceProfile &profile, FS &fs, const String &filepath) {
    // Open file for reading
    File file = fs.open(filepath, FILE_READ);
    if (!file) {
        Serial.printf("[BLE Spoof] Failed to open file for reading: %s\n", filepath.c_str());
        return false;
    }
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("[BLE Spoof] Failed to parse JSON: %s\n", error.c_str());
        return false;
    }
    
    // Convert JSON to profile
    if (!jsonToProfile(doc, profile)) {
        Serial.println("[BLE Spoof] Failed to convert JSON to profile");
        return false;
    }
    
    Serial.printf("[BLE Spoof] Profile loaded from: %s\n", filepath.c_str());
    return true;
}

bool BleSpoofModule::deleteProfile(FS &fs, const String &filepath) {
    if (fs.remove(filepath)) {
        Serial.printf("[BLE Spoof] Profile deleted: %s\n", filepath.c_str());
        return true;
    }
    Serial.printf("[BLE Spoof] Failed to delete profile: %s\n", filepath.c_str());
    return false;
}

std::vector<String> BleSpoofModule::listProfiles(FS &fs, const String &directory) {
    std::vector<String> profiles;
    
    File dir = fs.open(directory);
    if (!dir || !dir.isDirectory()) {
        Serial.printf("[BLE Spoof] Directory not found: %s\n", directory.c_str());
        return profiles;
    }
    
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String filename = file.name();
            if (filename.endsWith(".json")) {
                profiles.push_back(String(file.path()));
            }
        }
        file = dir.openNextFile();
    }
    
    dir.close();
    Serial.printf("[BLE Spoof] Found %d profiles in %s\n", profiles.size(), directory.c_str());
    return profiles;
}

bool BleSpoofModule::profileToJson(const BleDeviceProfile &profile, JsonDocument &doc) {
    doc["name"] = profile.name;
    doc["address"] = profile.address;
    doc["addressType"] = profile.addressType;
    doc["rssi"] = profile.rssi;
    doc["appearance"] = profile.appearance;
    doc["txPower"] = profile.txPower;
    doc["manufacturerData"] = profile.manufacturerData;
    doc["payload"] = profile.payload;
    doc["payloadLength"] = profile.payloadLength;
    
    // Service UUIDs as array
    JsonArray uuids = doc["serviceUUIDs"].to<JsonArray>();
    for (const String &uuid : profile.serviceUUIDs) {
        uuids.add(uuid);
    }
    
    return true;
}

bool BleSpoofModule::jsonToProfile(const JsonDocument &doc, BleDeviceProfile &profile) {
    profile.clear();
    
    if (!doc["name"].isNull()) profile.name = doc["name"].as<String>();
    if (!doc["address"].isNull()) profile.address = doc["address"].as<String>();
    if (!doc["addressType"].isNull()) profile.addressType = doc["addressType"].as<uint8_t>();
    if (!doc["rssi"].isNull()) profile.rssi = doc["rssi"].as<int>();
    if (!doc["appearance"].isNull()) profile.appearance = doc["appearance"].as<uint16_t>();
    if (!doc["txPower"].isNull()) profile.txPower = doc["txPower"].as<int>();
    if (!doc["manufacturerData"].isNull()) profile.manufacturerData = doc["manufacturerData"].as<String>();
    if (!doc["payload"].isNull()) profile.payload = doc["payload"].as<String>();
    if (!doc["payloadLength"].isNull()) profile.payloadLength = doc["payloadLength"].as<size_t>();
    
    // Service UUIDs
    if (!doc["serviceUUIDs"].isNull() && doc["serviceUUIDs"].is<JsonArray>()) {
        JsonArray uuids = doc["serviceUUIDs"].as<JsonArray>();
        for (JsonVariant uuid : uuids) {
            profile.serviceUUIDs.push_back(uuid.as<String>());
        }
    }
    
    return true;
}

void BleSpoofModule::ensureDirectoryExists(FS &fs, const String &path) {
    if (!fs.exists(path)) {
        if (fs.mkdir(path)) {
            Serial.printf("[BLE Spoof] Created directory: %s\n", path.c_str());
        } else {
            Serial.printf("[BLE Spoof] Failed to create directory: %s\n", path.c_str());
        }
    }
}

String BleSpoofModule::hexToString(const uint8_t *data, size_t length) {
    String hex = "";
    for (size_t i = 0; i < length; i++) {
        char buf[3];
        sprintf(buf, "%02X", data[i]);
        hex += buf;
    }
    return hex;
}

std::vector<uint8_t> BleSpoofModule::hexStringToBytes(const String &hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        String byteString = hex.substring(i, i + 2);
        uint8_t byte = (uint8_t)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

// ============================================================================
// Spoofing Functions (Phase 3)
// ============================================================================

bool BleSpoofModule::setSpoofName(const String &name) {
    if (!NimBLEDevice::getInitialized()) {
        Serial.println("[BLE Spoof] BLE not initialized");
        return false;
    }
    
    // Set device name
    NimBLEDevice::deinit(false);
    NimBLEDevice::init(name.c_str());
    
    currentProfile.name = name;
    Serial.printf("[BLE Spoof] Name set to: %s\n", name.c_str());
    return true;
}

bool BleSpoofModule::setSpoofAddress(const String &address) {
    Serial.printf("[BLE Spoof] Setting random BLE address: %s\n", address.c_str());
    
    // Parse the MAC address string (format: AA:BB:CC:DD:EE:FF)
    uint8_t addr[6];
    int values[6];
    
    if (sscanf(address.c_str(), "%x:%x:%x:%x:%x:%x", 
               &values[0], &values[1], &values[2], 
               &values[3], &values[4], &values[5]) != 6) {
        Serial.println("[BLE Spoof] Invalid MAC address format");
        return false;
    }
    
    // Convert to uint8_t array (reverse order for BLE)
    for (int i = 0; i < 6; i++) {
        addr[5 - i] = (uint8_t)values[i];
    }
    
    // Set bit 6 and clear bit 7 to make it a valid random static address
    // This is required by BLE spec for random addresses
    addr[5] |= 0xC0;  // Set bits 6 and 7 (0b11xxxxxx)
    
    // Check if BLE is initialized
    if (!NimBLEDevice::getInitialized()) {
        Serial.println("[BLE Spoof] BLE not initialized, cannot set address");
        return false;
    }
    
    // Use NimBLE native function to set random address
    int rc = ble_hs_id_set_rnd(addr);
    
    if (rc != 0) {
        Serial.printf("[BLE Spoof] Failed to set random address, error: %d\n", rc);
        return false;
    }
    
    // Set the address type to random
    NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM, false);
    
    // Update current profile
    currentProfile.address = address;
    currentProfile.addressType = BLE_OWN_ADDR_RANDOM;
    
    Serial.println("[BLE Spoof] Random BLE address set successfully");
    Serial.printf("[BLE Spoof] Address will be: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    
    return true;
}

bool BleSpoofModule::setSpoofAppearance(uint16_t appearance) {
    currentProfile.appearance = appearance;
    Serial.printf("[BLE Spoof] Appearance set to: 0x%04X\n", appearance);
    return true;
}

bool BleSpoofModule::setAdvertisementData(const String &hexData) {
    currentProfile.payload = hexData;
    currentProfile.payloadLength = hexData.length() / 2;
    Serial.printf("[BLE Spoof] Advertisement data set (%d bytes)\n", currentProfile.payloadLength);
    return true;
}

bool BleSpoofModule::startSpoofing(const BleDeviceProfile &profile) {
    Serial.println("[BLE Spoof] Starting spoofing...");
    
    // Stop any existing spoofing
    if (spoofingActive) {
        stopSpoofing();
    }
    
    // Save current profile
    currentProfile = profile;
    
    // Initialize BLE if not done
    if (!NimBLEDevice::getInitialized()) {
        NimBLEDevice::init(profile.name.c_str());
    } else {
        // Re-initialize with new name
        NimBLEDevice::deinit(false);
        NimBLEDevice::init(profile.name.c_str());
    }
    
    // Set random MAC address BEFORE starting advertising
    // This must be done after init() but before creating server/advertising
    if (!profile.address.isEmpty()) {
        Serial.println("[BLE Spoof] Setting spoofed MAC address...");
        if (!setSpoofAddress(profile.address)) {
            Serial.println("[BLE Spoof] Warning: Failed to set MAC address, continuing with hardware MAC");
        }
    }
    
    // Create server
    pServer = NimBLEDevice::createServer();
    if (!pServer) {
        Serial.println("[BLE Spoof] Failed to create server");
        return false;
    }
    
    // Get advertising instance
    pAdvertising = NimBLEDevice::getAdvertising();
    if (!pAdvertising) {
        Serial.println("[BLE Spoof] Failed to get advertising");
        return false;
    }
    
    // Set appearance
    pAdvertising->setAppearance(profile.appearance);
    
    // Set manufacturer data if available
    if (!profile.manufacturerData.isEmpty()) {
        std::vector<uint8_t> mfgData = hexStringToBytes(profile.manufacturerData);
        if (mfgData.size() >= 2) {
            // First 2 bytes are company ID (little endian)
            uint16_t companyId = mfgData[0] | (mfgData[1] << 8);
            std::string data((char*)&mfgData[2], mfgData.size() - 2);
            pAdvertising->setManufacturerData(data, companyId);
            Serial.printf("[BLE Spoof] Manufacturer data set (Company ID: 0x%04X)\n", companyId);
        }
    }
    
    // Add service UUIDs
    for (const String &uuidStr : profile.serviceUUIDs) {
        NimBLEUUID uuid(uuidStr.c_str());
        pAdvertising->addServiceUUID(uuid);
        Serial.printf("[BLE Spoof] Added service UUID: %s\n", uuidStr.c_str());
    }
    
    // Setup HID service (Phase 4) - BEFORE starting advertising
    Serial.println("[BLE Spoof] Setting up HID + Battery services...");
    if (setupHIDService() && setupBatteryService()) {
        hidEnabled = true;
        Serial.println("[BLE Spoof] HID + Battery services configured");
        // Add HID service UUID to advertising
        pAdvertising->addServiceUUID(NimBLEUUID((uint16_t)0x1812));  // HID Service
        pAdvertising->addServiceUUID(NimBLEUUID((uint16_t)0x180F));  // Battery Service
    } else {
        Serial.println("[BLE Spoof] Warning: Failed to setup HID services");
        hidEnabled = false;
    }
    
    // Configure security/pairing (Phase 4.4)
    NimBLEDevice::setSecurityAuth(false, false, true);  // No bonding, no MITM, SC only
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);  // Just Works pairing
    
    // Start advertising
    pAdvertising->start();
    spoofingActive = true;
    
    Serial.println("[BLE Spoof] Spoofing started successfully");
    Serial.printf("[BLE Spoof] Advertising as: %s\n", profile.name.c_str());
    Serial.printf("[BLE Spoof] Appearance: 0x%04X\n", profile.appearance);
    Serial.printf("[BLE Spoof] HID enabled: %s\n", hidEnabled ? "YES" : "NO");
    
    return true;
}

bool BleSpoofModule::stopSpoofing() {
    if (!spoofingActive) {
        return true;
    }
    
    Serial.println("[BLE Spoof] Stopping spoofing...");
    
    // Clean up HID (Phase 4)
    if (hidEnabled && pHID) {
        delete pHID;
        pHID = nullptr;
        pInputKeyboard = nullptr;
        pOutputKeyboard = nullptr;
        hidEnabled = false;
    }
    
    // Stop advertising
    if (pAdvertising) {
        pAdvertising->stop();
        pAdvertising = nullptr;
    }
    
    // Clean up server
    if (pServer) {
        pServer = nullptr;
    }
    
    // Deinit BLE
    if (NimBLEDevice::getInitialized()) {
        NimBLEDevice::deinit(true);
    }
    
    spoofingActive = false;
    currentProfile.clear();
    
    Serial.println("[BLE Spoof] Spoofing stopped");
    return true;
}

// ============================================================================
// HID Functions (Phase 4)
// ============================================================================

bool BleSpoofModule::setupHIDService() {
    if (!pServer) {
        Serial.println("[BLE Spoof] Server not initialized");
        return false;
    }
    
    Serial.println("[BLE Spoof] Setting up HID service...");
    
    // Create HID Device
    pHID = new NimBLEHIDDevice(pServer);
    if (!pHID) {
        Serial.println("[BLE Spoof] Failed to create HID device");
        return false;
    }
    
    // Set manufacturer
    pHID->manufacturer()->setValue("Bruce");
    
    // Set PNP ID (Vendor ID, Product ID, Version)
    pHID->pnp(0x02, 0x05ac, 0x820a, 0x0210);  // Apple-like IDs
    
    // Set HID info (country code, flags)
    pHID->hidInfo(0x00, 0x01);
    
    // Set report map
    pHID->reportMap((uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor));
    
    // Get characteristics
    pInputKeyboard = pHID->inputReport(KEYBOARD_ID);
    pOutputKeyboard = pHID->outputReport(KEYBOARD_ID);
    
    // Start HID service
    pHID->startServices();
    
    Serial.println("[BLE Spoof] HID service ready");
    return true;
}

bool BleSpoofModule::setupBatteryService() {
    if (!pServer) {
        Serial.println("[BLE Spoof] Server not initialized");
        return false;
    }
    
    Serial.println("[BLE Spoof] Setting up Battery service...");
    
    // Create Battery Service (UUID 0x180F)
    NimBLEService *pBattery = pServer->createService("180F");
    if (!pBattery) {
        Serial.println("[BLE Spoof] Failed to create Battery service");
        return false;
    }
    
    // Create Battery Level Characteristic (UUID 0x2A19)
    NimBLECharacteristic *pBatteryLevel = pBattery->createCharacteristic(
        "2A19",
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    
    if (!pBatteryLevel) {
        Serial.println("[BLE Spoof] Failed to create Battery Level characteristic");
        return false;
    }
    
    // Set initial battery level
    pBatteryLevel->setValue(&batteryLevel, 1);
    
    // Start service
    pBattery->start();
    
    Serial.println("[BLE Spoof] Battery service ready");
    return true;
}

bool BleSpoofModule::enableHID(bool enable) {
    if (!spoofingActive) {
        Serial.println("[BLE Spoof] Cannot enable HID: spoofing not active");
        return false;
    }
    
    if (enable && !hidEnabled) {
        if (setupHIDService() && setupBatteryService()) {
            hidEnabled = true;
            Serial.println("[BLE Spoof] HID enabled");
            return true;
        }
        return false;
    } else if (!enable && hidEnabled) {
        // Cleanup HID
        if (pHID) {
            delete pHID;
            pHID = nullptr;
        }
        pInputKeyboard = nullptr;
        pOutputKeyboard = nullptr;
        hidEnabled = false;
        Serial.println("[BLE Spoof] HID disabled");
        return true;
    }
    
    return true;
}

bool BleSpoofModule::sendKeyPress(uint8_t key, uint8_t modifiers) {
    if (!hidEnabled || !pInputKeyboard) {
        Serial.println("[BLE Spoof] HID not enabled");
        return false;
    }
    
    // HID Report: [modifiers, reserved, key1, key2, key3, key4, key5, key6]
    uint8_t report[8] = {modifiers, 0, key, 0, 0, 0, 0, 0};
    
    pInputKeyboard->setValue(report, sizeof(report));
    pInputKeyboard->notify();
    
    Serial.printf("[BLE Spoof] Key press sent: 0x%02X (mod: 0x%02X)\n", key, modifiers);
    return true;
}

bool BleSpoofModule::sendKeyRelease() {
    if (!hidEnabled || !pInputKeyboard) {
        Serial.println("[BLE Spoof] HID not enabled");
        return false;
    }
    
    // Send empty report (all keys released)
    uint8_t report[8] = {0};
    
    pInputKeyboard->setValue(report, sizeof(report));
    pInputKeyboard->notify();
    
    Serial.println("[BLE Spoof] All keys released");
    return true;
}

bool BleSpoofModule::sendString(const String &text) {
    if (!hidEnabled) {
        Serial.println("[BLE Spoof] HID not enabled");
        return false;
    }
    
    for (size_t i = 0; i < text.length(); i++) {
        char c = text[i];
        uint8_t key = 0;
        uint8_t modifiers = 0;
        
        // Simple ASCII to keycode conversion (US layout)
        if (c >= 'a' && c <= 'z') {
            key = 0x04 + (c - 'a');  // KEY_A = 0x04
        } else if (c >= 'A' && c <= 'Z') {
            key = 0x04 + (c - 'A');
            modifiers = 0x02;  // Left Shift
        } else if (c >= '1' && c <= '9') {
            key = 0x1E + (c - '1');  // KEY_1 = 0x1E
        } else if (c == '0') {
            key = 0x27;  // KEY_0 = 0x27
        } else if (c == ' ') {
            key = 0x2C;  // KEY_SPACE = 0x2C
        } else if (c == '\n') {
            key = 0x28;  // KEY_ENTER = 0x28
        }
        // Add more characters as needed
        
        if (key != 0) {
            sendKeyPress(key, modifiers);
            delay(20);  // Delay between key presses
            sendKeyRelease();
            delay(20);
        }
    }
    
    Serial.printf("[BLE Spoof] String sent: %s\n", text.c_str());
    return true;
}

bool BleSpoofModule::setBatteryLevel(uint8_t level) {
    if (level > 100) level = 100;
    batteryLevel = level;
    
    // Update characteristic if service is active
    if (spoofingActive && pServer) {
        NimBLEService *pBattery = pServer->getServiceByUUID("180F");
        if (pBattery) {
            NimBLECharacteristic *pBatteryLevel = pBattery->getCharacteristic("2A19");
            if (pBatteryLevel) {
                pBatteryLevel->setValue(&batteryLevel, 1);
                pBatteryLevel->notify();
                Serial.printf("[BLE Spoof] Battery level updated: %d%%\n", batteryLevel);
            }
        }
    }
    
    return true;
}

// ============================================================================
// DuckyScript Execution (Phase 6.3)
// ============================================================================

// DuckyScript command parsing structures (from ducky_typer.cpp)
enum DuckyCommandType {
    DuckyCommandType_Unknown,
    DuckyCommandType_Cmd,
    DuckyCommandType_Print,
    DuckyCommandType_Delay,
    DuckyCommandType_Comment,
    DuckyCommandType_Loop,
    DuckyCommandType_Combination
};

struct DuckyCommand {
    const char *command;
    char key;
    DuckyCommandType type;
};

struct DuckyCombination {
    const char *command;
    char key1;
    char key2;
    char key3;
};

// DuckyScript command tables (from ducky_typer.cpp)
const DuckyCombination duckyComb[]{
    {"CTRL-ALT",       KEY_LEFT_CTRL, KEY_LEFT_ALT,   0             },
    {"CTRL-SHIFT",     KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 0             },
    {"CTRL-GUI",       KEY_LEFT_CTRL, KEY_LEFT_GUI,   0             },
    {"CTRL-ESCAPE",    KEY_LEFT_CTRL, KEY_ESC,        0             },
    {"ALT-SHIFT",      KEY_LEFT_ALT,  KEY_LEFT_SHIFT, 0             },
    {"ALT-GUI",        KEY_LEFT_ALT,  KEY_LEFT_GUI,   0             },
    {"GUI-SHIFT",      KEY_LEFT_GUI,  KEY_LEFT_SHIFT, 0             },
    {"GUI-SPACE",      KEY_LEFT_GUI,  KEY_SPACE,      0             },
    {"CTRL-ALT-SHIFT", KEY_LEFT_CTRL, KEY_LEFT_ALT,   KEY_LEFT_SHIFT},
    {"CTRL-ALT-GUI",   KEY_LEFT_CTRL, KEY_LEFT_ALT,   KEY_LEFT_GUI  },
    {"ALT-SHIFT-GUI",  KEY_LEFT_ALT,  KEY_LEFT_SHIFT, KEY_LEFT_GUI  },
    {"CTRL-SHIFT-GUI", KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_LEFT_GUI  }
};

#define DEF_DELAY 100

const DuckyCommand duckyCmds[]{
    {"STRING",         0,                DuckyCommandType_Print      },
    {"STRINGLN",       0,                DuckyCommandType_Print      },
    {"REM",            0,                DuckyCommandType_Comment    },
    {"DELAY",          0,                DuckyCommandType_Delay      },
    {"DEFAULTDELAY",   DEF_DELAY,        DuckyCommandType_Delay      },
    {"REPEAT",         0,                DuckyCommandType_Loop       },
    {"CTRL-ALT",       0,                DuckyCommandType_Combination},
    {"CTRL-SHIFT",     0,                DuckyCommandType_Combination},
    {"CTRL-GUI",       0,                DuckyCommandType_Combination},
    {"CTRL-ESCAPE",    0,                DuckyCommandType_Combination},
    {"ALT-SHIFT",      0,                DuckyCommandType_Combination},
    {"ALT-GUI",        0,                DuckyCommandType_Combination},
    {"GUI-SHIFT",      0,                DuckyCommandType_Combination},
    {"GUI-SPACE",      0,                DuckyCommandType_Combination},
    {"CTRL-ALT-SHIFT", 0,                DuckyCommandType_Combination},
    {"CTRL-ALT-GUI",   0,                DuckyCommandType_Combination},
    {"ALT-SHIFT-GUI",  0,                DuckyCommandType_Combination},
    {"CTRL-SHIFT-GUI", 0,                DuckyCommandType_Combination},
    {"BACKSPACE",      KEYBACKSPACE,     DuckyCommandType_Cmd        },
    {"DELETE",         KEY_DELETE,       DuckyCommandType_Cmd        },
    {"ALT",            KEY_LEFT_ALT,     DuckyCommandType_Cmd        },
    {"CTRL",           KEY_LEFT_CTRL,    DuckyCommandType_Cmd        },
    {"GUI",            KEY_LEFT_GUI,     DuckyCommandType_Cmd        },
    {"SHIFT",          KEY_LEFT_SHIFT,   DuckyCommandType_Cmd        },
    {"ESCAPE",         KEY_ESC,          DuckyCommandType_Cmd        },
    {"TAB",            KEYTAB,           DuckyCommandType_Cmd        },
    {"ENTER",          KEY_RETURN,       DuckyCommandType_Cmd        },
    {"DOWNARROW",      KEY_DOWN_ARROW,   DuckyCommandType_Cmd        },
    {"DOWN",           KEY_DOWN_ARROW,   DuckyCommandType_Cmd        },
    {"LEFTARROW",      KEY_LEFT_ARROW,   DuckyCommandType_Cmd        },
    {"LEFT",           KEY_LEFT_ARROW,   DuckyCommandType_Cmd        },
    {"RIGHTARROW",     KEY_RIGHT_ARROW,  DuckyCommandType_Cmd        },
    {"RIGHT",          KEY_RIGHT_ARROW,  DuckyCommandType_Cmd        },
    {"UPARROW",        KEY_UP_ARROW,     DuckyCommandType_Cmd        },
    {"UP",             KEY_UP_ARROW,     DuckyCommandType_Cmd        },
    {"BREAK",          KEY_PAUSE,        DuckyCommandType_Cmd        },
    {"CAPSLOCK",       KEY_CAPS_LOCK,    DuckyCommandType_Cmd        },
    {"PAUSE",          KEY_PAUSE,        DuckyCommandType_Cmd        },
    {"END",            KEY_END,          DuckyCommandType_Cmd        },
    {"HOME",           KEY_HOME,         DuckyCommandType_Cmd        },
    {"INSERT",         KEY_INSERT,       DuckyCommandType_Cmd        },
    {"NUMLOCK",        LED_NUMLOCK,      DuckyCommandType_Cmd        },
    {"PAGEUP",         KEY_PAGE_UP,      DuckyCommandType_Cmd        },
    {"PAGEDOWN",       KEY_PAGE_DOWN,    DuckyCommandType_Cmd        },
    {"PRINTSCREEN",    KEY_PRINT_SCREEN, DuckyCommandType_Cmd        },
    {"SCROLLOCK",      KEY_SCROLL_LOCK,  DuckyCommandType_Cmd        },
    {"MENU",           KEY_MENU,         DuckyCommandType_Cmd        },
    {"F1",             KEY_F1,           DuckyCommandType_Cmd        },
    {"F2",             KEY_F2,           DuckyCommandType_Cmd        },
    {"F3",             KEY_F3,           DuckyCommandType_Cmd        },
    {"F4",             KEY_F4,           DuckyCommandType_Cmd        },
    {"F5",             KEY_F5,           DuckyCommandType_Cmd        },
    {"F6",             KEY_F6,           DuckyCommandType_Cmd        },
    {"F7",             KEY_F7,           DuckyCommandType_Cmd        },
    {"F8",             KEY_F8,           DuckyCommandType_Cmd        },
    {"F9",             KEY_F9,           DuckyCommandType_Cmd        },
    {"F10",            KEY_F10,          DuckyCommandType_Cmd        },
    {"F11",            KEY_F11,          DuckyCommandType_Cmd        },
    {"F12",            KEY_F12,          DuckyCommandType_Cmd        },
    {"SPACE",          KEY_SPACE,        DuckyCommandType_Cmd        }
};

// Helper function to send a key press via our HID module
void sendKey(uint8_t key) {
    if (key == 0) return;
    bleSpoofModule.sendKeyPress(key, 0);
    delay(20);
    bleSpoofModule.sendKeyRelease();
    delay(20);
}

// Helper function to send a string via our HID module
void sendDuckyString(const String &str) {
    bleSpoofModule.sendString(str);
}

// Helper function to send a string with newline
void sendDuckyStringLn(const String &str) {
    bleSpoofModule.sendString(str);
    sendKey(KEY_RETURN);
}

bool BleSpoofModule::executeDuckyScript(FS &fs, const String &filepath) {
    if (!hidEnabled) {
        Serial.println("[BLE Spoof] HID not enabled");
        return false;
    }
    
    if (!fs.exists(filepath) || filepath == "") {
        Serial.println("[BLE Spoof] DuckyScript file not found");
        return false;
    }
    
    File payloadFile = fs.open(filepath, "r");
    if (!payloadFile) {
        Serial.println("[BLE Spoof] Failed to open DuckyScript file");
        return false;
    }
    
    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorder();
    tft.setCursor(10, 30);
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor);
    tft.println("Executing DuckyScript...");
    tft.println();
    
    String lineContent = "";
    String Command = "";
    char Cmd[15];
    String Argument = "";
    String RepeatTmp = "";
    char ArgChar = '\0';
    bool ArgIsCmd;
    
    // Release all keys at start
    sendKeyRelease();
    
    while (payloadFile.available()) {
        previousMillis = millis(); // resets DimScreen
        
        // Allow pausing/canceling
        if (check(EscPress)) {
            tft.setTextColor(TFT_RED);
            tft.println("\nScript canceled!");
            break;
        }
        
        // Read line (handle CRLF)
        lineContent = payloadFile.readStringUntil('\n');
        if (lineContent.endsWith("\r")) lineContent.remove(lineContent.length() - 1);
        
        RepeatTmp = lineContent.substring(0, lineContent.indexOf(' '));
        RepeatTmp = RepeatTmp.c_str();
        
        if (RepeatTmp == "REPEAT") {
            if (lineContent.indexOf(' ') > 0) {
                RepeatTmp = lineContent.substring(lineContent.indexOf(' ') + 1);
                if (RepeatTmp.toInt() == 0) {
                    RepeatTmp = "1";
                    tft.setTextColor(ALCOLOR);
                    tft.println("REPEAT argument NaN, repeating once");
                }
            } else {
                RepeatTmp = "1";
                tft.setTextColor(ALCOLOR);
                tft.println("REPEAT without argument, repeating once");
            }
        } else {
            Command = lineContent.substring(0, lineContent.indexOf(' '));
            strcpy(Cmd, Command.c_str());
            if (lineContent.indexOf(' ') > 0)
                Argument = lineContent.substring(lineContent.indexOf(' ') + 1);
            else Argument = "";
            RepeatTmp = "1";
        }
        
        uint16_t i;
        ArgIsCmd = false;
        Argument = Argument.c_str();
        ArgChar = Argument.charAt(0);
        
        for (i = 0; i < RepeatTmp.toInt(); i++) {
            DuckyCommand *ArgCmd = nullptr;
            DuckyCommand *PriCmd = nullptr;
            ArgIsCmd = false;
            
            for (auto cmds : duckyCmds) {
                if (strcmp(Cmd, cmds.command) == 0) {
                    PriCmd = &cmds;
                    
                    // STRING and STRINGLN
                    if (cmds.type == DuckyCommandType_Print) {
                        if (strcmp(cmds.command, "STRINGLN") == 0) {
                            sendDuckyStringLn(Argument);
                        } else {
                            sendDuckyString(Argument);
                        }
                        break;
                    }
                    // DELAY and DEFAULTDELAY
                    else if (cmds.type == DuckyCommandType_Delay) {
                        if ((int)cmds.key > 0) delay(DEF_DELAY);
                        else if (Argument.toInt() > 0) delay(Argument.toInt());
                        else delay(DEF_DELAY);
                        break;
                    }
                    // Comment
                    else if (cmds.type == DuckyCommandType_Comment) {
                        yield();
                        break;
                    }
                    // Normal commands
                    else if (cmds.type == DuckyCommandType_Cmd) {
                        sendKeyPress(cmds.key, 0);
                        ArgIsCmd = true;
                    }
                    // Combinations
                    else if (cmds.type == DuckyCommandType_Combination) {
                        for (auto comb : duckyComb) {
                            if (strcmp(Cmd, comb.command) == 0) {
                                uint8_t modifiers = 0;
                                
                                // Build modifiers byte
                                if (comb.key1 == KEY_LEFT_CTRL) modifiers |= 0x01;
                                if (comb.key1 == KEY_LEFT_SHIFT) modifiers |= 0x02;
                                if (comb.key1 == KEY_LEFT_ALT) modifiers |= 0x04;
                                if (comb.key1 == KEY_LEFT_GUI) modifiers |= 0x08;
                                
                                if (comb.key2 == KEY_LEFT_CTRL) modifiers |= 0x01;
                                if (comb.key2 == KEY_LEFT_SHIFT) modifiers |= 0x02;
                                if (comb.key2 == KEY_LEFT_ALT) modifiers |= 0x04;
                                if (comb.key2 == KEY_LEFT_GUI) modifiers |= 0x08;
                                
                                if (comb.key3 == KEY_LEFT_CTRL) modifiers |= 0x01;
                                if (comb.key3 == KEY_LEFT_SHIFT) modifiers |= 0x02;
                                if (comb.key3 == KEY_LEFT_ALT) modifiers |= 0x04;
                                if (comb.key3 == KEY_LEFT_GUI) modifiers |= 0x08;
                                
                                // Find the non-modifier key
                                uint8_t regularKey = 0;
                                if (comb.key1 != KEY_LEFT_CTRL && comb.key1 != KEY_LEFT_SHIFT && 
                                    comb.key1 != KEY_LEFT_ALT && comb.key1 != KEY_LEFT_GUI) {
                                    regularKey = comb.key1;
                                }
                                if (comb.key2 != KEY_LEFT_CTRL && comb.key2 != KEY_LEFT_SHIFT && 
                                    comb.key2 != KEY_LEFT_ALT && comb.key2 != KEY_LEFT_GUI) {
                                    regularKey = comb.key2;
                                }
                                if (comb.key3 != 0 && comb.key3 != KEY_LEFT_CTRL && comb.key3 != KEY_LEFT_SHIFT && 
                                    comb.key3 != KEY_LEFT_ALT && comb.key3 != KEY_LEFT_GUI) {
                                    regularKey = comb.key3;
                                }
                                
                                sendKeyPress(regularKey, modifiers);
                                ArgIsCmd = true;
                            }
                        }
                    }
                }
                
                // Check if the Argument contains a command
                if (strcmp(Argument.c_str(), cmds.command) == 0) {
                    ArgCmd = &cmds;
                }
            }
            
            if (ArgCmd != nullptr && PriCmd != nullptr) {
                if (ArgCmd->type == DuckyCommandType_Cmd) {
                    sendKeyPress(ArgCmd->key, 0);
                }
            } else if (ArgIsCmd && PriCmd != nullptr) {
                if (ArgChar != '\0') {
                    sendKeyPress((uint8_t)ArgChar, 0);
                }
            }
            
            sendKeyRelease();
            
            if (PriCmd == nullptr) {
                tft.setTextColor(ALCOLOR);
                tft.print(Command);
                tft.println(" -> Not Supported");
            } else {
                tft.setTextColor(bruceConfig.priColor);
                tft.print(Command);
            }
            
            if (Argument.length() > 0 && Command != "REM") {
                tft.setTextColor(TFT_WHITE);
                tft.print(" ");
                tft.println(Argument);
            } else {
                tft.println();
            }
        }
    }
    
    payloadFile.close();
    sendKeyRelease();
    
    Serial.println("[BLE Spoof] DuckyScript execution completed");
    return true;
}

// ============================================================================
// UI Functions
// ============================================================================

void ble_spoof_scan_and_capture() {
    displayTextLine("Scanning BLE devices...");
    
    // Start scan for 5 seconds
    if (!bleSpoofModule.startScan(5)) {
        displayError("Scan failed", true);
        return;
    }
    
    // Get scanned devices
    std::vector<BleDeviceProfile> devices = bleSpoofModule.getScannedDevices();
    
    if (devices.empty()) {
        displayWarning("No devices found", true);
        return;
    }
    
    // Build options menu with scanned devices
    options.clear();
    for (size_t i = 0; i < devices.size() && i < 100; i++) {
        const BleDeviceProfile &profile = devices[i];
        String label = profile.name;
        if (label.isEmpty()) {
            label = profile.address;
        }
        label += " (" + String(profile.rssi) + "dBm)";
        
        options.push_back({label, [profile]() {
            // Ask user to choose filesystem
            options.clear();
            FS *selectedFs = nullptr;
            
            if (setupSdCard()) {
                options.push_back({"SD Card", [&selectedFs]() { selectedFs = &SD; }});
            }
            options.push_back({"LittleFS", [&selectedFs]() { selectedFs = &LittleFS; }});
            addOptionToMainMenu();
            
            loopOptions(options, MENU_TYPE_REGULAR, "Save to:");
            
            if (selectedFs == nullptr) {
                displayWarning("Canceled", true);
                return;
            }
            
            // Ask for filename
            String filename = keyboard("", 30, "Profile name:");
            if (filename.isEmpty()) {
                displayWarning("Canceled", true);
                return;
            }
            
            // Save profile
            if (bleSpoofModule.saveProfile(profile, *selectedFs, filename)) {
                displaySuccess("Profile saved!", true);
            } else {
                displayError("Save failed", true);
            }
        }});
    }
    
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_REGULAR, "Select device to save:");
    
    // Clear devices after use
    bleSpoofModule.clearScannedDevices();
}

void ble_spoof_load_profile() {
    // Choose filesystem
    options.clear();
    FS *selectedFs = nullptr;
    
    if (setupSdCard()) {
        options.push_back({"SD Card", [&selectedFs]() { selectedFs = &SD; }});
    }
    options.push_back({"LittleFS", [&selectedFs]() { selectedFs = &LittleFS; }});
    addOptionToMainMenu();
    
    loopOptions(options, MENU_TYPE_REGULAR, "Load from:");
    
    if (selectedFs == nullptr) {
        displayWarning("Canceled", true);
        return;
    }
    
    // List profiles
    std::vector<String> profiles = bleSpoofModule.listProfiles(*selectedFs);
    
    if (profiles.empty()) {
        displayWarning("No profiles found", true);
        return;
    }
    
    // Build options menu
    options.clear();
    for (const String &filepath : profiles) {
        String filename = filepath.substring(filepath.lastIndexOf('/') + 1);
        filename.replace(".json", "");
        
        options.push_back({filename, [filepath, selectedFs]() {
            BleDeviceProfile profile;
            if (bleSpoofModule.loadProfile(profile, *selectedFs, filepath)) {
                // Display profile info
                drawMainBorder();
                tft.setTextSize(1);
                tft.setCursor(10, 30);
                tft.printf("Name: %s\n", profile.name.c_str());
                tft.setCursor(10, 45);
                tft.printf("Address: %s\n", profile.address.c_str());
                tft.setCursor(10, 60);
                tft.printf("RSSI: %d dBm\n", profile.rssi);
                tft.setCursor(10, 75);
                tft.printf("Appearance: 0x%04X\n", profile.appearance);
                
                printCenterFootnote("Press any key");
                while (!check(AnyKeyPress)) {
                    yield();
                }
            } else {
                displayError("Load failed", true);
            }
        }});
    }
    
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_REGULAR, "Select profile:");
}

void ble_spoof_start_spoofing() {
    // Choose filesystem
    options.clear();
    FS *selectedFs = nullptr;
    
    if (setupSdCard()) {
        options.push_back({"SD Card", [&selectedFs]() { selectedFs = &SD; }});
    }
    options.push_back({"LittleFS", [&selectedFs]() { selectedFs = &LittleFS; }});
    addOptionToMainMenu();
    
    loopOptions(options, MENU_TYPE_REGULAR, "Load from:");
    
    if (selectedFs == nullptr) {
        displayWarning("Canceled", true);
        return;
    }
    
    // List profiles
    std::vector<String> profiles = bleSpoofModule.listProfiles(*selectedFs);
    
    if (profiles.empty()) {
        displayWarning("No profiles found", true);
        return;
    }
    
    // Build options menu
    options.clear();
    for (const String &filepath : profiles) {
        String filename = filepath.substring(filepath.lastIndexOf('/') + 1);
        filename.replace(".json", "");
        
        options.push_back({filename, [filepath, selectedFs]() {
            BleDeviceProfile profile;
            if (bleSpoofModule.loadProfile(profile, *selectedFs, filepath)) {
                // Start spoofing
                if (bleSpoofModule.startSpoofing(profile)) {
                    // Display status
                    drawMainBorder();
                    tft.setTextSize(1);
                    tft.setTextColor(TFT_GREEN);
                    tft.setCursor(10, 30);
                    tft.printf("Spoofing active!\n\n");
                    tft.setTextColor(bruceConfig.priColor);
                    tft.setCursor(10, 50);
                    tft.printf("Name: %s\n", profile.name.c_str());
                    tft.setCursor(10, 65);
                    tft.printf("Appearance: 0x%04X\n", profile.appearance);
                    tft.setCursor(10, 80);
                    tft.printf("Services: %d\n", profile.serviceUUIDs.size());
                    
                    printCenterFootnote("Press ESC to stop");
                    
                    while (!check(EscPress)) {
                        yield();
                    }
                    
                    bleSpoofModule.stopSpoofing();
                    displaySuccess("Spoofing stopped", true);
                } else {
                    displayError("Failed to start", true);
                }
            } else {
                displayError("Load failed", true);
            }
        }});
    }
    
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_REGULAR, "Select profile:");
}

void ble_spoof_stop_spoofing() {
    if (bleSpoofModule.isSpoofing()) {
        bleSpoofModule.stopSpoofing();
        displaySuccess("Spoofing stopped", true);
    } else {
        displayWarning("Not spoofing", true);
    }
}

void ble_spoof_execute_payload() {
    // Check if spoofing is active
    if (!bleSpoofModule.isSpoofing()) {
        displayWarning("Start spoofing first", true);
        return;
    }
    
    // Check if HID is enabled
    if (!bleSpoofModule.isHIDEnabled()) {
        displayWarning("HID not enabled", true);
        return;
    }
    
    // Choose filesystem
    options.clear();
    FS *selectedFs = nullptr;
    
    if (setupSdCard()) {
        options.push_back({"SD Card", [&selectedFs]() { selectedFs = &SD; }});
    }
    options.push_back({"LittleFS", [&selectedFs]() { selectedFs = &LittleFS; }});
    addOptionToMainMenu();
    
    loopOptions(options, MENU_TYPE_REGULAR, "Load payload from:");
    
    if (selectedFs == nullptr) {
        displayWarning("Canceled", true);
        return;
    }
    
    // Use file picker to select DuckyScript file
    String filepath = loopSD(*selectedFs, true, "*.txt");
    
    if (filepath.isEmpty()) {
        displayWarning("Canceled", true);
        return;
    }
    
    // Display confirmation
    drawMainBorder();
    tft.setTextSize(1);
    tft.setCursor(10, 30);
    tft.setTextColor(bruceConfig.priColor);
    tft.println("Execute payload?");
    tft.println();
    tft.setTextColor(TFT_WHITE);
    String filename = filepath.substring(filepath.lastIndexOf('/') + 1);
    tft.printf("File: %s\n", filename.c_str());
    tft.println();
    tft.println("Press SELECT to execute");
    tft.println("Press ESC to cancel");
    
    while (true) {
        if (check(SelPress)) {
            // Execute the script
            if (bleSpoofModule.executeDuckyScript(*selectedFs, filepath)) {
                displaySuccess("Payload executed!", true);
            } else {
                displayError("Execution failed", true);
            }
            break;
        }
        if (check(EscPress)) {
            displayWarning("Canceled", true);
            break;
        }
        yield();
    }
}

void ble_spoof_main_menu() {
    options.clear();
    options.push_back({"Scan & Capture", ble_spoof_scan_and_capture});
    options.push_back({"Load Profile", ble_spoof_load_profile});
    options.push_back({"Start Spoofing", ble_spoof_start_spoofing});
    if (bleSpoofModule.isSpoofing()) {
        options.push_back({"Stop Spoofing", ble_spoof_stop_spoofing});
        options.push_back({"Execute Payload", ble_spoof_execute_payload});
    }
    addOptionToMainMenu();
    
    loopOptions(options, MENU_TYPE_SUBMENU, "BLE Audio Spoof");
}
