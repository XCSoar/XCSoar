// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StringBuffer.hxx"
#include "Math/Angle.hpp"
#include "util/Compiler.h"

#include <cstddef>

class Angle;

void
FormatBearing(char *buffer, size_t size, unsigned degrees_value,
              const char *suffix = NULL);

void
FormatBearing(char *buffer, size_t size, Angle value,
              const char *suffix = NULL);

[[gnu::const]]
static inline BasicStringBuffer<char, 16>
FormatBearing(unsigned degrees_value)
{
  BasicStringBuffer<char, 16> buffer;
  FormatBearing(buffer.data(), buffer.capacity(), degrees_value);
  return buffer;
}

[[gnu::const]]
static inline BasicStringBuffer<char, 16>
FormatBearing(Angle value)
{
  BasicStringBuffer<char, 16> buffer;
  FormatBearing(buffer.data(), buffer.capacity(), value);
  return buffer;
}

void
FormatAngleDelta(char *buffer, size_t size, Angle value);

[[gnu::const]]
static inline BasicStringBuffer<char, 16>
FormatAngleDelta(Angle value)
{
  BasicStringBuffer<char, 16> buffer;
  FormatAngleDelta(buffer.data(), buffer.capacity(), value);
  return buffer;
}

void
FormatVerticalAngleDelta(char *buffer, size_t size, Angle value);
