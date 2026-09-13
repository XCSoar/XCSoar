// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string>
#include <string_view>

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

struct Descriptor {
  std::string name;
  std::string description;
  std::string instructions;
  Mode mode = Mode::COUNT;
  Confirmation confirmation = Confirmation::REQUIRED;
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

} // namespace SystemTools
