#include "storage.h"
#include <SPI.h>
#include "camera_pins.h"

static bool sd_mounted = false;
static Preferences preferences;

bool initSDCard() {
    // Initialize SPI for SD card
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD Card mount failed");
        sd_mounted = false;
        return false;
    }
    
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        sd_mounted = false;
        return false;
    }
    
    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    
    sd_mounted = true;
    return true;
}

void deinitSDCard() {
    if (sd_mounted) {
        SD.end();
        sd_mounted = false;
    }
}

bool isSDCardMounted() {
    return sd_mounted;
}

bool fileExists(const char* path) {
    if (!sd_mounted) return false;
    return SD.exists(path);
}

String readFile(const char* path) {
    if (!sd_mounted) return "";
    
    File file = SD.open(path, FILE_READ);
    if (!file) {
        Serial.printf("Failed to open file for reading: %s\n", path);
        return "";
    }
    
    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }
    
    file.close();
    return content;
}

bool writeFile(const char* path, const String& content) {
    if (!sd_mounted) return false;
    
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.printf("Failed to open file for writing: %s\n", path);
        return false;
    }
    
    size_t bytesWritten = file.print(content);
    file.close();
    
    return bytesWritten == content.length();
}

bool deleteFile(const char* path) {
    if (!sd_mounted) return false;
    
    if (SD.exists(path)) {
        return SD.remove(path);
    }
    return true;
}

bool createDirectory(const char* path) {
    if (!sd_mounted) return false;
    
    if (!SD.exists(path)) {
        return SD.mkdir(path);
    }
    return true;
}

// NVS/Preferences operations
bool saveToNVS(const char* key, const String& value) {
    preferences.begin("esp32cam", false);
    bool success = preferences.putString(key, value);
    preferences.end();
    return success;
}

String readFromNVS(const char* key, const String& defaultValue) {
    preferences.begin("esp32cam", true);
    String value = preferences.getString(key, defaultValue);
    preferences.end();
    return value;
}

bool clearNVS() {
    preferences.begin("esp32cam", false);
    bool success = preferences.clear();
    preferences.end();
    return success;
}
