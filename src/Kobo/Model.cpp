// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Model.hpp"

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

static bool
ReadFromFile(const char *path, off_t offset,
             void *buffer, size_t size) noexcept
{
  const int fd = open(path, O_RDONLY|O_NOCTTY|O_CLOEXEC);
  if (fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
    return false;
  }

  bool success = false;
  ssize_t nbytes = pread(fd, buffer, size, offset);
  if (nbytes < 0)
    fprintf(stderr, "Failed to read from %s: %s\n", path, strerror(errno));
  else if (size_t(nbytes) != size)
    fprintf(stderr, "Short read from %s\n", path);
  else
    success = true;

  close(fd);
  return success;
}

static constexpr struct {
  const char *id;
  KoboModel model;
} kobo_model_ids[] = {
  { "SN-N514", KoboModel::AURA },
  { "SN-N236", KoboModel::AURA2 },
  { "SN-N705", KoboModel::MINI },
  { "SN-N905", KoboModel::TOUCH },
  { "SN-N587", KoboModel::TOUCH2 },
  { "SN-613A4", KoboModel::GLO },
  { "SN-R13A5", KoboModel::GLO },
  { "SN-N437", KoboModel::GLO_HD },
  { "SN-RN437", KoboModel::GLO_HD },
  { "SN-N249", KoboModel::CLARA_HD },
  { "SN-N506", KoboModel::CLARA_2E },
  { "SN-N365", KoboModel::CLARA_BW },
  { "SN-P365", KoboModel::CLARA_BW },
  { "SN-N367", KoboModel::CLARA_COLOUR },
  { "SN-N306", KoboModel::NIA },
  { "SN-N418", KoboModel::LIBRA2 },
  { "SN-N873", KoboModel::LIBRA_H2O },
};

KoboModel
ParseKoboModel(std::span<const char> data) noexcept
{
  for (const auto &i : kobo_model_ids) {
    const size_t id_length = strlen(i.id);
    for (size_t offset = 0; offset + id_length <= data.size(); ++offset)
      if (memcmp(data.data() + offset, i.id, id_length) == 0)
        return i.model;
  }

  return KoboModel::UNKNOWN;
}

KoboModel
DetectKoboModel() noexcept
{
  char buffer[16];
  if (ReadFromFile("/dev/mmcblk0", 0x200, buffer, sizeof(buffer))) {
    const KoboModel model = ParseKoboModel(buffer);
    if (model != KoboModel::UNKNOWN)
      return model;
  }

  char hwcfg[512];
  if (ReadFromFile("/dev/disk/by-partlabel/hwcfg", 0,
                   hwcfg, sizeof(hwcfg)))
    return ParseKoboModel(hwcfg);

  return KoboModel::UNKNOWN;
}

bool
IsKoboMediaTek() noexcept
{
  return IsKoboMediaTek(DetectKoboModel());
}

const char *
GetKoboWifiInterface(KoboModel model) noexcept
{
  switch (model) {
  case KoboModel::LIBRA2:
  case KoboModel::CLARA_BW:
  case KoboModel::CLARA_COLOUR:
    return "wlan0";

  case KoboModel::CLARA_2E:
    return "mlan0";

  default:
    return "eth0";
  }
}

const char *
GetKoboWifiInterface() noexcept
{
  return GetKoboWifiInterface(DetectKoboModel());
}
