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
    uint8_t flags;
    String completeName;
    uint16_t manufacturerId;
    std::vector<uint8_t> manufacturerData;
    std::vector<String> serviceUUIDs;
    int8_t txPower;
    std::vector<uint8_t> rawAdvData;
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
    
private:
    CustomPayload currentPayload;
    NimBLEAdvertising* pAdvertising;
    bool spamActive;
    
    // Helper to convert hex string to bytes
    std::vector<uint8_t> hexToBytes(const String& hex);
    
    // Helper to parse JSON payload
    bool parsePayload(JsonDocument& doc);
    
    // Configure BLE advertising with custom payload
    bool configureAdvertising();
};

// Main function to launch the custom payload spam UI
void ble_custom_payload();
