// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Kernel.hpp"

#include <stdexcept>

#ifdef KOBO

#include "Model.hpp"
#include "io/FileReader.hxx"
#include "io/BufferedReader.hxx"
#include "system/FileUtil.hpp"
#include "lib/zlib/GunzipReader.hxx"

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

static bool
Copy(int out_fd, int in_fd, const char *out_path, const char *in_path)
{
  while (true) {
    char buffer[512];
    ssize_t nread = read(in_fd, buffer, sizeof(buffer));
    if (nread < 0) {
      fprintf(stderr, "Failed to read from %s: %s\n",
              in_path, strerror(errno));
      return false;
    }

    if (nread == 0)
      return true;

    ssize_t nwritten = write(out_fd, buffer, nread);
    if (nwritten < 0) {
      fprintf(stderr, "Failed to write to %s: %s\n",
              out_path, strerror(errno));
      return false;
    }

    if (nwritten != nread) {
      fprintf(stderr, "Short write to %s\n", out_path);
      return false;
    }
  }
}

bool
KoboInstallKernel(const char *uimage_path)
{
  const char *const out_path = "/dev/mmcblk0";

  const int in_fd = open(uimage_path, O_RDONLY|O_NOCTTY|O_CLOEXEC);
  if (in_fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", uimage_path, strerror(errno));
    return false;
  }

  const int out_fd = open(out_path, O_WRONLY|O_NOCTTY|O_CLOEXEC);
  if (out_fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", out_path, strerror(errno));
    close(in_fd);
    return false;
  }

  constexpr off_t out_offset = 2048 * 512;

  if (lseek(out_fd, out_offset, SEEK_SET) != out_offset) {
    fprintf(stderr, "Failed to seek %s\n", out_path);
    close(out_fd);
    close(in_fd);
    return false;
  }

  bool success = Copy(out_fd, in_fd, out_path, uimage_path);

  close(in_fd);

  fdatasync(out_fd);
  close(out_fd);

  return success;
}

#endif

bool
IsKoboCustomKernel()
try {
#ifdef KOBO
  KoboModel kobo_model = DetectKoboModel();
  /* All Kobo except Clara HD, Clara 2E, Libra 2 and Libra H2O have a factory kernel without OTG mode so
     a custom kernel is installed for OTG. */
  if (kobo_model == KoboModel::CLARA_HD || kobo_model == KoboModel::CLARA_2E
      || kobo_model == KoboModel::LIBRA2 || kobo_model == KoboModel::LIBRA_H2O)
        return false;

  FileReader file(Path("/proc/config.gz"));
  GunzipReader gunzip(file);
  BufferedReader reader(gunzip);

  char *line;
  while ((line = reader.ReadLine()) != nullptr)
    if (strcmp(line, "CONFIG_USB_EHCI_ARC_OTG=y") == 0)
      return true;
#endif

  return false;
} catch (const std::runtime_error &e) {
  return false;
}

bool
IsKoboOTGHostMode()
{
#ifdef KOBO
  KoboModel kobo_model = DetectKoboModel();
  if (kobo_model != KoboModel::CLARA_HD && kobo_model != KoboModel::CLARA_2E
      && kobo_model != KoboModel::LIBRA2 && kobo_model != KoboModel::LIBRA_H2O)
        return IsKoboCustomKernel();
  /* for Clara HD, Libra 2 and Libra H2O read the mode from the debugfs */
  char buffer[5];
  bool success = File::ReadString(Path("/sys/kernel/debug/ci_hdrc.0/role"),
                   buffer, 5);
  if (success && (strcmp(buffer, "host") == 0))
    return true;
#endif
  return false;
}
