#include "ble_scanner.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include <globals.h>

// BLEScanner implementation
BLEScanner::BLEScanner() : pBLEScan(nullptr), callbacks(nullptr) {
    scannedDevices.clear();
}

BLEScanner::~BLEScanner() {
    stopScan();
    if (callbacks) {
        delete callbacks;
        callbacks = nullptr;
    }
}

// AdvertisedDeviceCallbacks implementation
BLEScanner::AdvertisedDeviceCallbacks::AdvertisedDeviceCallbacks(std::vector<ScannedDevice>* devices) 
    : deviceList(devices) {}

void BLEScanner::AdvertisedDeviceCallbacks::onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    ScannedDevice device;
    
    // Basic info
    device.name = advertisedDevice->haveName() ? String(advertisedDevice->getName().c_str()) : "Unknown";
    device.address = String(advertisedDevice->getAddress().toString().c_str());
    device.rssi = advertisedDevice->getRSSI();
    
    // Flags
    device.flags = 0;
    if (advertisedDevice->haveAppearance()) {
        device.flags = advertisedDevice->getAppearance();
    }
    
    // TX Power
    device.txPower = advertisedDevice->haveTXPower() ? advertisedDevice->getTXPower() : 0;
    
    // Manufacturer Data
    device.hasManufacturerData = advertisedDevice->haveManufacturerData();
    if (device.hasManufacturerData) {
        std::string mfgData = advertisedDevice->getManufacturerData();
        if (mfgData.length() >= 2) {
            device.manufacturerId = (uint8_t)mfgData[0] | ((uint8_t)mfgData[1] << 8);
            for (size_t i = 0; i < mfgData.length(); i++) {
                device.manufacturerData.push_back((uint8_t)mfgData[i]);
            }
        }
    }
    
    // Service UUIDs - using correct NimBLE API
    if (advertisedDevice->haveServiceUUID()) {
        // NimBLE 1.4.0 uses isAdvertisingService() to check individual UUIDs
        // We'll get the service UUID count and iterate
        NimBLEUUID serviceUUID = advertisedDevice->getServiceUUID();
        device.serviceUUIDs.push_back(String(serviceUUID.toString().c_str()));
    }
    
    // Raw advertisement data
    uint8_t* payload = advertisedDevice->getPayload();
    size_t payloadLen = advertisedDevice->getPayloadLength();
    for (size_t i = 0; i < payloadLen; i++) {
        device.rawAdvData.push_back(payload[i]);
    }
    
    device.hasScanResponse = false;
    
    // Check if device already exists (by address)
    bool found = false;
    for (auto& existingDevice : *deviceList) {
        if (existingDevice.address == device.address) {
            // Update RSSI if device already in list
            existingDevice.rssi = device.rssi;
            found = true;
            break;
        }
    }
    
    if (!found) {
        deviceList->push_back(device);
    }
}

