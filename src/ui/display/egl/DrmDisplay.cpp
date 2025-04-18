// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DrmDisplay.hpp"
#include "Hardware/DisplayDPI.hpp"
#include "lib/fmt/SystemError.hxx"

#include <span>

#include <fcntl.h>

namespace EGL {

static UniqueFileDescriptor
OpenDriDevice(const char *path)
{
  UniqueFileDescriptor fd;
  if (!fd.Open(path, O_RDWR))
    throw FmtErrno("Could not open DRI device {}", path);

  /* check if this card works */
  drmModeRes *resources = drmModeGetResources(fd.Get());
  if (resources == nullptr)
    throw FmtErrno("drmModeGetResources() for DRI device {} failed", path);

  return fd;
}

static auto
OpenDriDevice()
{
  if (const char *dri_device = getenv("DRI_DEVICE"))
    return OpenDriDevice(dri_device);

  /* some computers like the Raspberry Pi and the CubieBoard have two
     cards in /dev/dri/, of which only one works, but it's hard to
     tell which one; probe both */
  try {
    return OpenDriDevice("/dev/dri/card0");
  } catch (...) {
    try {
      return OpenDriDevice("/dev/dri/card1");
    } catch (...) {
    }

    /* if the second one fails, report the error for the first one */
    throw;
  }
}

static drmModeConnector *
ChooseConnector(FileDescriptor dri_fd,
                const std::span<const uint32_t> connectors)
{
  for (const auto id : connectors) {
    auto *connector = drmModeGetConnector(dri_fd.Get(), id);
    if (connector != nullptr && connector->connection == DRM_MODE_CONNECTED &&
        connector->count_modes > 0)
      return connector;

    drmModeFreeConnector(connector);
  }

  throw std::runtime_error("No usable DRM connector found");
}

static drmModeConnector *
ChooseConnector(FileDescriptor dri_fd, const drmModeRes &resources)
{
  const std::span connectors{
    resources.connectors, std::size_t(resources.count_connectors),
  };

  return ChooseConnector(dri_fd, connectors);
}

DrmDisplay::DrmDisplay()
  :dri_fd(OpenDriDevice())
{
  drmModeRes *resources = drmModeGetResources(dri_fd.Get());
  if (resources == nullptr)
    throw MakeErrno("drmModeGetResources() failed");

  auto *connector = ChooseConnector(dri_fd, *resources);
  connector_id = connector->connector_id;

  if (auto *encoder = drmModeGetEncoder(dri_fd.Get(), connector->encoder_id)) {
    crtc_id = encoder->crtc_id;
    drmModeFreeEncoder(encoder);
  } else
    throw std::runtime_error("No usable DRM encoder found");

  mode = connector->modes[0];

  size_mm = {connector->mmWidth, connector->mmHeight};

  drmModeFreeConnector(connector);
}

DrmDisplay::~DrmDisplay() noexcept = default;

void
DrmDisplay::SetMaster()
{
  if (drmSetMaster(dri_fd.Get()) != 0)
    throw MakeErrno("DRM_IOCTL_SET_MASTER failed");
}

void
DrmDisplay::DropMaster() noexcept
{
  drmDropMaster(dri_fd.Get());
}

} // namespace EGL
