// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MessagingRecord.hpp"
#include "util/StringCompare.hxx"
#include "util/StringStrip.hxx"
#include "util/StaticString.hxx"

bool
MessagingRecord::IsMissingFieldValue(std::string_view value) noexcept
{
  const std::string_view stripped = Strip(value);
  return stripped.empty() ||
    StringIsEqualIgnoreCase(stripped, std::string_view{"undefined"});
}

void
MessagingRecord::SanitizeFieldValue(Field f) noexcept
{
  auto &value = GetFieldValue(f);
  if (IsMissingFieldValue(value))
    value.clear();
}

void
MessagingRecord::SanitizeTextFields() noexcept
{
  for (uint8_t i = 0; i < GetFieldCount(); ++i)
    SanitizeFieldValue(static_cast<Field>(i));
}

const char *MessagingRecord::Format(StaticString<256> &buffer, const std::string &value) const noexcept {
  if (IsMissingFieldValue(value))
    return nullptr;
  return buffer.SetUTF8(value.c_str()) ? buffer.c_str() : nullptr;
}
