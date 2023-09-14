// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HexColor.hpp"
#include "ui/canvas/PortableColor.hpp"
#include "util/NumberParser.hpp"

#include <stdio.h>
#include <cassert>

void
FormatHexColor(char *buffer, size_t size, const RGB8Color color)
{
  assert(size >= 7);

  snprintf(buffer, size, "#%02X%02X%02X",
           color.Red(), color.Green(), color.Blue());
}

bool
ParseHexColor(const char *buffer, RGB8Color &color)
{
  if (*buffer != '#')
    return false;

  buffer++;

  char *endptr;
  unsigned value = ParseUnsigned(buffer, &endptr, 16);
  if (endptr != buffer + 6)
    return false;

  uint8_t r = value >> 16;
  uint8_t g = value >> 8;
  uint8_t b = value;

  color = RGB8Color(r, g, b);
  return true;
}

#ifdef _UNICODE

bool
ParseHexColor(const TCHAR *buffer, RGB8Color &color)
{
  if (*buffer != _T('#'))
    return false;

  buffer++;

  TCHAR *endptr;
  unsigned value = ParseUnsigned(buffer, &endptr, 16);
  if (endptr != buffer + 6)
    return false;

  uint8_t r = value >> 16;
  uint8_t g = value >> 8;
  uint8_t b = value;

  color = RGB8Color(r, g, b);
  return true;
}

#endif
