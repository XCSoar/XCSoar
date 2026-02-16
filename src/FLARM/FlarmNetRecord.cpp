// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlarmNetRecord.hpp"
const char *
FlarmNetRecord::Format([[maybe_unused]] StaticString<256> &buffer, 
                        const char *value) const noexcept
{
  return (value != nullptr && !*value) ? nullptr : value;
}
