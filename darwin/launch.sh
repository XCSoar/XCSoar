#!/bin/bash
set -euo pipefail

# Parse arguments
SHOW_LOGS=true
if [ $# -gt 0 ] && [ "$1" = "--no-logs" ]; then
    SHOW_LOGS=false
fi

# Path to the app bundle
APP_PATH="$(pwd)/output/IOS64SIM/ipa/Payload/XCSoar.app"

# Check if app exists
if [ ! -d "$APP_PATH" ]; then
    echo "Error: App not found at $APP_PATH"
    echo "Please build the app first"
    exit 1
fi

# Boot the simulator if not already running
echo "Starting iOS Simulator..."
xcrun simctl boot "iPhone 16 Pro" 2>/dev/null || echo "Simulator already running"

# Wait a moment for simulator to boot
sleep 2

# Open Simulator app
open -a Simulator

# Install the app
echo "Installing XCSoar.app..."
xcrun simctl install booted "$APP_PATH"

# Get the app's bundle identifier
BUNDLE_ID=$(defaults read "$APP_PATH/Info.plist" CFBundleIdentifier)

# Launch the app with console output
echo "Launching XCSoar (Bundle ID: $BUNDLE_ID)..."
if [ "$SHOW_LOGS" = true ]; then
    echo ""
    echo "Streaming stdout/stderr (press Ctrl+C to stop)..."
    echo "----------------------------------------"
    xcrun simctl launch --console booted "$BUNDLE_ID"
else
    xcrun simctl launch booted "$BUNDLE_ID"
    echo "XCSoar launched successfully!"
fi
