// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Id.hpp"

#include <fmt/core.h>

#include <stdlib.h>

FlarmId
FlarmId::Parse(const char *input, char **endptr_r) noexcept
{
  return FlarmId(strtol(input, endptr_r, 16));
}

const char *
FlarmId::Format(char *buffer) const noexcept
{
  *fmt::format_to(buffer, "{:X}", value) = 0;
  return buffer;
}
