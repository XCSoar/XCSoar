#!/bin/bash
set -euo pipefail

# Fetch ANGLE libraries and headers from XCSoar/angle-libs
#
# ANGLE provides OpenGL ES 2.0/3.0 via D3D11 on Windows. Pre-built DLLs and
# headers are downloaded from the GitHub CI release at:
#   https://github.com/XCSoar/angle-libs
#
# Usage: ./fetch-angle-from-github.sh [output_dir] [download_dir] [arch]
#   output_dir   - Installation directory (default: ./angle-libs)
#   download_dir - Download cache directory (default: ./output/download)
#   arch         - x64 or x86 (default: x64)

# Release tag and full commit hash from XCSoar/angle-libs
readonly ANGLE_LIBS_TAG="a96fca8"
readonly ANGLE_LIBS_COMMIT="a96fca8d5ee2ca61e8de419e38cd577579281c9e"

readonly OUTPUT_DIR="${1:-./angle-libs}"
readonly DOWNLOAD_DIR="${2:-./output/download}"
readonly ARCH="${3:-x64}"

readonly LIB_DIR="${OUTPUT_DIR}/lib"
readonly BIN_DIR="${OUTPUT_DIR}/bin"
readonly INCLUDE_DIR="${OUTPUT_DIR}/include"

case "${ARCH}" in
  x64)
    readonly TARBALL_SHA256="82723e19795d683e6af2afadf39fb00d248d6a5a2cb2af9faeebc017a7f4f5d8"
    readonly DLLTOOL="x86_64-w64-mingw32-dlltool"
    ;;
  x86)
    readonly TARBALL_SHA256="02eb72f10673bca9569c0c044f5ff877a2815a85b9c47555ec1b04a5d36a4c00"
    readonly DLLTOOL="i686-w64-mingw32-dlltool"
    ;;
  *)
    echo "ERROR: Unknown arch '${ARCH}': expected x64 or x86" >&2
    exit 1
    ;;
esac

readonly TARBALL_NAME="angle-windows-${ARCH}-${ANGLE_LIBS_COMMIT}.tar.gz"
readonly TARBALL_URL="https://github.com/XCSoar/angle-libs/releases/download/${ANGLE_LIBS_TAG}/${TARBALL_NAME}"
readonly TARBALL_DOWNLOAD="${DOWNLOAD_DIR}/${TARBALL_NAME}"
readonly TARBALL_DIR="angle-windows-${ARCH}-${ANGLE_LIBS_COMMIT}"

# x86 tarball has no include/ directory; fetch headers from x64 tarball
readonly X64_TARBALL_SHA256="82723e19795d683e6af2afadf39fb00d248d6a5a2cb2af9faeebc017a7f4f5d8"
readonly X64_TARBALL_NAME="angle-windows-x64-${ANGLE_LIBS_COMMIT}.tar.gz"
readonly X64_TARBALL_URL="https://github.com/XCSoar/angle-libs/releases/download/${ANGLE_LIBS_TAG}/${X64_TARBALL_NAME}"
readonly X64_TARBALL_DOWNLOAD="${DOWNLOAD_DIR}/${X64_TARBALL_NAME}"
readonly X64_TARBALL_DIR="angle-windows-x64-${ANGLE_LIBS_COMMIT}"

echo "Fetching ANGLE from XCSoar/angle-libs ${ANGLE_LIBS_TAG} (windows-${ARCH})"
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
    echo "ERROR: No sha256 tool found (need sha256sum or shasum)" >&2
    return 1
  fi
}

download_tarball() {
  local url="$1"
  local dest="$2"
  local sha256="$3"
  local label="$4"

  if [[ -f "${dest}" ]]; then
    echo "Using cached ${label}..."
    if ! sha256_check "${sha256}" "${dest}"; then
      echo "WARNING: Cached ${label} failed verification, re-downloading..." >&2
      rm -f "${dest}"
    fi
  fi

  if [[ ! -f "${dest}" ]]; then
    echo "Downloading ${label}..."
    mkdir -p "${DOWNLOAD_DIR}"
    if ! curl -fL -# "${url}" -o "${dest}"; then
      echo "ERROR: Download failed." >&2
      exit 1
    fi
    if ! sha256_check "${sha256}" "${dest}"; then
      rm -f "${dest}"
      echo "ERROR: Checksum verification failed for ${label}" >&2
      exit 1
    fi
  fi
}

