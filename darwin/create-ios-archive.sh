#!/bin/bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IPA_PATH="${IOS_SIGNED_IPA_PATH:-$PROJECT_ROOT/output/IOS64/xcsoar-signed.ipa}"
DSYM_PATH="${IOS_DSYM_PATH:-$PROJECT_ROOT/output/IOS64/XCSoar.app.dSYM}"
ARCHIVE_PATH="${IOS_ARCHIVE_PATH:-$PROJECT_ROOT/output/IOS64/XCSoar.xcarchive}"

if [[ ! -f "$IPA_PATH" ]]; then
  echo "Signed IPA not found: $IPA_PATH" >&2
  exit 1
fi

if [[ ! -d "$DSYM_PATH" ]]; then
  echo "dSYM not found: $DSYM_PATH" >&2
  exit 1
fi

TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TEMP_DIR"' EXIT

unzip -q "$IPA_PATH" -d "$TEMP_DIR"
APP_PATH="$(find "$TEMP_DIR/Payload" -mindepth 1 -maxdepth 1 -type d -name '*.app' -print -quit)"
if [[ -z "$APP_PATH" ]]; then
  echo "No app bundle found in $IPA_PATH" >&2
  exit 1
fi

APP_NAME="$(basename "$APP_PATH")"
APP_EXECUTABLE="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$APP_PATH/Info.plist")"
APP_BINARY="$APP_PATH/$APP_EXECUTABLE"
if [[ ! -f "$APP_BINARY" ]]; then
  echo "App executable not found: $APP_BINARY" >&2
  exit 1
fi

binary_uuid="$(xcrun dwarfdump --uuid "$APP_BINARY" | awk '{ print $2 }' | LC_ALL=C sort | tr '\n' ' ')"
dsym_uuid="$(xcrun dwarfdump --uuid "$DSYM_PATH" | awk '{ print $2 }' | LC_ALL=C sort | tr '\n' ' ')"
if [[ -z "$binary_uuid" || "$binary_uuid" != "$dsym_uuid" ]]; then
  echo "dSYM UUID mismatch: binary=$binary_uuid dSYM=$dsym_uuid" >&2
  exit 1
fi

bundle_identifier="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$APP_PATH/Info.plist")"
bundle_version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$APP_PATH/Info.plist")"
short_version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$APP_PATH/Info.plist")"
signing_identity="$(codesign -dvv "$APP_PATH" 2>&1 | sed -n 's/^Authority=//p' | head -n 1)"
team_identifier="$(codesign -dvv "$APP_PATH" 2>&1 | sed -n 's/^TeamIdentifier=//p' | head -n 1)"

rm -rf "$ARCHIVE_PATH"
mkdir -p "$ARCHIVE_PATH/Products/Applications" "$ARCHIVE_PATH/dSYMs"
cp -a "$APP_PATH" "$ARCHIVE_PATH/Products/Applications/$APP_NAME"
cp -a "$DSYM_PATH" "$ARCHIVE_PATH/dSYMs/"

cat > "$ARCHIVE_PATH/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>ApplicationProperties</key>
  <dict>
    <key>ApplicationPath</key>
    <string>Applications/$APP_NAME</string>
    <key>CFBundleIdentifier</key>
    <string>$bundle_identifier</string>
    <key>CFBundleShortVersionString</key>
    <string>$short_version</string>
    <key>CFBundleVersion</key>
    <string>$bundle_version</string>
    <key>SigningIdentity</key>
    <string>$signing_identity</string>
    <key>Team</key>
    <string>$team_identifier</string>
  </dict>
  <key>ArchiveVersion</key>
  <integer>2</integer>
  <key>CreationDate</key>
  <date>$(date -u +%Y-%m-%dT%H:%M:%SZ)</date>
  <key>Name</key>
  <string>XCSoar</string>
  <key>SchemeName</key>
  <string>XCSoar</string>
</dict>
</plist>
EOF

echo "Created iOS archive: $ARCHIVE_PATH"