#!/bin/bash
set -euo pipefail

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# Get the project root (parent of darwin directory)
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Load .env file if it exists
if [[ -f "$SCRIPT_DIR/.env" ]]; then
  echo "📝 Loading configuration from darwin/.env"
  set -a
  source "$SCRIPT_DIR/.env"
  set +a
fi

# Input configuration - use environment variables with fallback defaults
# App is always XCSoar.app (like iOS), regardless of TESTING flag
APP_PATH="${APP_PATH:-$PROJECT_ROOT/output/MACOS/app/XCSoar.app}"

PROFILE_PATH="${MACOS_PROFILE_PATH:-$SCRIPT_DIR/de_yorickreum_XCSoar_App_Store_Connect_macOS.provisionprofile}"
CERTIFICATE_NAME="${MACOS_CERTIFICATE_NAME:-3rd Party Mac Developer Application}"
INSTALLER_CERTIFICATE_NAME="${MACOS_INSTALLER_CERTIFICATE_NAME:-3rd Party Mac Developer Installer}"

# Output paths
OUTPUT_DIR="$PROJECT_ROOT/output/MACOS"
SIGNED_PKG_PATH="${SIGNED_PKG_PATH:-${OUTPUT_DIR}/XCSoar-signed.pkg}"

# Create temporary directory
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT
ENTITLEMENTS_TMP="$TMP_DIR/entitlements.plist"

# Guard against missing build artifact
if [[ ! -d "$APP_PATH" ]]; then
  echo "❌ .app bundle not found: $APP_PATH"
  echo "   Build with: make TARGET=MACOS app  (or TESTING=y for testing version)"
  exit 1
fi

# Check if provisioning profile exists
if [[ ! -f "$PROFILE_PATH" ]]; then
  echo "⚠️  No provisioning profile found at: $PROFILE_PATH"
  echo "   Creating unsigned copy..."
  
  # Just copy the unsigned app to signed location for consistency
  cp -a "$APP_PATH" "$TMP_DIR/XCSoar.app"
  
  # Create an unsigned pkg
  pkgbuild --root "$TMP_DIR" \
    --identifier "$(defaults read "$APP_PATH/Contents/Info.plist" CFBundleIdentifier)" \
    --version "$(defaults read "$APP_PATH/Contents/Info.plist" CFBundleShortVersionString)" \
    --install-location /Applications \
    "$SIGNED_PKG_PATH"
  
  echo "✅ Unsigned package created at: $SIGNED_PKG_PATH"
  exit 0
fi

echo "📦 Signing macOS app for App Store"
echo "   App: $APP_PATH"
echo "   Certificate: $CERTIFICATE_NAME"

# Use manual entitlements file with app-sandbox
ENTITLEMENTS_FILE="$SCRIPT_DIR/macos-appstore-sandbox.entitlements"
echo "📄 Using entitlements from: $ENTITLEMENTS_FILE"

if [[ ! -f "$ENTITLEMENTS_FILE" ]]; then
  echo "❌ Entitlements file not found: $ENTITLEMENTS_FILE"
  exit 1
fi

# Copy entitlements to temp location
cp "$ENTITLEMENTS_FILE" "$ENTITLEMENTS_TMP"

# Embed provisioning profile in app
mkdir -p "$APP_PATH/Contents"
cp "$PROFILE_PATH" "$APP_PATH/Contents/embedded.provisionprofile"

# Extract team/app identifier from provisioning profile and add to entitlements
echo "📄 Adding team identifiers from provisioning profile..."
security cms -D -i "$PROFILE_PATH" > "$TMP_DIR/profile.plist"

# Extract and add application-identifier and team-identifier
APP_ID=$(/usr/libexec/PlistBuddy -c "Print :Entitlements:com.apple.application-identifier" "$TMP_DIR/profile.plist" 2>/dev/null || echo "")
TEAM_ID=$(/usr/libexec/PlistBuddy -c "Print :Entitlements:com.apple.developer.team-identifier" "$TMP_DIR/profile.plist" 2>/dev/null || echo "")

if [[ -n "$APP_ID" ]]; then
  /usr/libexec/PlistBuddy -c "Add :com.apple.application-identifier string $APP_ID" "$ENTITLEMENTS_TMP" 2>/dev/null || \
  /usr/libexec/PlistBuddy -c "Set :com.apple.application-identifier $APP_ID" "$ENTITLEMENTS_TMP"
fi

if [[ -n "$TEAM_ID" ]]; then
  /usr/libexec/PlistBuddy -c "Add :com.apple.developer.team-identifier string $TEAM_ID" "$ENTITLEMENTS_TMP" 2>/dev/null || \
  /usr/libexec/PlistBuddy -c "Set :com.apple.developer.team-identifier $TEAM_ID" "$ENTITLEMENTS_TMP"
  
  # Also add keychain-access-groups
  /usr/libexec/PlistBuddy -c "Add :keychain-access-groups array" "$ENTITLEMENTS_TMP" 2>/dev/null
  /usr/libexec/PlistBuddy -c "Add :keychain-access-groups:0 string ${TEAM_ID}.*" "$ENTITLEMENTS_TMP" 2>/dev/null
fi

if [[ -d "$APP_PATH/Contents/Frameworks" ]]; then
  echo "🔏 Signing frameworks..."
  find "$APP_PATH/Contents/Frameworks" -name "*.dylib" -o -name "*.framework" | while read -r framework; do
    if [[ -f "$framework" ]] || [[ -d "$framework" ]]; then
      echo "   Signing: $(basename "$framework")"
      codesign --force --sign "$CERTIFICATE_NAME" \
        --timestamp --options runtime \
        "$framework" || echo "   ⚠️  Warning: Failed to sign $framework"
    fi
  done
fi

# Sign the main application bundle
echo "🔏 Signing application bundle..."
codesign --force --sign "$CERTIFICATE_NAME" \
  --entitlements "$ENTITLEMENTS_TMP" \
  --timestamp --options runtime \
  --deep "$APP_PATH"

# Verify signature
echo "✅ Verifying signature..."
if ! codesign --verify --deep --strict --verbose=2 "$APP_PATH"; then
  echo "❌ Code signing verification failed"
  exit 1
fi

# Display signature info
echo "📋 Signature information:"
codesign -dvvv "$APP_PATH" 2>&1 | grep -E "Authority|Identifier|TeamIdentifier|Signed Time"

# Create signed .pkg installer for App Store
echo "📦 Creating signed installer package..."

# Create signed .pkg using productbuild --component
# This automatically extracts LSMinimumSystemVersion from Info.plist
BUNDLE_ID="$(defaults read "$APP_PATH/Contents/Info.plist" CFBundleIdentifier)"
BUNDLE_VERSION="$(defaults read "$APP_PATH/Contents/Info.plist" CFBundleShortVersionString)"

echo "   Bundle ID: $BUNDLE_ID"
echo "   Version: $BUNDLE_VERSION"

# Build and sign product directly from app bundle
productbuild --component "$APP_PATH" /Applications \
  --sign "$INSTALLER_CERTIFICATE_NAME" \
  "$SIGNED_PKG_PATH"

# Fix ownership if files were created as root (shouldn't happen, but just in case)
if [[ -O "$SIGNED_PKG_PATH" ]]; then
  : # Already owned by current user
else
  echo "⚠️  Package has wrong ownership, this shouldn't happen"
fi

# Verify package signature
if ! pkgutil --check-signature "$SIGNED_PKG_PATH"; then
  echo "❌ Package signature verification failed"
  exit 1
fi

echo "✅ Signed package created at: $SIGNED_PKG_PATH"
