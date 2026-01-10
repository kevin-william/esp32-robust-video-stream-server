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
    if (!sd_mounted)
        return false;
    return SD.exists(path);
}

String readFile(const char* path) {
    if (!sd_mounted)
        return "";

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
    if (!sd_mounted)
        return false;

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
    if (!sd_mounted)
        return false;

    if (SD.exists(path)) {
        return SD.remove(path);
    }
    return true;
}

bool createDirectory(const char* path) {
    if (!sd_mounted)
        return false;

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

// ============================================================================
// Video Recording Functions
// ============================================================================

static File videoFile;
static bool recording_active = false;
static unsigned long frame_count = 0;

/**
 * Initialize video recording to a new file
 * Creates an MJPEG file with basic header
 *
 * @param filename Path to the video file
 * @return true if successful, false otherwise
 */
bool initVideoRecording(const char* filename) {
    if (!sd_mounted) {
        Serial.println("Cannot record: SD card not mounted");
        return false;
    }
    
    if (recording_active) {
        Serial.println("Recording already active, finalizing previous file");
        finalizeVideoRecording();
    }
    
    // Create recordings directory if it doesn't exist
    if (!SD.exists("/recordings")) {
        SD.mkdir("/recordings");
    }
    
    // Open file for writing
    videoFile = SD.open(filename, FILE_WRITE);
    if (!videoFile) {
        Serial.printf("Failed to create video file: %s\n", filename);
        return false;
    }
    
    recording_active = true;
    frame_count = 0;
    
    Serial.printf("Started video recording: %s\n", filename);
    return true;
}

/**
 * Write a JPEG frame to the video file
 * Uses MJPEG format (motion JPEG - sequence of JPEG frames)
 *
 * @param frameData Pointer to JPEG frame data
 * @param frameSize Size of frame data in bytes
 * @return true if successful, false otherwise
 */
bool writeFrameToVideo(const uint8_t* frameData, size_t frameSize) {
    if (!recording_active || !videoFile) {
        Serial.println("Cannot write frame: recording not active");
        return false;
    }
    
    // Write frame size as 4-byte header (for MJPEG parsing)
    uint32_t size = frameSize;
    videoFile.write((uint8_t*)&size, 4);
    
    // Write frame data
    size_t written = videoFile.write(frameData, frameSize);
    
    if (written != frameSize) {
        Serial.printf("Frame write incomplete: %d/%d bytes\n", written, frameSize);
        return false;
    }
    
    frame_count++;
    return true;
}

/**
 * Finalize and close the video recording
 * Flushes buffers and closes the file
 *
 * @return true if successful, false otherwise
 */
bool finalizeVideoRecording() {
    if (!recording_active || !videoFile) {
        return false;
    }
    
    videoFile.flush();
    videoFile.close();
    
    Serial.printf("Video recording finalized: %lu frames\n", frame_count);
    
    recording_active = false;
    frame_count = 0;
    
    return true;
}

/**
 * Check if video recording is currently active
 *
 * @return true if recording, false otherwise
 */
bool isVideoRecording() {
    return recording_active;
}
