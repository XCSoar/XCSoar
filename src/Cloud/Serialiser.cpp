// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Serialiser.hpp"

#include <algorithm>
#include <stdexcept>

#include <string.h>

void
Serialiser::WriteString(const char *s)
{
  size_t length = std::min<size_t>(strlen(s), 0xff00);
  Write16(length);
  Write(s, length);
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
