// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AngleFormatter.hpp"
#include "Math/Angle.hpp"
#include "util/StringFormat.hpp"
#include "Language/Language.hpp"

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

void
FormatBearingCompass(TCHAR *buffer, size_t size, unsigned angle, int level)
{
  assert(buffer != NULL);
  assert (size >= 4 );

  const TCHAR* table[3][16]={
                      {N_("N"),N_("E"),N_("S"),N_("W")},
                      {_T("N"),N_("NE"),_T("E"),N_("SE"),_T("S"),N_("SW"),_T("W"),N_("NW")},
                      {_T("N"),N_("NNE"),_T("NE"),N_("ENE"),_T("E"),N_("ESE"),_T("SE"),N_("SSE"),
		       _T("S"),N_("SSW"),_T("SW"),N_("WSW"),_T("W"),N_("WNW"),_T("NW"),N_("NNW")}};

_tcscpy(buffer, gettext( table[level][(int) (fmod((angle + 45/pow(2,level)),360 ) /
			                             (90/pow(2,level)))] ));
}

void
FormatBearingCompass(TCHAR *buffer, size_t size, Angle angle, int level)
{
  FormatBearingCompass(buffer, size, angle.AsBearing().Degrees(),level);
}
