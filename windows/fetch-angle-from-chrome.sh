#!/bin/bash
set -euo pipefail

# Fetch ANGLE libraries and headers from Chrome for Testing (Windows)
#
# ANGLE provides OpenGL ES 2.0/3.0 via D3D11 on Windows. This script extracts
# pre-built libraries from Chrome and fetches matching headers from the ANGLE
# repository.
#
# Note: Chrome for Testing only provides 64-bit Windows builds, so this script
# only supports x86_64 architecture.
#
# Usage: ./fetch-angle-from-chrome.sh [output_dir] [download_dir]
#   output_dir - Installation directory (default: ./angle-libs)
#   download_dir - Download cache directory (default: ./output/download)

# Chrome version and matching ANGLE revision (update together)
readonly CHROME_VERSION="131.0.6778.204"
readonly CHROME_SHA256="97e3ecd4befb54644c3de006510f39157781f43ccdb84cc92d028dbd3851bdcd"
# ANGLE revision from https://chromium.googlesource.com/chromium/src/+/131.0.6778.204/DEPS
readonly ANGLE_REVISION="ac6cda4cbd716102ded6a965f79573b41581898d"
readonly ANGLE_ARCHIVE_SHA256="e375d8b3f38ef6b33c07f03d4868443318b03adff7a9da82c81f3db006c0cdb8"

# Parse arguments
readonly OUTPUT_DIR="${1:-./angle-libs}"
readonly DOWNLOAD_DIR="${2:-./output/download}"
readonly PLATFORM="win64"

readonly LIB_DIR="${OUTPUT_DIR}/lib"
readonly BIN_DIR="${OUTPUT_DIR}/bin"
readonly INCLUDE_DIR="${OUTPUT_DIR}/include"
readonly CHROME_BASE_URL="https://storage.googleapis.com/chrome-for-testing-public"
readonly CHROME_ZIP="chrome-${PLATFORM}.zip"
readonly CHROME_DOWNLOAD="${DOWNLOAD_DIR}/chrome-${PLATFORM}-${CHROME_VERSION}.zip"

echo "Fetching ANGLE from Chrome ${CHROME_VERSION} (win64)"
echo "Output: ${OUTPUT_DIR}"
echo

# Setup temp directory with cleanup
TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "${TEMP_DIR}"' EXIT

# Checksum verification function (portable across Linux and macOS)
sha256_check() {
  local want="$1"
  local file="$2"
  
  if command -v sha256sum >/dev/null 2>&1; then
    echo "${want}  ${file}" | sha256sum -c --quiet
  elif command -v shasum >/dev/null 2>&1; then
    echo "${want}  ${file}" | shasum -a 256 -c --quiet
  else
    echo "WARNING: No sha256 tool found, skipping verification" >&2
    return 0
  fi
}

# Download Chrome if not cached
if [[ -f "${CHROME_DOWNLOAD}" ]]; then
  echo "Using cached Chrome..."
  # Verify cached file
  if ! sha256_check "${CHROME_SHA256}" "${CHROME_DOWNLOAD}"; then
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
    echo "ERROR: Download failed. Check version availability at:" >&2
    echo "  https://googlechromelabs.github.io/chrome-for-testing/" >&2
    exit 1
  fi
  
  # Verify downloaded file
  if ! sha256_check "${CHROME_SHA256}" "${CHROME_DOWNLOAD}"; then
    rm -f "${CHROME_DOWNLOAD}"
    echo "ERROR: Chrome download checksum verification failed" >&2
    exit 1
  fi
fi

echo "Extracting..."
unzip -q "${CHROME_DOWNLOAD}" -d "${TEMP_DIR}"

# Extract ANGLE libraries from Chrome
echo "Locating ANGLE libraries..."

# Find Chrome directory
CHROME_DIR="$(find "${TEMP_DIR}" -maxdepth 2 -type d -name 'chrome-win64' | head -n 1)"
if [[ -z "${CHROME_DIR}" ]]; then
  echo "ERROR: Chrome directory not found" >&2
  ls -la "${TEMP_DIR}" >&2
  exit 1
