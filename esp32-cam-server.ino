/*
 * ESP32-CAM Robust Video Stream Server
 * 
 * This is a wrapper file for Arduino IDE compatibility.
 * The actual implementation is in src/main.cpp
 * 
 * For Arduino IDE:
 * 1. Copy all files from src/ and include/ to the same directory as this .ino file
 * 2. Install required libraries (see README.md)
 * 3. Select Board: AI Thinker ESP32-CAM
 * 4. Upload
 * 
 * For better development experience, use PlatformIO or Arduino CLI.
 * See docs/BUILD.md for detailed instructions.
 */

// Include the main application
#include "src/main.cpp"
