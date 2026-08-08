// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ZipArchive.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "lib/fmt/PathFormatter.hpp"
#include "system/Path.hpp"

#include <zzip/zzip.h>

#ifdef _WIN32
#include "UniqueFileDescriptor.hxx"
#endif

ZipArchive::ZipArchive(Path path)
{
#ifdef _WIN32
  /* zzip_dir_open() uses CRT open() (ANSI); open via _wopen and hand
     the fd to zzip so UTF-8 Paths work. Steal only after success so a
     failed zzip_dir_fdopen (e.g. OOM before it adopts the fd) cannot
     leak the descriptor. */
  UniqueFileDescriptor fd;
  if (!fd.OpenReadOnly(path.c_str()))
    throw FmtRuntimeError("Failed to open ZIP archive {}", path);

  dir = zzip_dir_fdopen(fd.Get(), nullptr);
  if (dir == nullptr)
    throw FmtRuntimeError("Failed to open ZIP archive {}", path);

  (void)fd.Steal();
#else
  dir = zzip_dir_open(path.c_str(), nullptr);
  if (dir == nullptr)
    throw FmtRuntimeError("Failed to open ZIP archive {}", path);
#endif
}

ZipArchive::~ZipArchive() noexcept
{
  if (dir != nullptr)
    zzip_dir_close(dir);
}

bool
ZipArchive::Exists(const char *name) const noexcept
{
  ZZIP_STAT st;
  return zzip_dir_stat(dir, name, &st, 0) == 0;
}

std::string
ZipArchive::NextName() noexcept
{
  ZZIP_DIRENT e;
  return zzip_dir_read(dir, &e)
    ? std::string(e.d_name)
    : std::string();
}
