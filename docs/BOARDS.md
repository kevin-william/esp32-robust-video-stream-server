# Supported Boards

This document describes the supported ESP32 boards and their specific configurations.

## ESP32-CAM (AI-Thinker)

### Overview
The AI-Thinker ESP32-CAM is the primary development and testing platform for this project. It's a compact board with integrated OV2640 camera module, microSD card slot, and flash LED.

### Features
- ESP32-S chip with WiFi/Bluetooth
- 4MB Flash
- OV2640 camera (2MP, up to 1600x1200)
- MicroSD card slot
- Flash LED on GPIO 4
- External antenna connector

### Build Command
```bash
# Default environment
pio run

# Or explicitly
pio run -e esp32cam

# Upload
pio run -e esp32cam --target upload
```

### Pin Mapping

| Function | GPIO | Notes |
|----------|------|-------|
| **Camera** | | |
| PWDN | 32 | Power-down pin |
| RESET | -1 | Not connected |
| XCLK | 0 | Master clock (20MHz) |
| SIOD (SDA) | 26 | I2C Data |
| SIOC (SCL) | 27 | I2C Clock |
| Y9 (D7) | 35 | Data bit 7 |
| Y8 (D6) | 34 | Data bit 6 |
| Y7 (D5) | 39 | Data bit 5 |
| Y6 (D4) | 36 | Data bit 4 |
| Y5 (D3) | 21 | Data bit 3 |
| Y4 (D2) | 19 | Data bit 2 |
| Y3 (D1) | 18 | Data bit 1 |
| Y2 (D0) | 5 | Data bit 0 |
| VSYNC | 25 | Vertical sync |
| HREF | 23 | Horizontal reference |
| PCLK | 22 | Pixel clock |
| **SD Card** | | |
| CS | 13 | Chip select |
| MOSI | 15 | Master out, slave in |
| MISO | 2 | Master in, slave out |
| SCK | 14 | Clock |
| **LED** | | |
| Flash | 4 | High-brightness LED |

### Notes
- GPIO 0 (XCLK) must be LOW during boot for normal operation
- No external pull-ups needed on I2C lines (internal pull-ups are used)
- Flash LED can draw significant current (~150mA), ensure adequate power supply

---

## ESP32-WROVER-KIT

### Overview
The ESP32-WROVER-KIT is an official Espressif development board featuring the ESP32-WROVER module with integrated PSRAM. It includes JTAG debugging interface and LCD display support.

### Features
- ESP32-WROVER module (8MB PSRAM)
- 4MB Flash  
- OV2640 camera module (optional, sold separately)
- JTAG debugging via FT2232H
- LCD display connector
- RGB LED
- MicroSD card slot
- USB-to-serial converter

### Build Command
```bash
# Build for WROVER-KIT
pio run -e wrover-kit

# Upload
pio run -e wrover-kit --target upload

# Debug (requires JTAG connection)
pio debug -e wrover-kit
```

### Pin Mapping

| Function | GPIO | Notes |
|----------|------|-------|
| **Camera** | | |
| PWDN | -1 | Not used |
| RESET | -1 | Not used |
| XCLK | 21 | Master clock (20MHz) |
| SIOD (SDA) | 26 | I2C Data |
| SIOC (SCL) | 27 | I2C Clock |
| Y9 (D7) | 35 | Data bit 7 |
| Y8 (D6) | 34 | Data bit 6 |
| Y7 (D5) | 39 | Data bit 5 |
| Y6 (D4) | 36 | Data bit 4 |
| Y5 (D3) | 19 | Data bit 3 |
| Y4 (D2) | 18 | Data bit 2 |
| Y3 (D1) | 5 | Data bit 1 |
| Y2 (D0) | 4 | Data bit 0 |
| VSYNC | 25 | Vertical sync |
| HREF | 23 | Horizontal reference |
| PCLK | 22 | Pixel clock |
| **SD Card** | | |
| CS | 13 | Chip select |
| MOSI | 15 | Master out, slave in |
| MISO | 2 | Master in, slave out |
| SCK | 14 | Clock |
| **Other** | | |
| RGB LED | 0, 2, 4 | Red, Green, Blue |

### Notes
- Camera module is **not included** by default - must be purchased separately
- JTAG pins are reserved: GPIOs 12, 13, 14, 15
- 8MB PSRAM allows for higher resolution and more frame buffers
- No dedicated flash LED - can use RGB LED if needed

