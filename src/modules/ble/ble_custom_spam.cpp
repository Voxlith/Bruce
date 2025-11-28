#include "ble_custom_spam.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include <globals.h>

// Bluetooth maximum transmit power (reuse from ble_spam.cpp)
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32S3)
#define MAX_TX_POWER ESP_PWR_LVL_P21
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32C6)
#define MAX_TX_POWER ESP_PWR_LVL_P20
#else
#define MAX_TX_POWER ESP_PWR_LVL_P9
#endif

BLECustomSpam::BLECustomSpam() : pAdvertising(nullptr), spamActive(false), macRandomizationEnabled(true) {
    currentPayload.isValid = false;
}

BLECustomSpam::~BLECustomSpam() {
    stopSpam();
}

std::vector<uint8_t> BLECustomSpam::hexToBytes(const String& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        String byteString = hex.substring(i, i + 2);
        uint8_t byte = (uint8_t)strtol(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

bool BLECustomSpam::parsePayload(JsonDocument& doc) {
    currentPayload.isValid = false;
    
    // Required fields
    if (!doc["name"].is<String>()) {
        Serial.println("Missing 'name' field");
        return false;
    }
    currentPayload.name = doc["name"].as<String>();
    
    if (!doc["type"].is<String>()) {
        Serial.println("Missing 'type' field");
        return false;
    }
    currentPayload.type = doc["type"].as<String>();
    
    // Get MAC address if available
    if (doc["address"].is<String>()) {
        currentPayload.address = doc["address"].as<String>();
        Serial.println("Loaded MAC address: " + currentPayload.address);
    } else {
        currentPayload.address = "";
    }
    
    // Optional adv_data fields - Use new ArduinoJson API
    if (doc["adv_data"].is<JsonObject>()) {
        JsonObject advData = doc["adv_data"];
        
        // Flags
        if (advData["flags"].is<String>()) {
            String flagsStr = advData["flags"].as<String>();
            if (flagsStr.startsWith("0x")) {
                flagsStr = flagsStr.substring(2);
            }
            currentPayload.flags = (uint8_t)strtol(flagsStr.c_str(), nullptr, 16);
        } else {
            currentPayload.flags = 0x06; // Default: General Discoverable + BR/EDR Not Supported
        }
        
        // Complete name
        if (advData["complete_name"].is<String>()) {
            currentPayload.completeName = advData["complete_name"].as<String>();
        }
        
        // TX Power
        if (advData["tx_power"].is<int>()) {
            currentPayload.txPower = advData["tx_power"].as<int8_t>();
        } else {
            currentPayload.txPower = 0;
        }
        
        // Manufacturer data
        if (advData["manufacturer_data"].is<JsonObject>()) {
            JsonObject mfgData = advData["manufacturer_data"];
            if (mfgData["company_id"].is<String>()) {
                String companyIdStr = mfgData["company_id"].as<String>();
                if (companyIdStr.startsWith("0x")) {
                    companyIdStr = companyIdStr.substring(2);
                }
                currentPayload.manufacturerId = (uint16_t)strtol(companyIdStr.c_str(), nullptr, 16);
            }
            
            if (mfgData["data"].is<String>()) {
                String dataStr = mfgData["data"].as<String>();
                currentPayload.manufacturerData = hexToBytes(dataStr);
            }
        }
        
        // Service UUIDs
        if (advData["service_uuids"].is<JsonArray>()) {
            JsonArray uuids = advData["service_uuids"];
            for (JsonVariant uuid : uuids) {
                currentPayload.serviceUUIDs.push_back(uuid.as<String>());
            }
        }
    }
    
    // Raw advertisement data (if present, use this instead)
    if (doc["raw_adv_data"].is<String>()) {
        String rawStr = doc["raw_adv_data"].as<String>();
        currentPayload.rawAdvData = hexToBytes(rawStr);
        // Backup original data for MAC randomization
        originalRawAdvData = currentPayload.rawAdvData;
    }
    
    // Raw Scan Response data (if present)
    if (doc["raw_scan_rsp"].is<String>()) {
        String rawStr = doc["raw_scan_rsp"].as<String>();
        currentPayload.rawScanRsp = hexToBytes(rawStr);
        Serial.println("Loaded Scan Response data: " + String(currentPayload.rawScanRsp.size()) + " bytes");
        // Backup original scan response for MAC randomization
        originalRawScanRsp = currentPayload.rawScanRsp;
    }
    
    currentPayload.isValid = true;
    return true;
}

bool BLECustomSpam::loadPayload(const String& filepath) {
    // Determine file system
    FS* fs = nullptr;
    if (filepath.startsWith("/BruceSD/") && sdcardMounted) {
        fs = &SD;
    } else if (LittleFS.begin()) {
        fs = &LittleFS;
    } else {
        Serial.println("No file system available");
        return false;
    }
    
    // Open file
    File file = fs->open(filepath, FILE_READ);
    if (!file) {
        Serial.println("Failed to open payload file: " + filepath);
        return false;
    }
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.println("Failed to parse JSON: " + String(error.c_str()));
        return false;
    }
    
    // Parse payload data
    return parsePayload(doc);
}

bool BLECustomSpam::loadPayloadFromString(const String& jsonStr) {
    if (jsonStr.isEmpty()) {
        Serial.println("ERROR: JSON string is empty");
        return false;
    }
    
    Serial.println("Loading payload from JSON string...");
    Serial.println("JSON length: " + String(jsonStr.length()) + " bytes");
    
    // Parse JSON
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Use existing parsePayload method
    bool success = parsePayload(doc);
    
    if (success) {
        Serial.println("✓ Payload loaded from string successfully");
        
        // CRITICAL: Backup original data for MAC randomization
        originalRawAdvData = currentPayload.rawAdvData;
        originalRawScanRsp = currentPayload.rawScanRsp;
        
        Serial.println("✓ Original data backed up");
        Serial.print("  - rawAdvData size: ");
        Serial.println(originalRawAdvData.size());
        Serial.print("  - rawScanRsp size: ");
        Serial.println(originalRawScanRsp.size());
    } else {
        Serial.println("✗ Failed to parse payload from string");
    }
    
    return success;
}

String BLECustomSpam::getPayloadInfo() {
    if (!currentPayload.isValid) {
        return "No payload loaded";
    }
    
    String info = "Name: " + currentPayload.name + "\n";
    info += "Type: " + currentPayload.type + "\n";
    
    if (!currentPayload.address.isEmpty()) {
        info += "M: " + currentPayload.address + "\n";
    }
    
    if (!currentPayload.completeName.isEmpty()) {
        info += "BLE: " + currentPayload.completeName + "\n";
    }
    
    if (!currentPayload.manufacturerData.empty()) {
        info += "Mfg: 0x" + String(currentPayload.manufacturerId, HEX) + "\n";
        info += "Data: " + String(currentPayload.manufacturerData.size()) + "B\n";
    }
    
    if (!currentPayload.serviceUUIDs.empty()) {
        info += "Svc: " + String(currentPayload.serviceUUIDs.size()) + "\n";
    }
    
    if (!currentPayload.rawAdvData.empty()) {
        info += "Raw: " + String(currentPayload.rawAdvData.size()) + "B\n";
    }
    
    if (!currentPayload.rawScanRsp.empty()) {
        info += "Scan: " + String(currentPayload.rawScanRsp.size()) + "B\n";
    }
    
    return info;
}

void BLECustomSpam::displayPayloadInfo() {
    if (!currentPayload.isValid) {
        padprintln("No payload loaded");
        return;
    }
    
    padprintln("Name: " + currentPayload.name);
    padprintln("Type: " + currentPayload.type);
    
    if (!currentPayload.address.isEmpty()) {
        padprintln("M: " + currentPayload.address);
    }
    
    if (!currentPayload.completeName.isEmpty()) {
        padprintln("BLE: " + currentPayload.completeName);
    }
    
    if (!currentPayload.manufacturerData.empty()) {
        padprintln("Mfg: 0x" + String(currentPayload.manufacturerId, HEX));
        padprintln("Data: " + String(currentPayload.manufacturerData.size()) + "B");
    }
    
    if (!currentPayload.serviceUUIDs.empty()) {
        padprintln("Svc: " + String(currentPayload.serviceUUIDs.size()));
    }
    
    if (!currentPayload.rawAdvData.empty()) {
        padprintln("Raw: " + String(currentPayload.rawAdvData.size()) + "B");
    }
    
    if (!currentPayload.rawScanRsp.empty()) {
        padprintln("Scan: " + String(currentPayload.rawScanRsp.size()) + "B");
    }
}

bool BLECustomSpam::configureAdvertising() {
    if (!currentPayload.isValid) {
        return false;
    }
    
    // Initialize BLE if needed
    if (!NimBLEDevice::getInitialized()) {
        // Set MAC address from payload if available, otherwise random
        uint8_t macAddr[6];
        if (!currentPayload.address.isEmpty()) {
            // Parse MAC address from string (format: "AA:BB:CC:DD:EE:FF")
            String mac = currentPayload.address;
            mac.replace(":", "");
            for (int i = 0; i < 6; i++) {
                String byteStr = mac.substring(i * 2, i * 2 + 2);
                macAddr[i] = (uint8_t)strtol(byteStr.c_str(), nullptr, 16);
            }
            Serial.print("Using MAC from payload: ");
            for (int i = 0; i < 6; i++) {
                if (macAddr[i] < 0x10) Serial.print("0");
                Serial.print(macAddr[i], HEX);
                if (i < 5) Serial.print(":");
            }
            Serial.println();
        } else {
            // Generate random MAC for spam
            for (int i = 0; i < 6; i++) {
                macAddr[i] = esp_random() & 0xFF;
            }
            Serial.println("Using random MAC (no address in payload)");
        }
        esp_base_mac_addr_set(macAddr);
        
        NimBLEDevice::init("");
        vTaskDelay(10 / portTICK_PERIOD_MS);
        
        // Set max TX power
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
    }
    
    // Get advertising object
    pAdvertising = NimBLEDevice::getAdvertising();
    
    // Use raw data if available
    if (!currentPayload.rawAdvData.empty()) {
        // Create advertisement data object
        BLEAdvertisementData advertisementData;
        
        // CRITICAL: BLE Advertisement max size is 31 bytes!
        // If raw data > 31 bytes, split into Advertisement (31 bytes) + Scan Response (rest)
        size_t totalSize = currentPayload.rawAdvData.size();
        size_t advSize = (totalSize > 31) ? 31 : totalSize;
        
        Serial.print("Total raw data size: ");
        Serial.print(totalSize);
        Serial.println(" bytes");
        
        // Advertisement Data (max 31 bytes, includes flags)
        std::string rawData(
            (char*)currentPayload.rawAdvData.data(),
            advSize
        );
        
        Serial.print("Advertisement data (");
        Serial.print(advSize);
        Serial.print(" bytes): ");
        for (size_t i = 0; i < advSize; i++) {
            if ((uint8_t)rawData[i] < 0x10) Serial.print("0");
            Serial.print((uint8_t)rawData[i], HEX);
        }
        Serial.println();
        
        advertisementData.addData(rawData);
        pAdvertising->setAdvertisementData(advertisementData);
        
        // If data > 31 bytes, put remaining in Scan Response
        if (totalSize > 31) {
            size_t scanSize = totalSize - 31;
            if (scanSize > 31) {
                scanSize = 31; // Scan response also limited to 31 bytes
                Serial.print("WARNING: Data truncated! Total: ");
                Serial.print(totalSize);
                Serial.println(" bytes, max 62 bytes (31 adv + 31 scan)");
            }
            
            BLEAdvertisementData scanResponseData;
            std::string scanData(
                (char*)currentPayload.rawAdvData.data() + 31,
                scanSize
            );
            
            Serial.print("Scan Response data (");
            Serial.print(scanSize);
            Serial.print(" bytes): ");
            for (size_t i = 0; i < scanSize; i++) {
                if ((uint8_t)scanData[i] < 0x10) Serial.print("0");
                Serial.print((uint8_t)scanData[i], HEX);
            }
            Serial.println();
            
            scanResponseData.addData(scanData);
            pAdvertising->setScanResponseData(scanResponseData);
            Serial.println("Data split: Advertisement (31 bytes) + Scan Response (" + String(scanSize) + " bytes)");
            return true; // Exit early, scan response already set
        }
        
        Serial.println("Advertisement data set (fits in 31 bytes)");
    } else {
        // Build advertisement from fields
        BLEAdvertisementData advertisementData;
        
        // Flags
        advertisementData.setFlags(currentPayload.flags);
        
        // Name
        if (!currentPayload.completeName.isEmpty()) {
            advertisementData.setName(currentPayload.completeName.c_str());
        } else if (!currentPayload.name.isEmpty()) {
            advertisementData.setName(currentPayload.name.c_str());
        }
        
        // Manufacturer data
        if (!currentPayload.manufacturerData.empty()) {
            std::string mfgData;
            // Add company ID (little endian)
            mfgData += (char)(currentPayload.manufacturerId & 0xFF);
            mfgData += (char)((currentPayload.manufacturerId >> 8) & 0xFF);
            // Add manufacturer data
            for (uint8_t byte : currentPayload.manufacturerData) {
                mfgData += (char)byte;
            }
            advertisementData.setManufacturerData(mfgData);
        }
        
        // Service UUIDs
        for (const String& uuidStr : currentPayload.serviceUUIDs) {
            pAdvertising->addServiceUUID(NimBLEUUID(uuidStr.c_str()));
        }
        
        pAdvertising->setAdvertisementData(advertisementData);
    }
    
    // Set Scan Response data (contains the device name!)
    BLEAdvertisementData scanResponseData;
    if (!currentPayload.rawScanRsp.empty()) {
        // Use raw scan response data if available
        std::string rawScanData((char*)currentPayload.rawScanRsp.data(), currentPayload.rawScanRsp.size());
        
        Serial.print("Raw scan response to send (");
        Serial.print(currentPayload.rawScanRsp.size());
        Serial.print(" bytes): ");
        for (uint8_t byte : currentPayload.rawScanRsp) {
            if (byte < 0x10) Serial.print("0");
            Serial.print(byte, HEX);
        }
        Serial.println();
        
        scanResponseData.addData(rawScanData);
        Serial.println("Scan response data set with raw payload");
    } else if (!currentPayload.completeName.isEmpty() && currentPayload.completeName != "Unknown") {
        // Fallback: manually create scan response with name
        scanResponseData.setName(currentPayload.completeName.c_str());
        Serial.println("Setting scan response with name: " + currentPayload.completeName);
    }
    pAdvertising->setScanResponseData(scanResponseData);
    
    return true;
}

bool BLECustomSpam::startSpam(uint32_t intervalMs) {
    if (!currentPayload.isValid) {
        Serial.println("No valid payload loaded");
        return false;
    }
    
    if (!configureAdvertising()) {
        Serial.println("Failed to configure advertising");
        return false;
    }
    
    spamActive = true;
    return true;
}

// Helper function: Parse MAC address from String to uint8_t array
void BLECustomSpam::parseMacFromString(const String& macStr, uint8_t* mac) {
    if (macStr.isEmpty() || macStr.length() < 17 || !mac) {
        memset(mac, 0, 6);
        return;
    }
    
    String cleanMac = macStr;
    cleanMac.replace(":", "");
    
    for (int i = 0; i < 6; i++) {
        String byteStr = cleanMac.substring(i * 2, i * 2 + 2);
        mac[i] = (uint8_t)strtol(byteStr.c_str(), nullptr, 16);
    }
}

// Generate smart random MAC address (keeps OUI if available)
void BLECustomSpam::generateSmartRandomMac(uint8_t *newMac) {
    if (!newMac) return;
    
    if (!currentPayload.address.isEmpty() && currentPayload.address.length() >= 17) {
        // Parse 3 premiers bytes (OUI) de la MAC originale
        String mac = currentPayload.address;
        mac.replace(":", "");
        for (int i = 0; i < 3; i++) {
            String byteStr = mac.substring(i * 2, i * 2 + 2);
            newMac[i] = (uint8_t)strtol(byteStr.c_str(), nullptr, 16);
        }
        Serial.print("Using OUI from payload: ");
        for (int i = 0; i < 3; i++) {
            if (newMac[i] < 0x10) Serial.print("0");
            Serial.print(newMac[i], HEX);
            if (i < 2) Serial.print(":");
        }
        
        // Randomiser 3 derniers bytes
        for (int i = 3; i < 6; i++) {
            newMac[i] = esp_random() & 0xFF;
        }
        Serial.print(":");
        for (int i = 3; i < 6; i++) {
            if (newMac[i] < 0x10) Serial.print("0");
            Serial.print(newMac[i], HEX);
            if (i < 5) Serial.print(":");
        }
        Serial.println(" (randomized)");
    } else {
        // Fallback: MAC complètement aléatoire
        for (int i = 0; i < 6; i++) {
            newMac[i] = esp_random() & 0xFF;
        }
        // Set locally administered bit
        newMac[0] = (newMac[0] & 0xFC) | 0x02;
        Serial.print("Using fully random MAC (locally administered): ");
        for (int i = 0; i < 6; i++) {
            if (newMac[i] < 0x10) Serial.print("0");
            Serial.print(newMac[i], HEX);
            if (i < 5) Serial.print(":");
        }
        Serial.println();
    }
}

void BLECustomSpam::stopSpam() {
    spamActive = false;
    
    if (pAdvertising) {
        pAdvertising->stop();
    }
    
    if (NimBLEDevice::getInitialized()) {
        NimBLEDevice::deinit();
    }
    
    pAdvertising = nullptr;
}

bool BLECustomSpam::isSpamming() {
    return spamActive;
}

// Re-set advertising data (NimBLE loses data after stop())
void BLECustomSpam::resetAdvertisingData() {
    if (!pAdvertising || !currentPayload.isValid) {
        return;
    }
    
    // Re-set advertisement data
    if (!currentPayload.rawAdvData.empty()) {
        BLEAdvertisementData advertisementData;
        
        // Split data if > 31 bytes (same logic as configureAdvertising)
        size_t totalSize = currentPayload.rawAdvData.size();
        size_t advSize = (totalSize > 31) ? 31 : totalSize;
        
        std::string rawData(
            (char*)currentPayload.rawAdvData.data(),
            advSize
        );
        advertisementData.addData(rawData);
        pAdvertising->setAdvertisementData(advertisementData);
        
        // If data > 31 bytes, put remaining in Scan Response
        if (totalSize > 31) {
            size_t scanSize = totalSize - 31;
            if (scanSize > 31) scanSize = 31;
            
            BLEAdvertisementData scanResponseData;
            std::string scanData(
                (char*)currentPayload.rawAdvData.data() + 31,
                scanSize
            );
            scanResponseData.addData(scanData);
            pAdvertising->setScanResponseData(scanResponseData);
            return; // Exit early, scan response already set
        }
    } else {
        // Build from fields
        BLEAdvertisementData advertisementData;
        advertisementData.setFlags(currentPayload.flags);
        if (!currentPayload.completeName.isEmpty()) {
            advertisementData.setName(currentPayload.completeName.c_str());
        }
        if (!currentPayload.manufacturerData.empty()) {
            std::string mfgData;
            mfgData += (char)(currentPayload.manufacturerId & 0xFF);
            mfgData += (char)((currentPayload.manufacturerId >> 8) & 0xFF);
            for (uint8_t byte : currentPayload.manufacturerData) {
                mfgData += (char)byte;
            }
            advertisementData.setManufacturerData(mfgData);
        }
        pAdvertising->setAdvertisementData(advertisementData);
    }
    
    // Re-set scan response data
    BLEAdvertisementData scanResponseData;
    if (!currentPayload.rawScanRsp.empty()) {
        std::string rawScanData((char*)currentPayload.rawScanRsp.data(), currentPayload.rawScanRsp.size());
        scanResponseData.addData(rawScanData);
    } else if (!currentPayload.completeName.isEmpty() && currentPayload.completeName != "Unknown") {
        scanResponseData.setName(currentPayload.completeName.c_str());
    }
    pAdvertising->setScanResponseData(scanResponseData);
}

// Replace last 3 bytes of MAC in raw data (smarter approach)
void BLECustomSpam::replaceLastThreeMacBytes(std::vector<uint8_t>& data, const uint8_t* oldLast3, const uint8_t* newLast3) {
    if (data.empty() || !oldLast3 || !newLast3) {
        Serial.println("ERROR: replaceLastThreeMacBytes - invalid parameters");
        return;
    }
    
    Serial.println("=== replaceLastThreeMacBytes DEBUG ===");
    Serial.print("Data size: ");
    Serial.println(data.size());
    
    Serial.print("Old last 3 bytes (BE): ");
    for (int i = 0; i < 3; i++) {
        if (oldLast3[i] < 0x10) Serial.print("0");
        Serial.print(oldLast3[i], HEX);
        if (i < 2) Serial.print(" ");
    }
    Serial.println();
    
    Serial.print("New last 3 bytes (BE): ");
    for (int i = 0; i < 3; i++) {
        if (newLast3[i] < 0x10) Serial.print("0");
        Serial.print(newLast3[i], HEX);
        if (i < 2) Serial.print(" ");
    }
    Serial.println();
    
    // Prepare little-endian version (reversed)
    uint8_t oldLast3LE[3], newLast3LE[3];
    for (int i = 0; i < 3; i++) {
        oldLast3LE[i] = oldLast3[2 - i];
        newLast3LE[i] = newLast3[2 - i];
    }
    
    Serial.print("Old last 3 bytes (LE): ");
    for (int i = 0; i < 3; i++) {
        if (oldLast3LE[i] < 0x10) Serial.print("0");
        Serial.print(oldLast3LE[i], HEX);
        if (i < 2) Serial.print(" ");
    }
    Serial.println();
    
    // Print raw data to search
    Serial.print("Raw data to search: ");
    for (size_t i = 0; i < data.size() && i < 50; i++) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
    }
    Serial.println();
    
    int replacements = 0;
    
    // Search for 3-byte patterns (both big-endian and little-endian)
    for (size_t i = 0; i <= data.size() - 3; i++) {
        // Check little-endian match (most common in BLE)
        if (memcmp(&data[i], oldLast3LE, 3) == 0) {
            Serial.printf("Found LE match at position %d\n", i);
            memcpy(&data[i], newLast3LE, 3);
            replacements++;
        }
        // Check big-endian match
        else if (memcmp(&data[i], oldLast3, 3) == 0) {
            Serial.printf("Found BE match at position %d\n", i);
            memcpy(&data[i], newLast3, 3);
            replacements++;
        }
    }
    
    if (replacements > 0) {
        Serial.printf("✓ Total replacements: %d\n", replacements);
        Serial.print("Raw data AFTER replacement: ");
        for (size_t i = 0; i < data.size() && i < 50; i++) {
            if (data[i] < 0x10) Serial.print("0");
            Serial.print(data[i], HEX);
        }
        Serial.println();
    } else {
        Serial.println("✗ No matches found - raw_adv_data unchanged");
    }
    Serial.println("======================================");
}

