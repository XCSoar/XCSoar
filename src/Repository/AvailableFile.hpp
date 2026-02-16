// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"
#include "FileType.hpp"
#include "time/BrokenDate.hpp"

#include <string>

/**
 * The description of a file that is available in a remote repository.
 */
struct AvailableFile {
  /**
   * Base name of the file.
   */
  std::string name;

  /**
   * Description of the file.
   */
  std::string description;

  /**
   * Absolute HTTP URI.
   */
  std::string uri;

  /**
   * A short symbolic name for the area.  Empty means this file is
   * global.
   */
  StaticString<8> area;

  FileType type;

  BrokenDate update_date;

  /**
  * The SHA256 hash of the contents of this file.
  * Zeroed if no hash is available.
  */
  std::array<std::byte, 32> sha256_hash;

  bool IsEmpty() const {
    return name.empty();
  }

  bool IsValid() const {
    return !name.empty() && !uri.empty();
  }

  bool HasHash() const {
    for (std::size_t i = 0; i < sha256_hash.size(); i++) {
      if (sha256_hash[i] != std::byte{0})
        return true;
    }

    return false;
  }

  void Clear() {
    name.clear();
    uri.clear();
    area.clear();
    type = FileType::UNKNOWN;
    update_date = BrokenDate::Invalid();
    sha256_hash.fill(std::byte{0});
  }

  const char *GetName() const {
    return name.c_str();
  }

  const char *GetDescription() const {
    return description.c_str();
  }

  const char *GetURI() const {
    return uri.c_str();
  }

  const char *GetArea() const {
    return area;
  }
};
