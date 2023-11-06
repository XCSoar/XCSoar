// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Reader.hxx"

#include <cstdint>

struct zzip_file;
struct zzip_dir;

class ZipReader final : public Reader {
  struct zzip_file *const file;

public:
  /**
   * Throws std::runtime_errror on error.
   */
  ZipReader(struct zzip_dir *dir, const char *path);

  virtual ~ZipReader();

  [[gnu::pure]]
  uint_least64_t GetSize() const;

  [[gnu::pure]]
  uint_least64_t GetPosition() const;

  /* virtual methods from class Reader */
  std::size_t Read(std::span<std::byte> dest) override;
};