fi

echo "  Found: ${CHROME_DIR##*/}"

# Locate ANGLE DLLs - they're in the root Chrome directory on Windows
EGL_SOURCE="${CHROME_DIR}/libEGL.dll"
GLES_SOURCE="${CHROME_DIR}/libGLESv2.dll"

if [[ ! -f "${EGL_SOURCE}" ]]; then
  echo "ERROR: libEGL.dll not found" >&2
  exit 1
fi
if [[ ! -f "${GLES_SOURCE}" ]]; then
  echo "ERROR: libGLESv2.dll not found" >&2
  exit 1
fi

mkdir -p "${BIN_DIR}" "${LIB_DIR}"
cp "${EGL_SOURCE}" "${GLES_SOURCE}" "${BIN_DIR}/"
echo "  DLLs: $(du -sh "${BIN_DIR}" | cut -f1)"

# Generate MinGW import libraries
echo "Creating import libraries..."

# Check for required tools
DLLTOOL="x86_64-w64-mingw32-dlltool"
if ! command -v "${DLLTOOL}" >/dev/null 2>&1; then
  echo "ERROR: ${DLLTOOL} not found" >&2
  echo "Install mingw-w64 cross-compiler" >&2
  exit 1
fi

if ! command -v gendef >/dev/null 2>&1; then
  echo "ERROR: gendef not found" >&2
  echo "Install mingw-w64-tools package" >&2
  exit 1
fi

# Create import library from DLL
create_import_lib() {
  local dll_path="$1"
  local dll_name
  dll_name="$(basename "${dll_path}")"
  local base="${dll_name%.dll}"
  local def_file="${TEMP_DIR}/${base}.def"
  local lib_file="${LIB_DIR}/${base}.dll.a"
  
  # Resolve to absolute path before changing directory
  local dll_abs_path
  dll_abs_path="$(cd "$(dirname "${dll_path}")" && pwd)/$(basename "${dll_path}")"
  
  # Generate .def file from DLL (gendef outputs to current directory)
  pushd "${TEMP_DIR}" > /dev/null
  if ! gendef "${dll_abs_path}" 2>/dev/null; then
    popd > /dev/null
    echo "ERROR: gendef failed for ${dll_name}" >&2
    return 1
  fi
  popd > /dev/null
  
  # Create import library
  if ! "${DLLTOOL}" -d "${def_file}" -l "${lib_file}" -D "${dll_name}" 2>/dev/null; then
    echo "ERROR: dlltool failed for ${dll_name}" >&2
    return 1
  fi
}

create_import_lib "${BIN_DIR}/libEGL.dll" || exit 1
create_import_lib "${BIN_DIR}/libGLESv2.dll" || exit 1
echo "  Import libs: $(ls "${LIB_DIR}"/*.dll.a 2>/dev/null | wc -l) files"

# Fetch ANGLE headers
echo "Fetching ANGLE headers..."

readonly ANGLE_ARCHIVE_URL="https://github.com/google/angle/archive/${ANGLE_REVISION}.tar.gz"
readonly ANGLE_ARCHIVE="${DOWNLOAD_DIR}/angle-${ANGLE_REVISION}.tar.gz"

# Download ANGLE archive if not cached
if [[ -f "${ANGLE_ARCHIVE}" ]]; then
  echo "Using cached ANGLE headers..."
  # Verify cached file
  if ! sha256_check "${ANGLE_ARCHIVE_SHA256}" "${ANGLE_ARCHIVE}"; then
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
  if ! sha256_check "${ANGLE_ARCHIVE_SHA256}" "${ANGLE_ARCHIVE}"; then
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
echo
echo "ANGLE installed to: ${OUTPUT_DIR}"
echo "  DLLs: ${BIN_DIR}"
echo "  Import libs: ${LIB_DIR}"
echo "  Headers: ${INCLUDE_DIR}"
echo
echo "Note: Chrome ANGLE DLLs are 64-bit only."
