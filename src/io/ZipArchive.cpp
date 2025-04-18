// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ZipArchive.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "lib/fmt/PathFormatter.hpp"
#include "system/ConvertPathName.hpp"

#include <zzip/zzip.h>

ZipArchive::ZipArchive(Path path)
  :dir(zzip_dir_open(NarrowPathName(path), nullptr))
{
  if (dir == nullptr)
    throw FmtRuntimeError("Failed to open ZIP archive {}", path);
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
