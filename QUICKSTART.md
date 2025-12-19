# Quick Start Guide

Get your ESP32-CAM up and running in minutes!

## What You Need

### Hardware
- ESP32-CAM board (AI-Thinker or compatible)
- USB-to-Serial adapter (FTDI, CP2102, or CH340)
- MicroSD card (optional, FAT32)
- 5V 2A power supply

### Software
- [PlatformIO](https://platformio.org/) **OR** [Arduino IDE](https://www.arduino.cc/en/software)
- Serial terminal (built-in with above tools)

## Quick Setup (3 Steps)

### Step 1: Wire Up

Connect ESP32-CAM to USB-TTL adapter:

```
ESP32-CAM  →  USB-TTL
------------------------
GND        →  GND
5V         →  5V
U0T        →  RX
U0R        →  TX
IO0        →  GND (for programming only!)
```

### Step 2: Flash Firmware

#### Using PlatformIO (Recommended)

```bash
# Clone and build
git clone https://github.com/kevin-william/esp32-robust-video-stream-server.git
cd esp32-robust-video-stream-server
pio run --target upload

# Monitor output
pio device monitor
```

#### Using Arduino IDE

1. Install ESP32 board support (see docs/BUILD.md)
2. Install libraries: ArduinoJson, AsyncTCP, ESPAsyncWebServer, esp32-camera
3. Open `esp32-cam-server.ino`
4. Select Board: "AI Thinker ESP32-CAM"
5. Click Upload

### Step 3: Configure WiFi

1. **Disconnect IO0 from GND** and press RESET
2. Look for WiFi network: `ESP32-CAM-Setup` (password: `12345678`)
3. Connect and open browser to: `http://192.168.4.1`
4. Click "Scan Networks" and select your WiFi
5. Enter password and click "Connect"
6. Device restarts and connects to your WiFi!

## Using the Camera

Once connected, find your ESP32-CAM's IP address in the serial monitor, then:

### View Live Stream
```
http://<YOUR-ESP32-IP>/stream
```

### Capture Photo
```bash
curl http://<YOUR-ESP32-IP>/capture -o photo.jpg
```

### Control Panel
```
http://<YOUR-ESP32-IP>/
```
Use the web interface to:
- View live stream
- Capture photos
- Adjust camera settings (quality, brightness, etc.)
- Control LED flash
- Monitor system status

## Common Issues

### "Camera init failed"
- Check camera ribbon cable
- Ensure 5V 2A power supply
- Verify pin connections

### "Can't connect to WiFi"
- Check password
- Use 2.4GHz network (ESP32 doesn't support 5GHz)
- Move closer to router

### "Upload failed"
- Ensure IO0 is connected to GND during upload
- Try lower baud rate (115200)
- Press RESET button when "Connecting..." appears

## What's Next?

- Read [API Documentation](docs/API.md) for all endpoints
- Check [Architecture Guide](docs/ARCHITECTURE.md) to understand the design
- Review [Build Instructions](docs/BUILD.md) for advanced options
- See [Contributing Guide](CONTRIBUTING.md) to help improve the project

## Example API Calls

```bash
# Get system status
curl http://192.168.1.100/status

# Adjust camera quality
curl "http://192.168.1.100/control?var=quality&val=10"

# Sleep camera (save power)
curl http://192.168.1.100/sleep

# Wake camera
curl http://192.168.1.100/wake

# Restart device
curl http://192.168.1.100/restart
```

## Support

- 📖 Full documentation in `docs/` folder
- 🐛 Report issues on GitHub
- 💬 Check existing issues for solutions
- 🚀 Contribute improvements (see CONTRIBUTING.md)

---

**Enjoy your ESP32-CAM! 📹**
