#!/bin/bash
set -euo pipefail

# Fetch ANGLE libraries and headers from Chrome for Testing
#
# ANGLE provides OpenGL ES 2.0/3.0 via Metal on macOS, replacing Apple's
# deprecated OpenGL. This script extracts pre-built libraries from Chrome and
# fetches matching headers from the ANGLE repository.
#
# Usage: ./fetch-angle-from-chrome.sh [output_dir] [download_dir]
#   output_dir - Installation directory (default: ./angle-libs)
#   download_dir - Download cache directory (default: ./output/download)

# Chrome version and matching ANGLE revision (update together)
readonly CHROME_VERSION="131.0.6778.204"
readonly CHROME_SHA256="8fd911a87a91304b62e6e995114f5a3cf381a7d788d79895aff3e325b6c207ed"
# ANGLE revision from https://chromium.googlesource.com/chromium/src/+/131.0.6778.204/DEPS
readonly ANGLE_REVISION="ac6cda4cbd716102ded6a965f79573b41581898d"
readonly ANGLE_ARCHIVE_SHA256="e375d8b3f38ef6b33c07f03d4868443318b03adff7a9da82c81f3db006c0cdb8"

# Parse arguments
readonly OUTPUT_DIR="${1:-./angle-libs}"
readonly DOWNLOAD_DIR="${2:-./output/download}"
readonly PLATFORM="mac-arm64"

readonly LIB_DIR="${OUTPUT_DIR}/lib"
readonly INCLUDE_DIR="${OUTPUT_DIR}/include"
readonly CHROME_BASE_URL="https://storage.googleapis.com/chrome-for-testing-public"
readonly CHROME_ZIP="chrome-${PLATFORM}.zip"
readonly CHROME_DOWNLOAD="${DOWNLOAD_DIR}/${CHROME_ZIP}"

echo "Fetching ANGLE from Chrome ${CHROME_VERSION} (mac-arm64)"
echo "Output: ${OUTPUT_DIR}"
echo

# Setup temp directory with cleanup
TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TEMP_DIR}"' EXIT

# Download Chrome if not cached
if [[ -f "${CHROME_DOWNLOAD}" ]]; then
  echo "Using cached Chrome..."
  # Verify cached file
  if ! echo "${CHROME_SHA256}  ${CHROME_DOWNLOAD}" | shasum -a 256 -c --quiet; then
    echo "WARNING: Cached file failed verification, re-downloading..." >&2
    rm -f "${CHROME_DOWNLOAD}"
  fi
fi

# Download if not cached or verification failed
if [[ ! -f "${CHROME_DOWNLOAD}" ]]; then
  echo "Downloading Chrome..."
  mkdir -p "${DOWNLOAD_DIR}"
  if ! curl -fL -# "${CHROME_BASE_URL}/${CHROME_VERSION}/${PLATFORM}/${CHROME_ZIP}" \
       -o "${CHROME_DOWNLOAD}"; then
    echo "ERROR: Download failed. Check version availability." >&2
    exit 1
  fi
  
  # Verify downloaded file
  if ! echo "${CHROME_SHA256}  ${CHROME_DOWNLOAD}" | shasum -a 256 -c --quiet; then
    rm -f "${CHROME_DOWNLOAD}"
    echo "ERROR: Chrome download checksum verification failed" >&2
    exit 1
  fi
fi

echo "Extracting..."
unzip -q "${CHROME_DOWNLOAD}" -d "${TEMP_DIR}"

# Extract ANGLE libraries from Chrome bundle
echo "Locating ANGLE libraries..."

# Find Chrome app bundle
CHROME_APP="$(find "${TEMP_DIR}" -maxdepth 4 -type d -name '*Chrome*app' | head -n 1)"
if [[ -z "${CHROME_APP}" ]]; then
  echo "ERROR: Chrome .app not found" >&2
  exit 1
fi

echo "  Found: ${CHROME_APP##*/}"

# Find ANGLE libraries in Chrome Framework
# Try multiple possible paths (structure varies by Chrome version)
CHROME_LIB_DIR="$(find "${CHROME_APP}" \( \
  -path '*/Framework.framework/Versions/*/Libraries' -o \
  -path '*/Framework.framework/Libraries' -o \
  -path '*/Frameworks/*/Libraries' \
\) -type d | head -n 1)"