// MAC randomization control
void BLECustomSpam::setMacRandomization(bool enabled) {
    macRandomizationEnabled = enabled;
    Serial.println("MAC randomization: " + String(enabled ? "ON" : "OFF"));
}

bool BLECustomSpam::getMacRandomization() const {
    return macRandomizationEnabled;
}

// Public method to perform one spam cycle
void BLECustomSpam::doSpamCycle() {
    if (spamActive) {
        uint8_t macToUse[6];
        uint8_t oldMac[6];
        
        // Parse original MAC
        parseMacFromString(currentPayload.address, oldMac);
        
        if (macRandomizationEnabled) {
            Serial.println("\n*** Random MAC ENABLED - Starting cycle ***");
            
            // Restore original raw data
            if (!originalRawAdvData.empty()) {
                currentPayload.rawAdvData = originalRawAdvData;
                Serial.println("✓ Restored original rawAdvData");
            } else {
                Serial.println("✗ originalRawAdvData is EMPTY!");
            }
            if (!originalRawScanRsp.empty()) {
                currentPayload.rawScanRsp = originalRawScanRsp;
                Serial.println("✓ Restored original rawScanRsp");
            }
            
            // Print original MAC
            Serial.print("Original MAC: ");
            for (int i = 0; i < 6; i++) {
                if (oldMac[i] < 0x10) Serial.print("0");
                Serial.print(oldMac[i], HEX);
                if (i < 5) Serial.print(":");
            }
            Serial.println();
            
            // Generate smart random MAC (keeps OUI, randomizes last 3 bytes)
            generateSmartRandomMac(macToUse);
            
            Serial.print("New Random MAC: ");
            for (int i = 0; i < 6; i++) {
                if (macToUse[i] < 0x10) Serial.print("0");
                Serial.print(macToUse[i], HEX);
                if (i < 5) Serial.print(":");
            }
            Serial.println();
            
            // Replace last 3 bytes of MAC in raw_adv_data (if found)
            if (!currentPayload.rawAdvData.empty()) {
                Serial.println("Attempting to replace in rawAdvData...");
                replaceLastThreeMacBytes(currentPayload.rawAdvData, &oldMac[3], &macToUse[3]);
            } else {
                Serial.println("✗ currentPayload.rawAdvData is EMPTY!");
            }
            
            // Replace last 3 bytes of MAC in raw_scan_rsp (if found)
            if (!currentPayload.rawScanRsp.empty()) {
                Serial.println("Attempting to replace in rawScanRsp...");
                replaceLastThreeMacBytes(currentPayload.rawScanRsp, &oldMac[3], &macToUse[3]);
            }
        } else {
            // Use original MAC from payload
            memcpy(macToUse, oldMac, 6);
            Serial.println("Using original MAC (randomization disabled)");
        }
        
        // CRITICAL: Deinit BLE to apply new MAC
        if (NimBLEDevice::getInitialized()) {
            NimBLEDevice::deinit();
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        
        // Set MAC address
        esp_base_mac_addr_set(macToUse);
        
        // Reinit BLE with new MAC
        NimBLEDevice::init("");
        vTaskDelay(10 / portTICK_PERIOD_MS);
        
        // Set max TX power
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
        
        // Get advertising object (new instance after reinit)
        pAdvertising = NimBLEDevice::getAdvertising();
        
        // Configure advertising with modified data
        resetAdvertisingData();
        
        // Spam cycle
        pAdvertising->start();
        vTaskDelay(200 / portTICK_PERIOD_MS);  // 200ms broadcast for better detection
        pAdvertising->stop();
        vTaskDelay(10 / portTICK_PERIOD_MS);   // Small delay after stop
        
        // DON'T deinit here - will be done at next cycle
    }
}

// Main UI function
void ble_custom_payload() {
    BLECustomSpam spammer;
    bool payloadLoaded = false;
    bool spamming = false;
    int spamCount = 0;
    unsigned long lastSpamTime = 0;
    const uint32_t SPAM_INTERVAL = 100; // ms between advertisements
    
    drawMainBorderWithTitle("BLE Custom Payload");
    padprintln("");
    padprintln("Select payload file...");
    padprintln("");
    padprintln("Press SELECT to browse");
    padprintln("Press ESC to exit");
    
    // Wait for file selection
    while (true) {
        if (check(EscPress)) {
            returnToMenu = true;
            return;
        }
        
        if (check(SelPress)) {
            // Browse for payload file
            FS* fs = sdcardMounted ? (FS*)&SD : (FS*)&LittleFS;
            // Fix: use "JSON" instead of "*.json" for extension filter
            String basePath = sdcardMounted ? "/BruceSD/ble/payloads" : "/ble/payloads";
            String selectedFile = loopSD(*fs, true, "JSON", basePath);
            
            if (selectedFile.isEmpty() || selectedFile == "/") {
                // User cancelled
                displayTextLine("Cancelled");
                delay(1000);
                returnToMenu = true;
                return;
            }
            
            // Load the payload
            displayTextLine("Loading...");
            if (spammer.loadPayload(selectedFile)) {
                payloadLoaded = true;
                displayTextLine("Loaded!");
                delay(1000);
                break;
            } else {
                displayTextLine("Load failed!");
                delay(2000);
                returnToMenu = true;
                return;
            }
        }
        
        delay(100);
    }
    
    // Display payload info with config menu
    if (payloadLoaded) {
        bool macRandomEnabled = true; // Default ON
        
        // Show payload info
        drawMainBorderWithTitle("PAYLOAD LOADED");
        padprintln("");
        spammer.displayPayloadInfo();
        padprintln("");
        padprintln("SELECT:Config ESC:Back");
        
        // Wait for user input
        while (true) {
            if (check(EscPress)) {
                returnToMenu = true;
                return;
            }
            
            if (check(SelPress)) {
                // Show config menu
                options = {
                    {
                        macRandomEnabled ? "[X] Random MAC" : "[ ] Random MAC",
                        [&macRandomEnabled]() {
                            macRandomEnabled = !macRandomEnabled;
                            displayTextLine(macRandomEnabled ? "Random MAC: ON" : "Random MAC: OFF");
                            delay(500);
                        }
                    },
                    {
                        "Start Spam",
                        [&]() {
                            spammer.setMacRandomization(macRandomEnabled);
                            if (spammer.startSpam(SPAM_INTERVAL)) {
                                spamming = true;
                                spamCount = 0;
                                lastSpamTime = millis();
                            } else {
                                displayTextLine("Start failed!");
                                delay(2000);
                                returnToMenu = true;
                            }
                        }
                    },
                    {
                        "Back",
                        []() {}
                    }
                };
                
                loopOptions(options);
                
                if (spamming) {
                    break; // Exit to spam loop
                }
                if (returnToMenu) {
                    return;
                }
                
                // Redraw payload info after menu
                drawMainBorderWithTitle("PAYLOAD LOADED");
                padprintln("");
                spammer.displayPayloadInfo();
                padprintln("");
                padprintln("SELECT:Config ESC:Back");
            }
            
            delay(100);
        }
    }
    
    // Spam loop
    while (spamming) {
        if (millis() - lastSpamTime >= SPAM_INTERVAL) {
            // DON'T change MAC for custom payload - use the original MAC from the payload
            // Changing MAC might break device recognition
            // uint8_t macAddr[6];
            // for (int i = 0; i < 6; i++) {
            //     macAddr[i] = esp_random() & 0xFF;
            // }
            // esp_base_mac_addr_set(macAddr);
            
            // Use public method to spam
            spammer.doSpamCycle();
            
            spamCount++;
            lastSpamTime = millis();
            
            // Update display every 5 spams for more feedback
            if (spamCount % 5 == 0) {
                drawMainBorderWithTitle("Spamming... (" + String(spamCount) + ")");
                padprintln("");
                padprintln("Payload: " + spammer.getPayload().name);
                padprintln("");
                padprintln("Press ESC to stop");
            }
        }
        
        if (check(EscPress)) {
            spammer.stopSpam();
            spamming = false;
            break;
        }
        
        delay(10);
    }
    
    displayTextLine("Stopped. Count: " + String(spamCount));
    delay(2000);
    returnToMenu = true;
}
