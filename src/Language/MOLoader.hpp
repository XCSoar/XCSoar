// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MOFile.hpp"
#include "io/FileMapping.hpp"
#include "system/Path.hpp"

#include <memory>

/**
 * Loader for GNU gettext *.mo files.
 */
class MOLoader {
  std::unique_ptr<FileMapping> mapping;
  std::unique_ptr<MOFile> file;

public:
  explicit MOLoader(std::span<const std::byte> raw)
    :file(new MOFile(raw)) {}

  explicit MOLoader(Path path)
    :mapping(new FileMapping(path)),
     file(new MOFile(*mapping)) {
  }

  bool error() const {
    return file == nullptr || file->error();
  }

  const MOFile &get() const {
    return *file;
  }
};
