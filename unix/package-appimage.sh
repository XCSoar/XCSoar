#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project

set -euo pipefail
cd "$(dirname "$0")/.."

binary=$1

arch=$(uname -m)
case "$(uname -s):$arch" in
  # 32-bit userland on a 64-bit ARM CPU reports armv8l.
  Linux:armv7l|Linux:armv8l)
    arch=armhf
    checksum=e359161979fa4bee50b92ce7102fb510299caebf34f711d983fba7a8f4bb1c2e
    runtime_checksum=e9060d37577b8a29914ec12d8740add24e19ff29012fb1fa0f60daf62db0688d
    ;;
  Linux:x86_64)
    checksum=c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d
    runtime_checksum=2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d
    ;;
  Linux:aarch64)
    checksum=620095110d693282b8ebeb244a95b5e911cf8f65f76c88b4b47d16ae6346fcff
    runtime_checksum=00cbdfcf917cc6c0ff6d3347d59e0ca1f7f45a6df1a428a0d6d8a78664d87444
    ;;
  *)
    echo "AppImage packaging requires native x86_64, aarch64 or armhf Linux." >&2
    exit 1
    ;;
esac

build_dir="$PWD/output/appimage"
mkdir -p "$build_dir"
stage=$(mktemp -d "$build_dir/stage.XXXXXXXX")
trap 'rm -rf -- "$stage"' EXIT

appdir="$stage/AppDir"
install -Dm755 "$binary" "$appdir/usr/bin/xcsoar"
strip "$appdir/usr/bin/xcsoar"
"${MAKE:-make}" TARGET=UNIX DESTDIR="$appdir" install-mo install-desktop
install -Dm644 COPYING "$appdir/usr/share/doc/xcsoar/COPYING"
install -Dm644 THIRD_PARTY_NOTICES.txt \
  "$appdir/usr/share/doc/xcsoar/THIRD_PARTY_NOTICES.txt"

# OBS supplies verified release inputs before entering the build worker.
fetch_asset() {
  local url=$1 digest=$2 name=${1##*/}
  if [ -f "$name" ]; then
    cp -- "$name" "$stage/$name"
  else
    curl -fsSL --retry 3 "$url" -o "$stage/$name"
  fi
  sha256sum --check <<< "$digest  $stage/$name"
}

tool="linuxdeploy-$arch.AppImage"
release=1-alpha-20251107-1
url="https://github.com/linuxdeploy/linuxdeploy/releases/download/$release"
fetch_asset "$url/$tool" "$checksum"
chmod +x "$stage/$tool"

runtime="runtime-$arch"
url="https://github.com/AppImage/type2-runtime/releases/download/20251108"
fetch_asset "$url/$runtime" "$runtime_checksum"

image="XCSoar-$(cat VERSION.txt)-$arch.AppImage"
APPIMAGE_EXTRACT_AND_RUN=1 LDAI_RUNTIME_FILE="$stage/$runtime" \
  LDAI_OUTPUT="$stage/$image" "$stage/$tool" \
  --appdir "$appdir" \
  --desktop-file "$appdir/usr/share/applications/xcsoar.desktop" \
  --icon-file "$appdir/usr/share/icons/hicolor/scalable/apps/xcsoar.svg" \
  --exclude-library 'libEGL.so.*' --exclude-library 'libGLESv2.so.*' \
  --exclude-library 'libGLdispatch.so.*' --output appimage

# For a signer that embeds the signature later (OBS), prepare the image
# the way appimagetool --sign does: write the SHA-256 hex digest to sign
# while .sha256_sig and .sig_key are still zero, then embed the signer's
# public key in .sig_key. Validators zero both sections for the digest.
if [ -n "${APPIMAGE_SIGNING_KEY:-}" ]; then
  sha256sum "$stage/$image" | cut -d' ' -f1 > "$stage/$image.digest"
  read -r offset size < <(readelf -SW "$stage/$image" |
    sed 's/^ *\[ *[0-9]*\]//' | awk '$1 == ".sig_key" { print $4, $5 }')
  if [ "$(stat -c %s "$APPIMAGE_SIGNING_KEY")" -gt $((16#$size)) ]; then
    echo "The signing key does not fit into .sig_key." >&2
    exit 1
  fi
  dd if="$APPIMAGE_SIGNING_KEY" of="$stage/$image" bs=1 \
    seek=$((16#$offset)) conv=notrunc status=none
fi

# Verify the runtime can extract the package before exposing it as an artifact.
(
  cd "$stage"
  "./$image" --appimage-extract usr/bin/xcsoar >/dev/null
  test -x squashfs-root/usr/bin/xcsoar
)
mv -- "$stage/$image" "$build_dir/$image"
if [ -f "$stage/$image.digest" ]; then
  mv -- "$stage/$image.digest" "$build_dir/$image.digest"
fi
echo "$build_dir/$image"
