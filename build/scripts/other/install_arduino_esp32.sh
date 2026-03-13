#!/bin/bash

# ==============================================================================
# The Ultimate "Whack-a-Mole" Arduino CLI ESP32 Installer
# Defeats WSL/NAT "Connection Reset" firewalls automatically.
# ==============================================================================

ESP_INDEX_URL="https://espressif.github.io/arduino-esp32/package_esp32_index.json"
ARDUINO_DIR="$HOME/.arduino15"
STAGING_DIR="$ARDUINO_DIR/staging/packages"

echo "Checking for required system tools (wget)..."
if ! command -v wget &> /dev/null; then
    sudo apt-get update && sudo apt-get install -y wget
fi

echo "Cleaning up any broken manual installations..."
rm -rf "$ARDUINO_DIR/packages/esp32/hardware/esp32/3.3.7"

echo "---------------------------------------------------"
echo "PHASE 1: Standard Initialization"
echo "---------------------------------------------------"

mkdir -p "$STAGING_DIR"
arduino-cli config init --overwrite
arduino-cli config set network.connection_timeout 300s
arduino-cli config add board_manager.additional_urls "$ESP_INDEX_URL"

echo "---------------------------------------------------"
echo "PHASE 2: Forcing Index Files (Bypassing update-index)"
echo "---------------------------------------------------"
# update-index almost always fails on strict networks. We bypass it by
# directly placing the indices where the CLI expects them.
echo "Downloading core JSON indices..."
wget -q --show-progress https://downloads.arduino.cc/packages/package_index.json -O "$ARDUINO_DIR/package_index.json"
wget -q --show-progress "$ESP_INDEX_URL" -O "$ARDUINO_DIR/package_esp32_index.json"
wget -q --show-progress https://downloads.arduino.cc/libraries/library_index.json -O "$ARDUINO_DIR/library_index.json"

echo "---------------------------------------------------"
echo "PHASE 3: The Automated Whack-a-Mole Install Loop"
echo "---------------------------------------------------"

MAX_RETRIES=10
RETRY_COUNT=0
INSTALL_SUCCESS=false

# Loop to catch failing HEAD requests, cache them, and retry
while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    echo "Running Arduino CLI Installer (Attempt $((RETRY_COUNT+1))/$MAX_RETRIES)..."

    # Run the install and capture the output to read the errors
    INSTALL_OUT=$(arduino-cli core install esp32:esp32 2>&1)
    EXIT_CODE=$?

    # Print the output so the user can see what's happening
    echo "$INSTALL_OUT"

    if [ $EXIT_CODE -eq 0 ]; then
        echo "✅ SUCCESS! All dependencies installed cleanly."
        INSTALL_SUCCESS=true
        break
    fi

    # Use RegEx to scrape the exact URL that triggered the "Connection reset"
    FAILED_URL=$(echo "$INSTALL_OUT" | grep -o 'Head "[^"]*"' | head -n 1 | cut -d '"' -f 2)

    if [ -n "$FAILED_URL" ]; then
        echo "❌ Network blocked a tool download!"
        echo "🪓 Whack-a-Mole Activated: Intercepting URL -> $FAILED_URL"

        cd "$STAGING_DIR" || exit
        echo "Downloading to staging cache via wget..."
        wget -nc -q --show-progress "$FAILED_URL"

        echo "File cached. Retrying installation..."
        RETRY_COUNT=$((RETRY_COUNT+1))
    else
        echo "❌ Installation failed for an unknown reason (Not a HEAD request block)."
        break
    fi
done

if [ "$INSTALL_SUCCESS" = true ]; then
    echo "==================================================="
    echo "🏁 Installation Complete & Bulletproof!"
    arduino-cli core list
    echo "==================================================="
else
    echo "🚨 Max retries reached or unknown error occurred."
fi