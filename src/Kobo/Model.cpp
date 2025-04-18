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
  { "SN-N306", KoboModel::NIA },
  { "SN-N418", KoboModel::LIBRA2 },
  { "SN-N873", KoboModel::LIBRA_H2O },
};

static KoboModel
DetectKoboModel(const char *p) noexcept
{
  for (const auto &i : kobo_model_ids)
    if (memcmp(p, i.id, strlen(i.id)) == 0)
      return i.model;

  return KoboModel::UNKNOWN;
}

KoboModel
DetectKoboModel() noexcept
{
  char buffer[16];
  if (!ReadFromFile("/dev/mmcblk0", 0x200, buffer, sizeof(buffer)))
    return KoboModel::UNKNOWN;

  return DetectKoboModel(buffer);
}

const char *
GetKoboWifiInterface() noexcept
{
  switch (DetectKoboModel())
  {
    case KoboModel::LIBRA2:
      return "wlan0";
    case KoboModel::CLARA_2E:
      return "mlan0";
    default:
      return "eth0";
  }
}
