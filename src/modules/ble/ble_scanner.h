#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>
#include <vector>

// Structure to hold scanned BLE device information
struct ScannedDevice {
    String name;
    String address;
    int rssi;
    std::vector<uint8_t> manufacturerData;
    uint16_t manufacturerId;
    std::vector<String> serviceUUIDs;
    int8_t txPower;
    uint8_t flags;
    std::vector<uint8_t> rawAdvData;
    std::vector<uint8_t> rawScanRsp;
    bool hasManufacturerData;
    bool hasScanResponse;
};

// BLE Scanner class
class BLEScanner {
public:
    BLEScanner();
    ~BLEScanner();
    
    // Start scanning for BLE devices
    bool startScan(uint32_t duration = 5);
    
    // Stop scanning
    void stopScan();
    
    // Get list of scanned devices
    std::vector<ScannedDevice>& getDevices();
    
    // Clear the device list
    void clearDevices();
    
    // Save a device payload to JSON file
    bool saveDevicePayload(const ScannedDevice& device, String filename = "");
    
    // Get device details as formatted string
    String getDeviceDetails(const ScannedDevice& device);
    
private:
    NimBLEScan* pBLEScan;
    std::vector<ScannedDevice> scannedDevices;
    
    // Callback class for scan results
    class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    public:
        AdvertisedDeviceCallbacks(std::vector<ScannedDevice>* devices);
        void onResult(NimBLEAdvertisedDevice* advertisedDevice) override;
    private:
        std::vector<ScannedDevice>* deviceList;
    };
    
    AdvertisedDeviceCallbacks* callbacks;
    
    // Helper to convert bytes to hex string
    String bytesToHex(const uint8_t* data, size_t len);
    
    // Helper to extract manufacturer data
    void extractManufacturerData(NimBLEAdvertisedDevice* device, ScannedDevice& scanned);
};

// Main function to launch the BLE scanner UI
void ble_scan_advertiser();
