// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

class Reader;
class OutputStream;

class StorageDevice {
public:
  enum class Kind {
    Internal,
    Removable,
    COUNT
  };

  virtual ~StorageDevice() = default;

  /* stable identifier */
  virtual std::string Id() const = 0;
  virtual std::string Name() const = 0;
  virtual std::string Label() const { 
    return Name(); 
  }
  virtual Kind GetKind() const = 0;

  virtual bool IsWritable() const = 0;

  /**
   * Open a file inside the storage device.
   *
   * @param path path relative to the storage device root
   */
  virtual std::unique_ptr<Reader>
  OpenRead(Path path) const = 0;

  /**
   * Open a file for writing inside the storage device.
   *
   * @param path path relative to the storage device root
   */
  virtual std::unique_ptr<OutputStream>
  OpenWrite(Path path, bool truncate = true) = 0;

  /**
   * Filesystem/device space information (total and free bytes).
   * Returns empty optional if the information is not available for this
   * device (e.g. device removed or not mounted).
   */
  struct Space {
    uint64_t total_bytes;
    uint64_t free_bytes; /* available to unprivileged users */
  };

  virtual std::optional<Space> GetSpace() const noexcept {
    return std::nullopt;
  }
};