### Debugging
The WROVER-KIT includes JTAG support for advanced debugging:

```bash
# Start debug session
pio debug -e wrover-kit

# Common GDB commands
break main.cpp:123  # Set breakpoint
continue            # Continue execution
print variable      # Print variable value
backtrace          # Show call stack
```

---

## Adding Support for New Boards

To add support for a new ESP32 board with camera:

### 1. Add PlatformIO Environment

Edit `platformio.ini` and add a new environment:

```ini
[env:your-board]
platform = espressif32
board = your-board-name
framework = arduino

monitor_speed = 115200
monitor_filters = esp32_exception_decoder

build_flags = 
    -DCORE_DEBUG_LEVEL=2
    -DBOARD_HAS_PSRAM            ; If your board has PSRAM
    -DCAMERA_MODEL_YOUR_BOARD    ; Define your board model
    -DCONFIG_ASYNC_TCP_RUNNING_CORE=0
    -DCONFIG_ASYNC_TCP_USE_WDT=1
    -O2
    -DARDUINO_LOOP_STACK_SIZE=8192

lib_deps = 
    bblanchon/ArduinoJson@^6.21.0
    me-no-dev/AsyncTCP@^1.1.1
    https://github.com/me-no-dev/ESPAsyncWebServer.git

upload_speed = 921600
board_build.partitions = min_spiffs.csv
board_build.filesystem = littlefs
```

### 2. Add Pin Definitions

Edit `include/camera_pins.h` and add your board's pin mapping:

```cpp
#ifdef CAMERA_MODEL_YOUR_BOARD
#    define PWDN_GPIO_NUM xx
#    define RESET_GPIO_NUM xx
#    define XCLK_GPIO_NUM xx
#    define SIOD_GPIO_NUM xx
#    define SIOC_GPIO_NUM xx

#    define Y9_GPIO_NUM xx
#    define Y8_GPIO_NUM xx
#    define Y7_GPIO_NUM xx
#    define Y6_GPIO_NUM xx
#    define Y5_GPIO_NUM xx
#    define Y4_GPIO_NUM xx
#    define Y3_GPIO_NUM xx
#    define Y2_GPIO_NUM xx
#    define VSYNC_GPIO_NUM xx
#    define HREF_GPIO_NUM xx
#    define PCLK_GPIO_NUM xx

#    define LED_GPIO_NUM xx
#    define LED_LEDC_CHANNEL 2

#    define SD_CS_PIN xx
#    define SD_MOSI_PIN xx
#    define SD_MISO_PIN xx
#    define SD_SCK_PIN xx
#endif
```

### 3. Test

```bash
# Build
pio run -e your-board

# Upload and monitor
pio run -e your-board --target upload
pio device monitor -b 115200
```

### 4. Document

Add your board configuration to this file and update the main README.md.

---

## Build Flags Reference

| Flag | Description |
|------|-------------|
| `CAMERA_MODEL_AI_THINKER` | AI-Thinker ESP32-CAM board |
| `CAMERA_MODEL_WROVER_KIT` | ESP32-WROVER-KIT board |
| `BOARD_HAS_PSRAM` | Board has external PSRAM |
| `CORE_DEBUG_LEVEL` | ESP32 logging level (0-5) |
| `CONFIG_ASYNC_TCP_RUNNING_CORE` | Core for AsyncTCP tasks |
| `ARDUINO_LOOP_STACK_SIZE` | Stack size for Arduino loop |

## Troubleshooting

### ESP32-CAM Won't Boot
- Ensure GPIO 0 is not pulled HIGH during boot
- Check power supply (needs stable 5V, ~500mA min)
- Disconnect camera module and test

### WROVER-KIT Camera Not Detected
- Verify camera module is properly connected
- Check I2C communication on GPIOs 26/27
- Ensure XCLK is generating 20MHz signal

### Upload Fails
- **ESP32-CAM**: Hold BOOT button (GPIO 0) during upload
- **WROVER-KIT**: Should upload automatically via USB
- Check serial port selection and permissions

### Low Frame Rate
- Reduce resolution (try QVGA or HVGA)
- Increase JPEG quality number (lower quality, faster encoding)
- Enable PSRAM if available
- Check WiFi signal strength
