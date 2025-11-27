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

BLECustomSpam::BLECustomSpam() : pAdvertising(nullptr), spamActive(false) {
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

String BLECustomSpam::getPayloadInfo() {
    if (!currentPayload.isValid) {
        return "No payload loaded";
    }
    
    String info = "Name: " + currentPayload.name + "\n";
    info += "Type: " + currentPayload.type + "\n";
    
    if (!currentPayload.completeName.isEmpty()) {
        info += "BLE Name: " + currentPayload.completeName + "\n";
    }
    
    if (!currentPayload.manufacturerData.empty()) {
        info += "Mfg ID: 0x" + String(currentPayload.manufacturerId, HEX) + "\n";
        info += "Mfg Data Len: " + String(currentPayload.manufacturerData.size()) + " bytes\n";
    }
    
    if (!currentPayload.serviceUUIDs.empty()) {
        info += "Services: " + String(currentPayload.serviceUUIDs.size()) + "\n";
    }
    
    if (!currentPayload.rawAdvData.empty()) {
        info += "Raw Data: " + String(currentPayload.rawAdvData.size()) + " bytes\n";
    }
    
    return info;
}

bool BLECustomSpam::configureAdvertising() {
    if (!currentPayload.isValid) {
        return false;
    }
    
    // Initialize BLE if needed
    if (!NimBLEDevice::getInitialized()) {
        // Generate random MAC for spam
        uint8_t macAddr[6];
        for (int i = 0; i < 6; i++) {
            macAddr[i] = esp_random() & 0xFF;
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
        BLEAdvertisementData advertisementData;
        std::string rawData((char*)currentPayload.rawAdvData.data(), currentPayload.rawAdvData.size());
        advertisementData.addData(rawData);
        pAdvertising->setAdvertisementData(advertisementData);
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
    
    // Empty scan response
    BLEAdvertisementData scanResponseData;
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

// Public method to perform one spam cycle
void BLECustomSpam::doSpamCycle() {
    if (pAdvertising && spamActive) {
        pAdvertising->start();
        vTaskDelay(20 / portTICK_PERIOD_MS);
        pAdvertising->stop();
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
            String selectedFile = loopSD(*fs, true, "*.json", "/BruceSD/ble/payloads");
            
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
    
    // Display payload info and start spamming
    if (payloadLoaded) {
        drawMainBorderWithTitle("Payload Loaded");
        padprintln("");
        padprintln(spammer.getPayloadInfo());
        padprintln("");
        padprintln("SELECT:Start ESC:Back");
        
        // Wait for start command
        while (true) {
            if (check(EscPress)) {
                returnToMenu = true;
                return;
            }
            
            if (check(SelPress)) {
                if (spammer.startSpam(SPAM_INTERVAL)) {
                    spamming = true;
                    spamCount = 0;
                    lastSpamTime = millis();
                    break;
                } else {
                    displayTextLine("Start failed!");
                    delay(2000);
                    returnToMenu = true;
                    return;
                }
            }
            
            delay(100);
        }
    }
    
    // Spam loop
    while (spamming) {
        if (millis() - lastSpamTime >= SPAM_INTERVAL) {
            // Generate new random MAC
            uint8_t macAddr[6];
            for (int i = 0; i < 6; i++) {
                macAddr[i] = esp_random() & 0xFF;
            }
            esp_base_mac_addr_set(macAddr);
            
            // Use public method to spam
            spammer.doSpamCycle();
            
            spamCount++;
            lastSpamTime = millis();
            
            // Update display every 10 spams
            if (spamCount % 10 == 0) {
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
