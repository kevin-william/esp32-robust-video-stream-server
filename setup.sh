#!/bin/bash
# ESP32-CAM Quick Setup Script
# Helps set up development environment and build the project

set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║        ESP32-CAM Robust Video Stream Server - Setup           ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper functions
print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

# Check Python
echo "Checking prerequisites..."
if command -v python3 &> /dev/null; then
    PYTHON_VERSION=$(python3 --version)
    print_success "Python found: $PYTHON_VERSION"
else
    print_error "Python 3 is required but not found"
    echo "Please install Python 3.7 or higher from python.org"
    exit 1
fi

# Check if PlatformIO is installed
if command -v pio &> /dev/null; then
    PIO_VERSION=$(pio --version)
    print_success "PlatformIO found: $PIO_VERSION"
else
    print_warning "PlatformIO not found"
    read -p "Install PlatformIO now? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Installing PlatformIO..."
        python3 -m pip install --user platformio
        export PATH=$PATH:$HOME/.local/bin
        print_success "PlatformIO installed!"
    else
        print_error "PlatformIO is required. Install manually with:"
        echo "  pip install platformio"
        exit 1
    fi
fi

# Add PlatformIO to PATH if needed
export PATH=$PATH:$HOME/.local/bin

echo ""
echo "What would you like to do?"
echo "1) Build project"
echo "2) Build and upload to device"
echo "3) Monitor serial output"
echo "4) Clean build files"
echo "5) Show device information"
echo "6) Exit"
echo ""
read -p "Enter choice [1-6]: " choice

case $choice in
    1)
        echo ""
        print_info "Building project..."
        pio run
        print_success "Build complete!"
        echo ""
        echo "Firmware location:"
        ls -lh .pio/build/esp32cam/*.bin 2>/dev/null || echo "Check .pio/build/esp32cam/ directory"
        ;;
    2)
        echo ""
        print_info "Building and uploading..."
        echo ""
        print_warning "Make sure:"
        echo "  1. ESP32-CAM is connected to USB-TTL adapter"
        echo "  2. IO0 is connected to GND"
        echo "  3. Device is powered on"
        echo ""
        read -p "Press Enter when ready..."
        
        # List available ports
        print_info "Available ports:"
        pio device list
        echo ""
        read -p "Enter port (e.g., /dev/ttyUSB0 or COM3): " port
        
        pio run --target upload --upload-port "$port"
        print_success "Upload complete!"
        echo ""
        print_warning "Now disconnect IO0 from GND and press RESET"
        ;;
    3)
        echo ""
        print_info "Available ports:"
        pio device list
        echo ""
        read -p "Enter port to monitor: " port
        echo ""
        print_info "Starting monitor (Ctrl+C to exit)..."
        pio device monitor --port "$port" --baud 115200
        ;;
    4)
        echo ""
        print_info "Cleaning build files..."
        pio run --target clean
        print_success "Clean complete!"
        ;;
    5)
        echo ""
        print_info "Device Information:"
        echo ""
        pio device list
        ;;
    6)
        echo "Goodbye!"
        exit 0
        ;;
    *)
        print_error "Invalid choice"
        exit 1
        ;;
esac

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                       Quick Help                               ║"
echo "╠════════════════════════════════════════════════════════════════╣"
echo "║ First boot: Device starts in AP mode                          ║"
echo "║ SSID: ESP32-CAM-Setup                                         ║"
echo "║ Password: 12345678                                            ║"
echo "║ Navigate to: http://192.168.4.1                               ║"
echo "║                                                                ║"
echo "║ After WiFi config: Check serial monitor for IP address        ║"
echo "║ Access at: http://<IP-ADDRESS>/                               ║"
echo "║                                                                ║"
echo "║ Full docs: docs/                                              ║"
echo "║ Quick start: QUICKSTART.md                                    ║"
echo "║ API reference: docs/API.md                                    ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
