#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project
#
# Create a Debian sysroot for cross-compiling XCSoar.
# armhf (default): TARGET=PI / PI2 / PI3, root /opt/pi/root.
# arm64: TARGET=PI4 / PI5, root /opt/pi64/root.
# Pi 4/5 can also be built natively on the board (see doc/build.rst).
#
# Usage (requires root, e.g. sudo):
#   sudo ./ide/provisioning/setup-pi-sysroot.sh
#   sudo ./ide/provisioning/setup-pi-sysroot.sh --arch arm64
#   sudo PI_ROOT=$HOME/pi-sysroot ./ide/provisioning/setup-pi-sysroot.sh
#
# Then on the host:
#   make TARGET=PI2 SYSROOT=/opt/pi/root USE_CCACHE=y DEBUG=y
#   make TARGET=PI4 SYSROOT=/opt/pi64/root USE_CCACHE=y DEBUG=y
#
# Host packages: ide/provisioning/install-debian-packages.sh PI_HOST
#
# Sysroot dev packages align with LINUX + LIBINPUT_GBM in
# install-debian-packages.sh, plus Pi graphics (DRM/GBM/GLES).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

PI_SYSROOT_PACKAGES=(
  build-essential
  pkg-config
  ca-certificates
  zlib1g-dev
  libbz2-dev
  libfmt-dev
  libdbus-1-dev
  libsodium-dev
  libfreetype-dev
  libpng-dev
  libjpeg-dev
  libtiff5-dev
  libgeotiff-dev
  libc-ares-dev
  libcurl4-openssl-dev
  libssl-dev
  liblua5.4-dev
  libasound2-dev
  libdrm-dev
  libgbm-dev
  libgles2-mesa-dev
  libinput-dev
  libudev-dev
  libegl1-mesa-dev
  mesa-common-dev
)

PI_ARCH="${PI_ARCH:-armhf}"
PI_ROOT="${PI_ROOT:-}"
DEBIAN_SUITE="${DEBIAN_SUITE:-trixie}"
DEBIAN_MIRROR="${DEBIAN_MIRROR:-http://deb.debian.org/debian}"
INSTALL_HOST_DEPS="${INSTALL_HOST_DEPS:-1}"
FORCE=0
PACKAGES_ONLY=0

usage() {
  sed -n '2,19p' "$0" | sed 's/^# \{0,1\}//'
  cat <<EOF

Options:
  --arch armhf|arm64   Debian architecture (default: armhf, or PI_ARCH)
  --force              Remove an existing sysroot and recreate it
  --packages-only      Install dev packages into an existing sysroot (no debootstrap)
  --no-host-deps       Do not apt-get install tools on the host
  -h, --help           Show this help

Environment:
  PI_ARCH              armhf (default) or arm64
  PI_ROOT              Sysroot path (armhf: /opt/pi/root, arm64: /opt/pi64/root)
  DEBIAN_SUITE         Debian release (default: trixie)
  DEBIAN_MIRROR        Debian mirror URL
  INSTALL_HOST_DEPS    1 = install host packages (default), 0 = skip
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
  --arch)
    if [[ $# -lt 2 ]]; then
      echo "--arch needs armhf or arm64" >&2
      exit 1
    fi
    PI_ARCH="$2"
    shift 2
    ;;
  --force)
    FORCE=1
    shift
    ;;
  --no-host-deps)
    INSTALL_HOST_DEPS=0
    shift
    ;;
  --packages-only)
    PACKAGES_ONLY=1
    shift
    ;;
  -h | --help)
    usage
    exit 0
    ;;
  *)
    echo "Unknown option: $1" >&2
    usage >&2
    exit 1
    ;;
  esac
done

case "${PI_ARCH}" in
armhf)
  DEBIAN_ARCH=armhf
  QEMU_STATIC=qemu-arm-static
  MULTIARCH=arm-linux-gnueabihf
  PI_ROOT="${PI_ROOT:-/opt/pi/root}"
  XCSOAR_TARGET=PI2
  ;;
arm64)
  DEBIAN_ARCH=arm64
  QEMU_STATIC=qemu-aarch64-static
  MULTIARCH=aarch64-linux-gnu
  PI_ROOT="${PI_ROOT:-/opt/pi64/root}"
  XCSOAR_TARGET=PI4
  ;;
*)
  echo "Unknown architecture: ${PI_ARCH} (use armhf or arm64)" >&2
  exit 1
  ;;
esac

if [[ "${EUID}" -ne 0 ]]; then
  echo "Run as root (e.g. sudo $0)" >&2
  exit 1
fi

if ! command -v debootstrap >/dev/null; then
  echo "debootstrap not found. Run:" >&2
  echo "  sudo ${SCRIPT_DIR}/install-debian-packages.sh UPDATE PI_HOST" >&2
  exit 1
fi

