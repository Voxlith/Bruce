#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include <NimBLEAdvertising.h>
#include <vector>

// Structure to hold custom payload data
struct CustomPayload {
    String name;
    String type;
    String address;  // BLE MAC address
    uint8_t flags;
    String completeName;
    uint16_t manufacturerId;
    std::vector<uint8_t> manufacturerData;
    std::vector<String> serviceUUIDs;
    int8_t txPower;
    std::vector<uint8_t> rawAdvData;
    std::vector<uint8_t> rawScanRsp;  // Scan Response data
    bool isValid;
};

// BLE Custom Spam class
class BLECustomSpam {
public:
    BLECustomSpam();
    ~BLECustomSpam();
    
    // Load payload from JSON file
    bool loadPayload(const String& filepath);
    
    // Get loaded payload info
    String getPayloadInfo();
    
    // Display payload info with proper padding
    void displayPayloadInfo();
    
    // Start spamming with loaded payload
    bool startSpam(uint32_t intervalMs = 100);
    
    // Stop spamming
    void stopSpam();
    
    // Check if currently spamming
    bool isSpamming();
    
    // Get current payload
    const CustomPayload& getPayload() const { return currentPayload; }
    
    // Public method to perform one spam cycle
    void doSpamCycle();
    
    // MAC randomization control
    void setMacRandomization(bool enabled);
    bool getMacRandomization() const;
    
    // Load payload from JSON string (instead of file)
    bool loadPayloadFromString(const String& jsonStr);
    
private:
    CustomPayload currentPayload;
    std::vector<uint8_t> originalRawAdvData;  // Backup of original raw adv data
    std::vector<uint8_t> originalRawScanRsp;  // Backup of original scan response
    NimBLEAdvertising* pAdvertising;
    bool spamActive;
    bool macRandomizationEnabled;
    
    // Helper to convert hex string to bytes
    std::vector<uint8_t> hexToBytes(const String& hex);
    
    // Helper to parse JSON payload
    bool parsePayload(JsonDocument& doc);
    
    // Configure BLE advertising with custom payload
    bool configureAdvertising();
    
    // Re-set advertising data (without re-init BLE)
    void resetAdvertisingData();
    
    // MAC randomization functions
    void generateSmartRandomMac(uint8_t* newMac);
    void parseMacFromString(const String& macStr, uint8_t* mac);
    void replaceLastThreeMacBytes(std::vector<uint8_t>& data, const uint8_t* oldLast3, const uint8_t* newLast3);
};

// Main function to launch the custom payload spam UI
void ble_custom_payload();
