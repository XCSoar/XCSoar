// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Serialiser.hpp"
#include "util/SpanCast.hxx"

#include <stdexcept>

#include <string.h>

void
Serialiser::WriteString(std::string_view s)
{
  if (s.size() > 0xff00)
    s = s.substr(0, 0xff00);

  Write16(s.size());
  Write(AsBytes(s));
}

std::string
Deserialiser::ReadString()
{
  size_t length = Read16();
  if (length > 0xff00)
    throw std::runtime_error("Malformed string");

  const char *data = (const char *)ReadFull(length);
  Consume(length);
  return std::string(data, length);
}