bool BLEScanner::startScan(uint32_t duration) {
    // Initialize BLE if not already done
    if (!NimBLEDevice::getInitialized()) {
        NimBLEDevice::init("");
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
    // Create scan object
    pBLEScan = NimBLEDevice::getScan();
    
    // Set callback
    callbacks = new AdvertisedDeviceCallbacks(&scannedDevices);
    pBLEScan->setAdvertisedDeviceCallbacks(callbacks, false);
    
    // Configure scan
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    pBLEScan->setMaxResults(0); // No limit
    
    // Start scan
    pBLEScan->start(duration, false);
    
    return true;
}

void BLEScanner::stopScan() {
    if (pBLEScan) {
        pBLEScan->stop();
        pBLEScan->clearResults();
    }
    
    if (NimBLEDevice::getInitialized()) {
        NimBLEDevice::deinit();
    }
}

std::vector<ScannedDevice>& BLEScanner::getDevices() {
    return scannedDevices;
}

void BLEScanner::clearDevices() {
    scannedDevices.clear();
}

String BLEScanner::bytesToHex(const uint8_t* data, size_t len) {
    String hex = "";
    for (size_t i = 0; i < len; i++) {
        char buf[3];
        sprintf(buf, "%02X", data[i]);
        hex += buf;
    }
    return hex;
}

bool BLEScanner::saveDevicePayload(const ScannedDevice& device, String filename) {
    // Ensure SD card or LittleFS is available
    FS* fs = nullptr;
    if (sdcardMounted) {
        fs = &SD;
    } else if (LittleFS.begin()) {
        fs = &LittleFS;
    } else {
        Serial.println("No file system available");
        return false;
    }
    
    // Create directory if it doesn't exist
    String dirPath = "/BruceSD/ble/payloads";
    if (fs == &LittleFS) {
        dirPath = "/ble/payloads";
    }
    
    // Create nested directories
    if (!fs->exists("/BruceSD") && fs == &SD) fs->mkdir("/BruceSD");
    if (!fs->exists(dirPath.substring(0, dirPath.lastIndexOf('/')))) {
        fs->mkdir(dirPath.substring(0, dirPath.lastIndexOf('/')));
    }
    if (!fs->exists(dirPath)) {
        fs->mkdir(dirPath);
    }
    
    // Generate filename if not provided
    if (filename.isEmpty()) {
        filename = device.name;
        if (filename.isEmpty() || filename == "Unknown") {
            filename = device.address;
            filename.replace(":", "");
        }
        // Remove invalid characters
        filename.replace("/", "_");
        filename.replace("\\", "_");
        filename.replace(" ", "_");
    }
    
    // Ensure .json extension
    if (!filename.endsWith(".json")) {
        filename += ".json";
    }
    
    String filepath = dirPath + "/" + filename;
    
    // Create JSON document
    JsonDocument doc;
    
    doc["version"] = "1.0";
    doc["name"] = device.name;
    doc["type"] = "advertisement";
    doc["address"] = device.address;
    doc["rssi"] = device.rssi;
    
    // Advertisement data
    JsonObject advData = doc["adv_data"].to<JsonObject>();
    advData["flags"] = String(device.flags, HEX);
    advData["complete_name"] = device.name;
    advData["tx_power"] = device.txPower;
    
    // Manufacturer data
    if (device.hasManufacturerData) {
        JsonObject mfgData = advData["manufacturer_data"].to<JsonObject>();
        mfgData["company_id"] = "0x" + String(device.manufacturerId, HEX);
        mfgData["data"] = bytesToHex(device.manufacturerData.data(), device.manufacturerData.size());
    }
    
    // Service UUIDs
    if (!device.serviceUUIDs.empty()) {
        JsonArray uuids = advData["service_uuids"].to<JsonArray>();
        for (const auto& uuid : device.serviceUUIDs) {
            uuids.add(uuid);
        }
    }
    
    // Raw data
    doc["raw_adv_data"] = bytesToHex(device.rawAdvData.data(), device.rawAdvData.size());
    
    // Write to file
    File file = fs->open(filepath, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }
    
    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write JSON");
        file.close();
        return false;
    }
    
    file.close();
    Serial.println("Payload saved to: " + filepath);
    return true;
}

String BLEScanner::getDeviceDetails(const ScannedDevice& device) {
    String details = "Name: " + device.name + "\n";
    details += "Address: " + device.address + "\n";
    details += "RSSI: " + String(device.rssi) + " dBm\n";
    
    if (device.hasManufacturerData) {
        details += "Mfg ID: 0x" + String(device.manufacturerId, HEX) + "\n";
        details += "Mfg Data: " + bytesToHex(device.manufacturerData.data(), 
                                            min((size_t)16, device.manufacturerData.size())) + "\n";
    }
    
    if (!device.serviceUUIDs.empty()) {
        details += "Services: ";
        for (size_t i = 0; i < min((size_t)2, device.serviceUUIDs.size()); i++) {
            details += device.serviceUUIDs[i];
            if (i < device.serviceUUIDs.size() - 1) details += ", ";
        }
        details += "\n";
    }
    
    return details;
}

