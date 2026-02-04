// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#pragma once

#include "io/Reader.hxx"
#include "io/OutputStream.hxx"
#include "system/Path.hpp"

#include <optional>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>

class StorageDevice {
public:
  enum class Kind {
    Internal,
    Removable
  };

  virtual ~StorageDevice() = default;

  /* stable identifier */
  virtual std::string id() const = 0;
  virtual std::string name() const = 0;
  virtual std::string label() const { 
    return name(); 
  }
  virtual Kind kind() const = 0;

  virtual bool isWritable() const = 0;

  /**
   * Open a file inside the storage device.
   *
   * @param path path relative to the storage device root
   */
  virtual std::unique_ptr<Reader>
  openRead(Path path) const = 0;

  /**
   * Open a file for writing inside the storage device.
   *
   * @param path path relative to the storage device root
   */
  virtual std::unique_ptr<OutputStream>
  openWrite(Path path, bool truncate = true) = 0;

  /**
   * Filesystem/device space information (total and free bytes).
   * Returns empty optional if the information is not available for this
   * device (e.g. device removed or not mounted).
   */
  struct Space {
    uint64_t total_bytes;
    uint64_t free_bytes; /* available to unprivileged users */
  };

  virtual std::optional<Space> space() const noexcept {
    return std::nullopt;
  }
};
