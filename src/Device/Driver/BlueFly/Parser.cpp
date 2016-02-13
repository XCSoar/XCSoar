/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Device/Driver/BlueFlyVario.hpp"
#include "Internal.hpp"

bool
BlueFlyDevice::ParseBAT(const char *content, NMEAInfo &info)
{
  // e.g.
  // BAT 1234

  char *endptr;
  int mV = (int)strtol(content, &endptr, 16);
  if (endptr == content) return true;

  do {
    // piecewise linear approximation
    if (mV > 3900) {
      info.battery_level = 70 + (mV - 3900) / 10.;
      break;
    }
    if (mV > 3700) {
      info.battery_level = 4 + (mV - 3700) / 3.;
      break;
    }
    if (mV > 3600) {
      info.battery_level = 0.04 * (mV - 3600);
      break;
    }
    // considered empty ...
    info.battery_level = 0;
    break;
  }  while (0);

  if (info.battery_level > 100)
    info.battery_level = 100;
  info.battery_level_available.Update(info.clock);

  return true;
}

gcc_pure
static inline double
ComputeNoncompVario(const double pressure, const double d_pressure)
{
  static constexpr double FACTOR(-2260.389548275485);
  static constexpr double EXPONENT(-0.8097374740609689);
  return FACTOR * pow(pressure, EXPONENT) * d_pressure;
}

bool
BlueFlyDevice::ParsePRS(const char *content, NMEAInfo &info)
{
  // e.g. PRS 17CBA

  char *endptr;
  long value = strtol(content, &endptr, 16);
  if (endptr != content) {
    AtmosphericPressure pressure = AtmosphericPressure::Pascal(value);

    kalman_filter.Update(pressure.GetHectoPascal(), 0.25, 0.02);

    info.ProvideNoncompVario(ComputeNoncompVario(kalman_filter.GetXAbs(),
                                                 kalman_filter.GetXVel()));
    info.ProvideStaticPressure(AtmosphericPressure::HectoPascal(kalman_filter.GetXAbs()));
  }

  return true;
}

static bool
ParseUlong(const char **line, unsigned long &value)
{
  char *endptr;
  value = strtoul(*line, &endptr, 10);

  if (endptr == *line)
    return false;

  *line = endptr;

  return true;
}

/**
 * Parse the BlueFly Vario version.
 * Sent Upon a BST request.
 */
bool
BlueFlyDevice::ParseBFV(const char *content, NMEAInfo &info)
{
  // e.g. BFV 9

  unsigned long value;

  if (ParseUlong(&content, value)) {
    assert(value <= UINT_MAX);
    settings.version = value;
  }

  return true;
}

/**
 * Parse the BlueFly Vario parameter list.
 * Sent Upon a BST request.
 */
bool
BlueFlyDevice::ParseBST(const char *content, NMEAInfo &info)
{
  // e.g. BST BFK BFL BFP BAC BAD BTH BFQ BFI BSQ BSI BFS BOL BOS BRM BVL BOM BOF BQH BRB BPT BUR BLD BR2

  free(settings_keys);

  settings_keys = strdup(content);

  return true;
}

/**
 * Parse the BlueFly Vario parameter values.
 * Sent Upon a BST request.
 */
bool
BlueFlyDevice::ParseSET(const char *content, NMEAInfo &info)
{
  // e.g. SET 0 100 20 1 1 1 180 1000 100 400 100 20 5 5 100 50 0 10 21325 207 1 0 1 34

  const char *values, *token;
  unsigned long value;

  if (!settings.version || !settings_keys)
    /* we did not receive the previous BFV and BST, abort */
    return true;

  token = StringToken(settings_keys, " ");
  values = content;

  /* the first value should be ignored */
  if (!ParseUlong(&values, value))
    return true;

  mutex_settings.Lock();
  while (token && ParseUlong(&values, value)) {
    settings.Parse(token, value);
    token = StringToken(nullptr, " ");
  }
  settings_ready = true;
  settings_cond.broadcast();
  mutex_settings.Unlock();

  return true;
}

bool
BlueFlyDevice::ParseNMEA(const char *line, NMEAInfo &info)
{
  if (StringIsEqual(line, "PRS ", 4))
    return ParsePRS(line + 4, info);
  else if (StringIsEqual(line, "BAT ", 4))
    return ParseBAT(line + 4, info);
  else if (StringIsEqual(line, "BFV ", 4))
    return ParseBFV(line + 4, info);
  else if (StringIsEqual(line, "BST ", 4))
    return ParseBST(line + 4, info);
  else if (StringIsEqual(line, "SET ", 4))
    return ParseSET(line + 4, info);
  else
    return false;
}