// Main UI function
void ble_scan_advertiser() {
    BLEScanner scanner;
    bool scanning = false;
    int selectedIndex = 0;
    unsigned long scanStartTime = 0;
    const uint32_t SCAN_DURATION = 10; // seconds
    
    drawMainBorderWithTitle("BLE Scan Advertiser");
    padprintln("");
    padprintln("Press OK to start scan");
    padprintln("Press ESC to exit");
    
    while (true) {
        // Handle scanning state
        if (scanning) {
            if (millis() - scanStartTime < SCAN_DURATION * 1000) {
                // Still scanning
                drawMainBorderWithTitle("Scanning... " + String((SCAN_DURATION * 1000 - (millis() - scanStartTime)) / 1000) + "s");
                padprintln("");
                padprintln("Devices found: " + String(scanner.getDevices().size()));
                padprintln("");
                padprintln("Press ESC to stop");
                delay(500);
            } else {
                // Scan complete
                scanner.stopScan();
                scanning = false;
                
                if (scanner.getDevices().empty()) {
                    drawMainBorderWithTitle("No devices found");
                    padprintln("");
                    padprintln("Press OK to rescan");
                    padprintln("Press ESC to exit");
                } else {
                    selectedIndex = 0;
                }
            }
        } else if (!scanner.getDevices().empty()) {
            // Display device list
            auto& devices = scanner.getDevices();
            drawMainBorderWithTitle("Found " + String(devices.size()) + " devices");
            
            // Display devices with selection
            int startIdx = max(0, selectedIndex - 3);
            int endIdx = min((int)devices.size(), startIdx + 5);
            
            for (int i = startIdx; i < endIdx; i++) {
                String line = (i == selectedIndex) ? "> " : "  ";
                line += devices[i].name;
                if (devices[i].name.isEmpty() || devices[i].name == "Unknown") {
                    line = (i == selectedIndex) ? "> " : "  ";
                    line += devices[i].address.substring(0, 12);
                }
                line += " (" + String(devices[i].rssi) + ")";
                padprintln(line);
            }
            
            padprintln("");
            padprintln("OK:View ESC:Back");
        }
        
        // Handle input
        if (check(EscPress)) {
            if (scanning) {
                scanner.stopScan();
                scanning = false;
            }
            returnToMenu = true;
            break;
        }
        
        if (check(SelPress)) {
            if (scanning) {
                // Do nothing while scanning
            } else if (scanner.getDevices().empty()) {
                // Start new scan
                scanner.clearDevices();
                scanner.startScan(SCAN_DURATION);
                scanning = true;
                scanStartTime = millis();
            } else {
                // View selected device details
                auto& device = scanner.getDevices()[selectedIndex];
                
                // Show device details and options
                drawMainBorderWithTitle(device.name.isEmpty() ? device.address : device.name);
                padprintln("");
                padprintln(scanner.getDeviceDetails(device));
                padprintln("");
                padprintln("OK:Save ESC:Back");
                
                // Wait for input
                while (true) {
                    if (check(EscPress)) break;
                    if (check(SelPress)) {
                        // Save device
                        if (scanner.saveDevicePayload(device)) {
                            displayTextLine("Saved!");
                            delay(1000);
                        } else {
                            displayTextLine("Save failed!");
                            delay(1000);
                        }
                        break;
                    }
                    delay(100);
                }
            }
        }
        
        if (!scanning && !scanner.getDevices().empty()) {
            if (check(UpPress)) {
                selectedIndex = max(0, selectedIndex - 1);
                delay(200);
            }
            if (check(DownPress)) {
                selectedIndex = min((int)scanner.getDevices().size() - 1, selectedIndex + 1);
                delay(200);
            }
        }
        
        delay(100);
    }
    
    scanner.stopScan();
}
