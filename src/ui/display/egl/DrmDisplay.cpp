/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "DrmDisplay.hpp"
#include "Hardware/DisplayDPI.hpp"
#include "system/Error.hxx"

#include <span>

#include <fcntl.h>

namespace EGL {

#ifdef RASPBERRY_PI
/* on the Raspberry Pi 4, /dev/dri/card1 is the VideoCore IV (the
   "legacy mode") which we want to use for now), and /dev/dri/card0 is
   V3D which I havn't figured out yet */
static constexpr const char *DEFAULT_DRI_DEVICE = "/dev/dri/card1";
#else
constexpr const char * DEFAULT_DRI_DEVICE = "/dev/dri/card0";
#endif

static UniqueFileDescriptor
OpenDriDevice()
{
  const char *dri_device = getenv("DRI_DEVICE");
  if (nullptr == dri_device)
    dri_device = DEFAULT_DRI_DEVICE;
  printf("Using DRI device %s (use environment variable "
           "DRI_DEVICE to override)\n",
         dri_device);

  UniqueFileDescriptor dri_fd;
  if (!dri_fd.Open(dri_device, O_RDWR))
    throw FormatErrno("Could not open DRI device %s", dri_device);

  return dri_fd;
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

  if (connector->mmWidth > 0 && connector->mmHeight > 0)
    Display::ProvideSizeMM(mode.hdisplay, mode.vdisplay,
                           connector->mmWidth,
                           connector->mmHeight);

  drmModeFreeConnector(connector);
}

DrmDisplay::~DrmDisplay() noexcept = default;

} // namespace EGL