if [[ -z "${CHROME_LIB_DIR}" ]]; then
  echo "ERROR: Libraries directory not found. Chrome structure:" >&2
  find "${CHROME_APP}" -type d -name "*Framework*" | head -5 >&2
  exit 1
fi

echo "  Libraries: ${CHROME_LIB_DIR##*/}"

# Locate ANGLE library files
EGL_SOURCE="${CHROME_LIB_DIR}/libEGL.dylib"
GLES_SOURCE="${CHROME_LIB_DIR}/libGLESv2.dylib"

if [[ ! -f "${EGL_SOURCE}" ]]; then
  echo "ERROR: libEGL.dylib not found" >&2
  exit 1
fi
if [[ ! -f "${GLES_SOURCE}" ]]; then
  echo "ERROR: libGLESv2.dylib not found" >&2
  exit 1
fi

mkdir -p "${LIB_DIR}"
cp "${EGL_SOURCE}" "${GLES_SOURCE}" "${LIB_DIR}/"
echo "  Libraries: $(du -sh "${LIB_DIR}" | cut -f1)"

# Fetch ANGLE headers
echo "Fetching ANGLE headers..."

readonly ANGLE_ARCHIVE_URL="https://github.com/google/angle/archive/${ANGLE_REVISION}.tar.gz"
readonly ANGLE_ARCHIVE="${DOWNLOAD_DIR}/angle-${ANGLE_REVISION}.tar.gz"

# Download ANGLE archive if not cached
if [[ -f "${ANGLE_ARCHIVE}" ]]; then
  echo "Using cached ANGLE headers..."
  # Verify cached file
  if ! echo "${ANGLE_ARCHIVE_SHA256}  ${ANGLE_ARCHIVE}" | shasum -a 256 -c --quiet; then
    echo "WARNING: Cached ANGLE archive failed verification, re-downloading..." >&2
    rm -f "${ANGLE_ARCHIVE}"
  fi
fi

# Download if not cached or verification failed
if [[ ! -f "${ANGLE_ARCHIVE}" ]]; then
  mkdir -p "${DOWNLOAD_DIR}"
  if ! curl -fL -# "${ANGLE_ARCHIVE_URL}" -o "${ANGLE_ARCHIVE}"; then
    echo "ERROR: Failed to download ANGLE headers" >&2
    exit 1
  fi
  
  # Verify downloaded file
  if ! echo "${ANGLE_ARCHIVE_SHA256}  ${ANGLE_ARCHIVE}" | shasum -a 256 -c --quiet; then
    rm -f "${ANGLE_ARCHIVE}"
    echo "ERROR: ANGLE archive checksum verification failed" >&2
    exit 1
  fi
fi

# Extract headers from archive
mkdir -p "${INCLUDE_DIR}"
if ! tar xzf "${ANGLE_ARCHIVE}" -C "${TEMP_DIR}" "angle-${ANGLE_REVISION}/include"; then
  echo "ERROR: Failed to extract ANGLE headers" >&2
  exit 1
fi

# Copy only needed headers (EGL, GLES2, KHR, angle_gl.h, export.h, platform)
ANGLE_INCLUDE_SRC="${TEMP_DIR}/angle-${ANGLE_REVISION}/include"
for dir in EGL GLES2 KHR platform; do
  if [[ -d "${ANGLE_INCLUDE_SRC}/${dir}" ]]; then
    cp -R "${ANGLE_INCLUDE_SRC}/${dir}" "${INCLUDE_DIR}/"
  fi
done
for file in angle_gl.h export.h; do
  if [[ -f "${ANGLE_INCLUDE_SRC}/${file}" ]]; then
    cp "${ANGLE_INCLUDE_SRC}/${file}" "${INCLUDE_DIR}/"
  fi
done

# Validate required headers were copied
for required in EGL GLES2 KHR; do
  if [[ ! -d "${INCLUDE_DIR}/${required}" ]]; then
    echo "ERROR: Required header directory ${required}/ not found in ANGLE archive" >&2
    exit 1
  fi
done

# Count headers
header_count=$(find "${INCLUDE_DIR}" -name "*.h" | wc -l | tr -d ' ')
echo "  Headers: ${header_count} files"

# Summary
echo "ANGLE installed to: ${OUTPUT_DIR}"
