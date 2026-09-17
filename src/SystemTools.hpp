// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

class NLineReader;

namespace SystemTools {

enum class Mode {
  JOB,
  EXCLUSIVE,
  COUNT,
};

enum class Confirmation {
  REQUIRED,
  NONE,
  COUNT,
};

enum class Origin {
  SYSTEM,
  USER,
};

struct Descriptor {
  std::string name;
  std::string description;
  std::string instructions;
  Mode mode = Mode::COUNT;
  Confirmation confirmation = Confirmation::REQUIRED;
};

struct InstalledTool {
  std::string id;
  Descriptor descriptor;
  AllocatedPath executable;
  Origin origin;
};

/**
 * Is this a valid descriptor base name?
 *
 * Tool identifiers are deliberately restricted so they can safely be used
 * to derive the descriptor and executable names inside a fixed directory.
 */
[[gnu::pure]]
bool
IsValidId(std::string_view id) noexcept;

/**
 * Parse one system-tool descriptor.
 *
 * The default name, description and instructions are mandatory.  A localized
 * value such as name[de] overrides its default when @p language is "de".
 * Throws std::runtime_error if the descriptor is malformed.
 */
Descriptor
ParseDescriptor(NLineReader &reader, std::string_view language = {});

/**
 * Load usable tools from the configured installation directories and the
 * optional user directory.
 *
 * Unsafe or malformed entries are ignored and logged.  This function never
 * throws because it is also used to decide whether the UI page is available.
 */
std::vector<InstalledTool>
LoadInstalled(std::string_view language = {},
              Path user_directory = Path{nullptr}) noexcept;

} // namespace SystemTools
