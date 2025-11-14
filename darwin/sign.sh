#!/bin/bash
set -euo pipefail

# Input configuration
IPA_DIR="${IPA_DIR:-$(pwd)/output/IOS64/ipa}"
PROFILE_PATH="${PROFILE_PATH:-}"
CERTIFICATE_NAME="${CERTIFICATE_NAME:-}"

# Validate required configuration
if [ -z "$PROFILE_PATH" ] || [ ! -f "$PROFILE_PATH" ]; then
  echo "Error: PROFILE_PATH must be set to a valid provisioning profile" >&2
  echo "Usage: PROFILE_PATH=/path/to/profile.mobileprovision CERTIFICATE_NAME='Apple Development: Your Name (XXXXXXXXX)' $0" >&2
  exit 1
fi

if [ -z "$CERTIFICATE_NAME" ]; then
  echo "Error: CERTIFICATE_NAME must be set" >&2
  echo "Usage: PROFILE_PATH=/path/to/profile.mobileprovision CERTIFICATE_NAME='Apple Development: Your Name (XXXXXXXXX)' $0" >&2
  exit 1
fi
if [ ! -d "$IPA_DIR" ]; then
  echo "❌ IPA_DIR '$IPA_DIR' does not exist. Did you build the app with TARGET=IOS64?" >&2
  exit 2
fi

# Output configuration
IPA_SIGNED_PATH="$(pwd)/output/IOS64/xcsoar-signed.ipa"

# Sign app in output/IOS64/Payload directly
APP_PATH=$(find "$IPA_DIR/Payload" -name "*.app" -type d | head -n 1)
if [ ! -d "$APP_PATH" ]; then
  echo "❌ .app not found in Payload in $IPA_DIR" >&2
  exit 1
fi

# Embed provisioning profile
cp "$PROFILE_PATH" "$APP_PATH/embedded.mobileprovision"

# Extract entitlements from provisioning profile
ENTITLEMENTS_TMP="$(pwd)/output/IOS64/entitlements.plist"
security cms -D -i "$PROFILE_PATH" > "$(pwd)/output/IOS64/profile.plist"
if ! /usr/libexec/PlistBuddy -x -c "Print :Entitlements" "$(pwd)/output/IOS64/profile.plist" > "$ENTITLEMENTS_TMP"; then
  echo "❌ Failed to extract entitlements from provisioning profile" >&2
  exit 1
fi

# Sign the app
echo "🔏 Signing with certificate '$CERTIFICATE_NAME'..."
codesign -f -s "$CERTIFICATE_NAME" --entitlements "$ENTITLEMENTS_TMP" "$APP_PATH"

# Verify signature
if ! codesign --verify --deep --strict "$APP_PATH"; then
  echo "❌ Code signing verification failed" >&2
  exit 1
fi

# Repackage IPA
cd "$IPA_DIR"
zip -qr "$IPA_SIGNED_PATH" Payload

echo "✅ Signed IPA created at: $IPA_SIGNED_PATH"
