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
  uint64_t GetSize() const;

  [[gnu::pure]]
  uint64_t GetPosition() const;

  /* virtual methods from class Reader */
  std::size_t Read(void *data, std::size_t size) override;
};
