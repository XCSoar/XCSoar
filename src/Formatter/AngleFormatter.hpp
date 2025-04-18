// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StringBuffer.hxx"
#include "Math/Angle.hpp"
#include "util/Compiler.h"

#include <tchar.h>
#include <cstddef>

class Angle;

void
FormatBearing(TCHAR *buffer, size_t size, unsigned degrees_value,
              const TCHAR *suffix = NULL);

void
FormatBearing(TCHAR *buffer, size_t size, Angle value,
              const TCHAR *suffix = NULL);

[[gnu::const]]
static inline BasicStringBuffer<TCHAR, 16>
FormatBearing(unsigned degrees_value)
{
  BasicStringBuffer<TCHAR, 16> buffer;
  FormatBearing(buffer.data(), buffer.capacity(), degrees_value);
  return buffer;
}

[[gnu::const]]
static inline BasicStringBuffer<TCHAR, 16>
FormatBearing(Angle value)
{
  BasicStringBuffer<TCHAR, 16> buffer;
  FormatBearing(buffer.data(), buffer.capacity(), value);
  return buffer;
}

void
FormatAngleDelta(TCHAR *buffer, size_t size, Angle value);

[[gnu::const]]
static inline BasicStringBuffer<TCHAR, 16>
FormatAngleDelta(Angle value)
{
  BasicStringBuffer<TCHAR, 16> buffer;
  FormatAngleDelta(buffer.data(), buffer.capacity(), value);
  return buffer;
}

void
FormatVerticalAngleDelta(TCHAR *buffer, size_t size, Angle value);
