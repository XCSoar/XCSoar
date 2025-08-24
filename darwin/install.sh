#!/bin/bash
set -euo pipefail

# Configuration
IPA_SIGNED_PATH="$(pwd)/output/IOS64/xcsoar-signed.ipa"
DEVICE_NAME="Your Device Name" # To find device name run: xcrun devicectl list devices

# Input validation
if [ ! -f "$IPA_SIGNED_PATH" ]; then
  echo "❌ Signed IPA not found: $IPA_SIGNED_PATH"
  echo "   Please run the signing script first."
  exit 1
fi

# Install app on device
echo "📱 Installing app on device '$DEVICE_NAME'..."
if ! xcrun devicectl device install app "$IPA_SIGNED_PATH" --device "$DEVICE_NAME"; then
  echo "❌ Installation failed."
  exit 1
fi

echo "✅ App installed successfully"
