// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MessagingRecord.hpp"
#include "util/StaticString.hxx"

const char *MessagingRecord::Format(StaticString<256> &buffer, const std::string &value) const noexcept {
  if (value.empty())
    return nullptr;
  return buffer.SetUTF8(value.c_str()) ? buffer.c_str() : nullptr;
}
