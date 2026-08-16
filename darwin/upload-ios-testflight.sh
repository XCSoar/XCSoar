#!/bin/bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "$script_dir/.env" ]]; then
  # shellcheck disable=SC1091
  source "$script_dir/.env"
fi

find_archive() {
  for candidate in \
    "ios-testflight-build/output/IOS64/XCSoar.xcarchive" \
    "ios-testflight-build/XCSoar.xcarchive" \
    "XCSoar.xcarchive"; do
    if [[ -d "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

find_ipa() {
  for candidate in \
    "ios-testflight-build/output/IOS64/xcsoar-signed.ipa" \
    "ios-testflight-build/xcsoar-signed.ipa" \
    "xcsoar-signed.ipa"; do
    if [[ -f "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

if [[ -n "${ASC_API_KEY_BASE64:-}" && -n "${ASC_API_KEY_ID:-}" && -n "${ASC_API_ISSUER_ID:-}" ]]; then
  archive_path="$(find_archive)" || {
    echo "iOS archive not found after download" >&2
    exit 1
  }

  temporary_directory="$(mktemp -d "${RUNNER_TEMP:-/tmp}/xcsoar-ios-upload.XXXXXX")"
  trap 'rm -rf "$temporary_directory"' EXIT
  api_key_path="$temporary_directory/AuthKey_${ASC_API_KEY_ID}.p8"
  export_options="$temporary_directory/ExportOptions.plist"
  profile_plist="$temporary_directory/embedded-profile.plist"

  printf '%s' "$ASC_API_KEY_BASE64" | base64 --decode > "$api_key_path"
  chmod 600 "$api_key_path"

  profile_path="$(find "$archive_path/Products/Applications" -name embedded.mobileprovision -type f -print -quit)"
  if [[ -z "$profile_path" ]]; then
    echo "Embedded provisioning profile not found in iOS archive" >&2
    exit 1
  fi
  security cms -D -i "$profile_path" > "$profile_plist"
  profile_uuid="$(/usr/libexec/PlistBuddy -c 'Print :UUID' "$profile_plist")"
  profile_name="$(/usr/libexec/PlistBuddy -c 'Print :Name' "$profile_plist")"
  app_path="$(find "$archive_path/Products/Applications" -maxdepth 1 -name '*.app' -type d -print -quit)"
  bundle_identifier="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$app_path/Info.plist")"
  mkdir -p "$HOME/Library/MobileDevice/Provisioning Profiles"
  cp "$profile_path" "$HOME/Library/MobileDevice/Provisioning Profiles/$profile_uuid.mobileprovision"
  cat > "$export_options" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>destination</key>
  <string>upload</string>
  <key>method</key>
  <string>app-store-connect</string>
  <key>provisioningProfiles</key>
  <dict>
    <key>$bundle_identifier</key>
    <string>$profile_name</string>
  </dict>
  <key>signingCertificate</key>
  <string>Apple Distribution</string>
  <key>signingStyle</key>
  <string>manual</string>
  <key>uploadSymbols</key>
  <true/>
</dict>
</plist>
EOF

  xcodebuild -exportArchive \
    -archivePath "$archive_path" \
    -exportOptionsPlist "$export_options" \
    -exportPath "$temporary_directory/export" \
    -authenticationKeyPath "$api_key_path" \
    -authenticationKeyID "$ASC_API_KEY_ID" \
    -authenticationKeyIssuerID "$ASC_API_ISSUER_ID"
  exit 0
fi

echo "::warning::App Store Connect API key is not configured; uploading the IPA without dSYM delivery"
ipa_path="$(find_ipa)" || {
  echo "Signed IPA not found after download" >&2
  exit 1
}

if [[ -z "${APPLE_ID:-}" || -z "${APP_SPECIFIC_PASSWORD:-}" ]]; then
  echo "TestFlight credentials not configured" >&2
  exit 1
fi

echo "Uploading iOS build to App Store Connect / TestFlight..."
if [[ -n "${ASC_PROVIDER:-}" ]]; then
  xcrun altool --upload-app \
    -f "$ipa_path" \
    -t ios \
    -u "$APPLE_ID" \
    -p "$APP_SPECIFIC_PASSWORD" \
    --asc-provider "$ASC_PROVIDER"
else
  xcrun altool --upload-app \
    -f "$ipa_path" \
    -t ios \
    -u "$APPLE_ID" \
    -p "$APP_SPECIFIC_PASSWORD"
fi
