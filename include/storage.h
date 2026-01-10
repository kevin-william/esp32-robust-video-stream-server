#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <FS.h>
#include <Preferences.h>
#include <SD.h>

// SD Card management
bool initSDCard();
void deinitSDCard();
bool isSDCardMounted();

// File operations
bool fileExists(const char* path);
String readFile(const char* path);
bool writeFile(const char* path, const String& content);
bool deleteFile(const char* path);
bool createDirectory(const char* path);

// NVS/Preferences operations
bool saveToNVS(const char* key, const String& value);
String readFromNVS(const char* key, const String& defaultValue = "");
bool clearNVS();

// Video recording operations
bool initVideoRecording(const char* filename);
bool writeFrameToVideo(const uint8_t* frameData, size_t frameSize);
bool finalizeVideoRecording();
bool isVideoRecording();

#endif  // STORAGE_H
