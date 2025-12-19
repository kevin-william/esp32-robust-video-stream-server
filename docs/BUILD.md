# Build Instructions

Complete guide for building and flashing the ESP32-CAM Robust Video Stream Server.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [PlatformIO (Recommended)](#platformio-recommended)
3. [Arduino IDE](#arduino-ide)
4. [Arduino CLI](#arduino-cli)
5. [Flashing](#flashing)
6. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Hardware

- ESP32-CAM board (AI-Thinker, WROVER-KIT, or compatible)
- USB-to-Serial adapter (FTDI, CP2102, CH340, etc.)
- MicroSD card (optional, FAT32 formatted)
- 5V 2A power supply (USB may be insufficient)

### Software

- Python 3.7 or higher
- Git
- Serial monitor (PuTTY, screen, or IDE built-in)

### Wiring for Programming

**ESP32-CAM to USB-TTL Adapter:**

| ESP32-CAM | USB-TTL |
|-----------|---------|
| GND       | GND     |
| 5V        | 5V      |
| U0T (GPIO 1) | RX   |
| U0R (GPIO 3) | TX   |
| IO0       | GND (for programming mode) |

**Important:** 
- Connect IO0 to GND before powering on to enter programming mode
- Disconnect IO0 after flashing to run normally
- Use external 5V power if USB power is unstable

---

## PlatformIO (Recommended)

### Installation

#### Option 1: VS Code Extension

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install PlatformIO IDE extension:
   - Open VS Code
   - Go to Extensions (Ctrl+Shift+X)
   - Search for "PlatformIO IDE"
   - Click Install

#### Option 2: Command Line

```bash
# Install PlatformIO Core
python -m pip install --upgrade pip
pip install platformio

# Verify installation
pio --version
```

### Building the Project

```bash
# Clone the repository
git clone https://github.com/kevin-william/esp32-robust-video-stream-server.git
cd esp32-robust-video-stream-server

# Build the project
pio run

# Or build specific environment
pio run -e esp32cam
pio run -e esp32cam-debug
```

### Flashing

```bash
# Connect ESP32-CAM in programming mode (IO0 to GND)
# Upload firmware
pio run --target upload

# Specify port if multiple devices
pio run --target upload --upload-port /dev/ttyUSB0  # Linux/Mac
pio run --target upload --upload-port COM3           # Windows

# Upload and monitor
pio run --target upload && pio device monitor
```

### Monitoring

```bash
# Serial monitor (115200 baud)
pio device monitor

# With filters
pio device monitor --filter esp32_exception_decoder

# Exit monitor: Ctrl+C
```

### Cleaning

```bash
# Clean build files
pio run --target clean

# Clean all
pio run --target cleanall
```

---

## Arduino IDE

### Installation

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software) (version 1.8.x or 2.x)

2. Add ESP32 board support:
   - Open Arduino IDE
   - Go to File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Click OK

3. Install ESP32 boards:
   - Go to Tools → Board → Boards Manager
   - Search for "esp32"
   - Install "esp32 by Espressif Systems"

4. Install required libraries:
   - Go to Sketch → Include Library → Manage Libraries
   - Install the following:
     - **ArduinoJson** (by Benoit Blanchon) - version 6.21.0 or higher
     - **AsyncTCP** (by me-no-dev)
     - **ESPAsyncWebServer** (by me-no-dev)
   
   - For ESP32-Camera library:
     ```bash
     # Clone into Arduino libraries folder
     cd ~/Arduino/libraries  # Linux/Mac
     # cd Documents/Arduino/libraries  # Windows
     
     git clone https://github.com/espressif/esp32-camera.git
     ```

### Building and Uploading

1. Open the project:
   - File → Open → Navigate to `src/main.cpp`
   - Save as `main.ino` (Arduino IDE requires .ino extension)

2. Configure board:
   - Tools → Board → ESP32 Arduino → AI Thinker ESP32-CAM
   - Tools → Flash Size → 4MB (32Mb)
   - Tools → Partition Scheme → Minimal SPIFFS (Large APPS with OTA)
   - Tools → Upload Speed → 921600 (or 115200 if issues)

3. Select port:
   - Tools → Port → Select your USB-TTL adapter port
   - Windows: COM3, COM4, etc.
   - Linux: /dev/ttyUSB0, /dev/ttyACM0
   - Mac: /dev/cu.usbserial-*

4. Upload:
   - Ensure IO0 is connected to GND
   - Click Upload button (→)
   - Wait for "Connecting..." message
   - Press and hold RESET button on ESP32-CAM if it doesn't connect
   - Release RESET when uploading starts

5. Monitor:
   - Disconnect IO0 from GND
   - Press RESET button
   - Open Serial Monitor (Tools → Serial Monitor)
   - Set baud rate to 115200

---

## Arduino CLI

### Installation

```bash
# Linux
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Mac (Homebrew)
brew install arduino-cli

# Windows (Chocolatey)
choco install arduino-cli

# Verify
arduino-cli version
```

### Setup

```bash
# Initialize configuration
arduino-cli config init

# Add ESP32 board index
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

# Update package index
arduino-cli core update-index

# Install ESP32 platform
arduino-cli core install esp32:esp32

# Install libraries
arduino-cli lib install "ArduinoJson"
arduino-cli lib install "AsyncTCP"
arduino-cli lib install "ESPAsyncWebServer"

# Install esp32-camera manually
cd ~/Arduino/libraries
git clone https://github.com/espressif/esp32-camera.git
```

### Building

```bash
# Clone repository
git clone https://github.com/kevin-william/esp32-robust-video-stream-server.git
cd esp32-robust-video-stream-server

# Rename main.cpp to main.ino
cp src/main.cpp main.ino

# Compile
arduino-cli compile --fqbn esp32:esp32:esp32cam .

# Compile with verbose output
arduino-cli compile --fqbn esp32:esp32:esp32cam . -v
```

### Uploading

```bash
# List available ports
arduino-cli board list

# Upload (replace /dev/ttyUSB0 with your port)
arduino-cli upload --fqbn esp32:esp32:esp32cam -p /dev/ttyUSB0 .

# Upload with verbose output
arduino-cli upload --fqbn esp32:esp32:esp32cam -p /dev/ttyUSB0 . -v
```

### Monitoring

```bash
# Using arduino-cli
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200

# Or use screen (Linux/Mac)
screen /dev/ttyUSB0 115200

# Or use PuTTY (Windows)
# Connection type: Serial
# Serial line: COM3
# Speed: 115200
```

---

## Flashing

### First Time Flash

1. **Enter Programming Mode:**
   - Connect IO0 to GND
   - Power on or press RESET
   - IO0 LED should light up

2. **Flash Firmware:**
   - Use one of the methods above
   - Wait for "Writing at 0x..." messages
   - Wait for "Hash of data verified"

3. **Exit Programming Mode:**
   - Disconnect IO0 from GND
   - Press RESET button
   - Device should boot normally

### Subsequent Flashes

After first successful flash, you can use OTA updates (future feature) or repeat the programming mode steps.

### Bootloader Issues

If device won't enter programming mode:

```bash
# Use esptool directly
pip install esptool

# Erase flash
esptool.py --chip esp32 --port /dev/ttyUSB0 erase_flash

# Flash bootloader and firmware
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash -z 0x1000 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

---

## Troubleshooting

### Build Errors

#### "esp_camera.h: No such file or directory"

```bash
# Install esp32-camera library
cd ~/Arduino/libraries
git clone https://github.com/espressif/esp32-camera.git
```

#### "AsyncTCP.h: No such file or directory"

```bash
# Install AsyncTCP
cd ~/Arduino/libraries
git clone https://github.com/me-no-dev/AsyncTCP.git
```

#### "Compilation error: ..."

- Ensure Arduino ESP32 core is up to date (2.0.x or higher)
- Verify all libraries are installed
- Clean build and retry

### Upload Errors

#### "A fatal error occurred: Failed to connect"

- Check IO0 is connected to GND
- Press RESET while "Connecting..." appears
- Try lower baud rate (115200)
- Check USB cable and driver

#### "A fatal error occurred: MD5 of file does not match"

- Clean build directory
- Erase flash: `esptool.py erase_flash`
- Retry upload

### Runtime Errors

#### "Camera init failed with error 0x105"

- Check camera ribbon cable connection
- Verify pin definitions match your board
- Ensure sufficient power supply (5V 2A)

#### "Brownout detector was triggered"

- Insufficient power supply
- Use external 5V 2A adapter
- Don't power from USB during camera operation

#### "Guru Meditation Error"

- Stack overflow - increase task stack size
- Memory leak - check frame buffer releases
- Reset device and check serial output

### Connectivity Issues

#### "WiFi connection failed"

- Verify SSID and password
- Check 2.4GHz network (ESP32 doesn't support 5GHz)
- Move closer to router

#### "Can't access web interface"

- Check IP address in serial monitor
- Verify device and computer on same network
- Try http://192.168.4.1 if in AP mode
- Disable firewall temporarily

---

## Performance Tuning

### Optimize for Speed

```cpp
// In platformio.ini or build flags
-O2                    // Compiler optimization level 2
-DCORE_DEBUG_LEVEL=0   // Disable debug output
```

### Optimize for Size

```cpp
-Os                    // Optimize for size
-ffunction-sections    // Remove unused functions
-fdata-sections
```

### Memory Options

```cpp
-DBOARD_HAS_PSRAM      // Enable PSRAM support
-mfix-esp32-psram-cache-issue  // PSRAM workaround
```

---

## Production Build

For production deployment:

1. **Set release build flags:**
   ```ini
   build_flags = 
       -DCORE_DEBUG_LEVEL=0
       -DRELEASE_BUILD=1
       -O2
   ```

2. **Enable security:**
   - Set strong admin password
   - Enable HTTPS (if needed)
   - Review CORS settings

3. **Test thoroughly:**
   - All endpoints
   - Long-running stability
   - Memory leak testing
   - Power cycle testing

4. **Flash and verify:**
   - Flash firmware
   - Verify configuration
   - Test all features
   - Document IP/credentials

---

## Additional Resources

- [PlatformIO Docs](https://docs.platformio.org/)
- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)
- [ESP32-Camera Library](https://github.com/espressif/esp32-camera)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [Project Documentation](docs/)

---

**Happy Building! 🔨**
