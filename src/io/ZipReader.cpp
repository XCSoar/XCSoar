// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ZipReader.hpp"

#include <zzip/util.h>

#include <stdexcept>

#include <stdio.h>

ZipReader::ZipReader(struct zzip_dir *dir, const char *path)
  :file(zzip_open_rb(dir, path))
{
  if (file == nullptr) {
    /* TODO: re-enable zziplib's error reporting, and improve this
       error message */
    char msg[256];
    snprintf(msg, sizeof(msg),
             "Failed to open '%s' from ZIP file", path);
    throw std::runtime_error(msg);
  }
}

ZipReader::~ZipReader()
{
  if (file != nullptr)
    zzip_close(file);
}

uint_least64_t
ZipReader::GetSize() const
{
  ZZIP_STAT st;
  return zzip_file_stat(file, &st) >= 0
    ? st.st_size
    : 0;
}

uint_least64_t
ZipReader::GetPosition() const
{
  return zzip_tell(file);
}

std::size_t
ZipReader::Read(std::span<std::byte> dest)
{
  zzip_ssize_t nbytes = zzip_file_read(file, dest.data(), dest.size());
  if (nbytes < 0)
    throw std::runtime_error("Failed to read from ZIP file");
  return nbytes;
}