is_unsafe_removal_path() {
  case "${PI_ROOT}" in
  / | /usr | /opt | /home | /var | /etc | /bin | /sbin | /lib | /lib64 | /root)
    return 0
    ;;
  esac
  [[ "${PI_ROOT}" != /* ]]
}

is_xcsoar_pi_sysroot() {
  [[ -f "${PI_ROOT}/.xcsoar-pi-sysroot" ]] || \
    [[ -f "${PI_ROOT}/debootstrap/debootstrap" ]] || \
    { [[ -d "${PI_ROOT}/usr/lib/${MULTIARCH}" ]] && \
      [[ -f "${PI_ROOT}/etc/debian_version" ]]; }
}

if [[ -d "${PI_ROOT}/usr" ]]; then
  if [[ "${FORCE}" -eq 1 ]]; then
    if is_unsafe_removal_path; then
      echo "Refusing to remove unsafe path: ${PI_ROOT}" >&2
      exit 1
    fi
    if ! is_xcsoar_pi_sysroot; then
      echo "Refusing --force: ${PI_ROOT} does not look like a Pi sysroot." >&2
      echo "Use a dedicated directory (armhf: /opt/pi/root, arm64: /opt/pi64/root)." >&2
      exit 1
    fi
    echo "Removing existing sysroot at ${PI_ROOT}"
    rm -rf "${PI_ROOT}"
  elif [[ "${PACKAGES_ONLY}" -eq 1 ]]; then
    echo "Installing packages into existing sysroot at ${PI_ROOT}"
  else
    echo "Sysroot already exists: ${PI_ROOT}" >&2
    echo "Use --force to recreate, --packages-only to install dev packages," >&2
    echo "or set PI_ROOT to another path." >&2
    exit 1
  fi
fi

install_sysroot_packages() {
  echo "Installing sysroot development packages..."
  DEBIAN_FRONTEND=noninteractive \
    chroot "${PI_ROOT}" apt-get update
  DEBIAN_FRONTEND=noninteractive \
    chroot "${PI_ROOT}" apt-get install -y --no-install-recommends \
    "${PI_SYSROOT_PACKAGES[@]}"
}

verify_sysroot() {
  PKG_CONFIG_LIBDIR="${PI_ROOT}/usr/lib/${MULTIARCH}/pkgconfig:${PI_ROOT}/usr/share/pkgconfig"
  echo "Verifying pkg-config entries in sysroot..."
  missing=0
  for pc in egl gbm libdrm libinput libudev bzip2 freetype2; do
    if ! PKG_CONFIG_LIBDIR="${PKG_CONFIG_LIBDIR}" \
      pkg-config --define-variable=prefix="${PI_ROOT}/usr" \
      --exists "${pc}"; then
      echo "pkg-config missing: ${pc}" >&2
      missing=1
    fi
  done

  if [[ "${missing}" -ne 0 ]]; then
    echo "Sysroot verification failed." >&2
    exit 1
  fi
}

if [[ "${INSTALL_HOST_DEPS}" == 1 ]]; then
  "${SCRIPT_DIR}/install-debian-packages.sh" UPDATE PI_HOST
fi

if ! command -v "${QEMU_STATIC}" >/dev/null; then
  echo "${QEMU_STATIC} not found (needed for chroot apt)." >&2
  exit 1
fi

if [[ "${PACKAGES_ONLY}" -eq 1 ]]; then
  if [[ ! -d "${PI_ROOT}/usr" ]]; then
    echo "Sysroot does not exist: ${PI_ROOT}" >&2
    echo "Create it first or drop --packages-only." >&2
    exit 1
  fi
  install_sysroot_packages
  echo "Cleaning apt caches in sysroot..."
  chroot "${PI_ROOT}" apt-get clean
  verify_sysroot
  touch "${PI_ROOT}/.xcsoar-pi-sysroot"
  exit 0
fi

mkdir -p "${PI_ROOT}"

echo "Bootstrapping ${DEBIAN_SUITE} ${DEBIAN_ARCH} into ${PI_ROOT}..."
debootstrap --arch="${DEBIAN_ARCH}" --foreign "${DEBIAN_SUITE}" "${PI_ROOT}" "${DEBIAN_MIRROR}"

QEMU_BIN="$(command -v "${QEMU_STATIC}")"
install -D -m 0755 "${QEMU_BIN}" "${PI_ROOT}/usr/bin/${QEMU_STATIC}"

echo "Running debootstrap second stage..."
DEBIAN_FRONTEND=noninteractive \
  chroot "${PI_ROOT}" /debootstrap/debootstrap --second-stage

install_sysroot_packages

echo "Cleaning apt caches in sysroot..."
chroot "${PI_ROOT}" apt-get clean

verify_sysroot

touch "${PI_ROOT}/.xcsoar-pi-sysroot"

cat <<EOF

Pi sysroot ready: ${PI_ROOT}

Cross-compile XCSoar:

  make -j\$(nproc) TARGET=${XCSOAR_TARGET} SYSROOT=${PI_ROOT} USE_CCACHE=y DEBUG=y
EOF

if [[ "${PI_ARCH}" == armhf ]]; then
  echo "For Pi 1 (ARMv6) use TARGET=PI.  TARGET=PI3 is an alias of PI2."
else
  echo "TARGET=PI5 is an alias of PI4 and writes to output/PI5/."
fi