# Download arch-specific DLL tarball
download_tarball "${TARBALL_URL}" "${TARBALL_DOWNLOAD}" "${TARBALL_SHA256}" "${TARBALL_NAME}"

# For x86, also download x64 tarball to get the headers (x86 tarball has none)
if [[ "${ARCH}" == "x86" ]]; then
  download_tarball "${X64_TARBALL_URL}" "${X64_TARBALL_DOWNLOAD}" "${X64_TARBALL_SHA256}" "${X64_TARBALL_NAME}"
fi

echo "Extracting..."
tar xzf "${TARBALL_DOWNLOAD}" -C "${TEMP_DIR}"

if [[ "${ARCH}" == "x86" ]]; then
  tar xzf "${X64_TARBALL_DOWNLOAD}" -C "${TEMP_DIR}" "${X64_TARBALL_DIR}/include"
fi

mkdir -p "${BIN_DIR}" "${LIB_DIR}" "${INCLUDE_DIR}"

# Copy DLLs
cp "${TEMP_DIR}/${TARBALL_DIR}/libEGL.dll" "${BIN_DIR}/"
cp "${TEMP_DIR}/${TARBALL_DIR}/libGLESv2.dll" "${BIN_DIR}/"
echo "  DLLs: $(du -sh "${BIN_DIR}" | cut -f1)"

# Copy headers (from x64 tarball when building x86)
if [[ "${ARCH}" == "x86" ]]; then
  cp -R "${TEMP_DIR}/${X64_TARBALL_DIR}/include/." "${INCLUDE_DIR}/"
else
  cp -R "${TEMP_DIR}/${TARBALL_DIR}/include/." "${INCLUDE_DIR}/"
fi
header_count=$(find "${INCLUDE_DIR}" -name "*.h" | wc -l | tr -d ' ')
echo "  Headers: ${header_count} files"

# Generate MinGW import libraries from DLLs
echo "Creating import libraries..."

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

create_import_lib() {
  local dll_path="$1"
  local dll_name
  dll_name="$(basename "${dll_path}")"
  local base="${dll_name%.dll}"
  local def_file="${TEMP_DIR}/${base}.def"
  local lib_file="${LIB_DIR}/${base}.dll.a"

  local dll_abs_path
  dll_abs_path="$(cd "$(dirname "${dll_path}")" && pwd)/$(basename "${dll_path}")"

  pushd "${TEMP_DIR}" > /dev/null
  if ! gendef "${dll_abs_path}" 2>/dev/null; then
    popd > /dev/null
    echo "ERROR: gendef failed for ${dll_name}" >&2
    return 1
  fi
  popd > /dev/null

  # x86 stdcall uses @N decorations: gendef omits @0 for void-argument
  # functions, so add it so MinGW resolves __imp__glFunc@0 references.
  # x64 has a single calling convention with no @N suffixes
  if [ "${ARCH}" = "x86" ]; then
    sed -i -E 's/^(gl[A-Za-z0-9_]*|EGL_[A-Za-z0-9_]*|egl[A-Za-z0-9_]*)$/\1@0/' "${def_file}"
  fi

  if ! "${DLLTOOL}" --kill-at -d "${def_file}" -l "${lib_file}" -D "${dll_name}" 2>/dev/null; then
    echo "ERROR: dlltool failed for ${dll_name}" >&2
    return 1
  fi
}

create_import_lib "${BIN_DIR}/libEGL.dll" || exit 1
create_import_lib "${BIN_DIR}/libGLESv2.dll" || exit 1
echo "  Import libs: $(ls "${LIB_DIR}"/*.dll.a 2>/dev/null | wc -l) files"

echo
echo "ANGLE installed to: ${OUTPUT_DIR}"
echo "  DLLs:        ${BIN_DIR}"
echo "  Import libs: ${LIB_DIR}"
echo "  Headers:     ${INCLUDE_DIR}"
