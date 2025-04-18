// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AngleFormatter.hpp"
#include "Math/Angle.hpp"
#include "util/StringFormat.hpp"

#include <cassert>
#include <string.h>

void
FormatBearing(TCHAR *buffer, size_t size, unsigned value_degrees,
              const TCHAR *suffix)
{
  assert(buffer != NULL);
  assert(size >= 8);

  if (suffix != NULL)
    StringFormat(buffer, size, _T("%u° %s"), value_degrees, suffix);
  else
    StringFormat(buffer, size, _T("%u°"), value_degrees);
}

void
FormatBearing(TCHAR *buffer, size_t size, Angle value, const TCHAR *suffix)
{
  FormatBearing(buffer, size, lround(value.AsBearing().Degrees()), suffix);
}

void
FormatAngleDelta(TCHAR *buffer, size_t size, Angle value)
{
  assert(buffer != NULL);
  assert(size >= 8);

  auto degrees = lround(value.AsDelta().Degrees());
  if (degrees > 1)
    StringFormat(buffer, size, _T("%u°»"), unsigned(degrees));
  else if (degrees < -1)
    StringFormat(buffer, size, _T("«%u°"), unsigned(-degrees));
  else
    _tcscpy(buffer, _T("«»"));
}

void
FormatVerticalAngleDelta(TCHAR *buffer, size_t size, Angle value)
{
  assert(buffer != NULL);
  assert(size >= 8);

  auto degrees = lround(value.AsDelta().Degrees());
  if (degrees < -1 || degrees > 1)
    StringFormat(buffer, size, _T("%+d°"), int(degrees));
  else
    _tcscpy(buffer, _T("--"));
}
