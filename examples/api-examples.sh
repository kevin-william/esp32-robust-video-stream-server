#!/bin/bash
# Example API calls for ESP32-CAM
# Update ESP32_IP with your device's IP address

# Configuration
ESP32_IP="192.168.1.100"
BASE_URL="http://${ESP32_IP}"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "ESP32-CAM API Examples"
echo "======================"
echo "Device: $BASE_URL"
echo ""

# Function to print and execute command
run_cmd() {
    echo -e "${BLUE}$1${NC}"
    echo -e "${GREEN}$ $2${NC}"
    eval $2
    echo ""
}

# 1. Get system status
run_cmd "1. Get system status" \
    "curl -s $BASE_URL/status | python3 -m json.tool"

# 2. Get sleep status
run_cmd "2. Get camera sleep status" \
    "curl -s $BASE_URL/sleepstatus | python3 -m json.tool"

# 3. Capture a photo
run_cmd "3. Capture photo (saved as capture.jpg)" \
    "curl -s $BASE_URL/capture -o capture.jpg && file capture.jpg"

# 4. Adjust camera quality
run_cmd "4. Set camera quality to 10" \
    "curl -s '$BASE_URL/control?var=quality&val=10' | python3 -m json.tool"

# 5. Set brightness
run_cmd "5. Set brightness to 1" \
    "curl -s '$BASE_URL/control?var=brightness&val=1' | python3 -m json.tool"

# 6. Enable horizontal mirror
run_cmd "6. Enable horizontal mirror" \
    "curl -s '$BASE_URL/control?var=hmirror&val=1' | python3 -m json.tool"

# 7. Set LED intensity
run_cmd "7. Set LED intensity to 50" \
    "curl -s '$BASE_URL/control?var=led_intensity&val=50' | python3 -m json.tool"

# 8. Turn off LED
run_cmd "8. Turn off LED" \
    "curl -s '$BASE_URL/control?var=led_intensity&val=0' | python3 -m json.tool"

# 9. WiFi scan
run_cmd "9. Scan WiFi networks" \
    "curl -s $BASE_URL/wifi-scan | python3 -m json.tool"

# 10. Sleep camera
run_cmd "10. Put camera to sleep" \
    "curl -s $BASE_URL/sleep | python3 -m json.tool"

# 11. Wake camera
echo "Waiting 2 seconds..."
sleep 2
run_cmd "11. Wake camera" \
    "curl -s $BASE_URL/wake | python3 -m json.tool"

echo "================================================"
echo "More examples:"
echo ""
echo "Stream to VLC:"
echo "  vlc $BASE_URL/stream"
echo ""
echo "Stream to ffplay:"
echo "  curl $BASE_URL/stream --output - | ffplay -"
echo ""
echo "View stream in browser:"
echo "  $BASE_URL/stream"
echo ""
echo "Open web UI:"
echo "  $BASE_URL/"
echo ""
echo "Restart device:"
echo "  curl $BASE_URL/restart"
echo ""
