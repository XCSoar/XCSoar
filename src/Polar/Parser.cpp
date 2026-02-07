// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Parser.hpp"
#include "Polar.hpp"
#include "util/NumberParser.hpp"
#include "Units/System.hpp"

#include <cstdio>
#include <tchar.h>

bool
ParsePolarShape(PolarShape &shape, const char *s) noexcept
{
  char *p;
  auto v1 = Units::ToSysUnit(ParseDouble(s, &p), Unit::KILOMETER_PER_HOUR);
  if (*p != ',')
    return false;

  auto w1 = ParseDouble(p + 1, &p);
  if (*p != ',')
    return false;

  auto v2 = Units::ToSysUnit(ParseDouble(p + 1, &p), Unit::KILOMETER_PER_HOUR);
  if (*p != ',')
    return false;

  auto w2 = ParseDouble(p + 1, &p);
  if (*p != ',')
    return false;

  auto v3 = Units::ToSysUnit(ParseDouble(p + 1, &p), Unit::KILOMETER_PER_HOUR);
  if (*p != ',')
    return false;

  auto w3 = ParseDouble(p + 1, &p);
  if (*p != '\0')
    return false;

  shape[0].v = v1;
  shape[0].w = w1;
  shape[1].v = v2;
  shape[1].w = w2;
  shape[2].v = v3;
  shape[2].w = w3;
  return true;
}

bool
ParsePolar(PolarInfo &polar_r, const char *s) noexcept
{
  PolarInfo polar;
  // Example:
  // *LS-3  WinPilot POLAR file: MassDryGross[kg], MaxWaterBallast[liters], Speed1[km/h], Sink1[m/s], Speed2, Sink2, Speed3, Sink3
  // 403, 101, 115.03, -0.86, 174.04, -1.76, 212.72,  -3.4

  if (s[0] == '*')
    /* a comment */
    return false;

  char *p;
  polar.shape.reference_mass = ParseDouble(s, &p);
  if (*p != ',')
    return false;

  polar.max_ballast = ParseDouble(p + 1, &p);
  if (*p != ',')
    return false;

  polar.shape[0].v = Units::ToSysUnit(ParseDouble(p + 1, &p), Unit::KILOMETER_PER_HOUR);
  if (*p != ',')
    return false;

  polar.shape[0].w = ParseDouble(p + 1, &p);
  if (*p != ',')
    return false;

  polar.shape[1].v = Units::ToSysUnit(ParseDouble(p + 1, &p), Unit::KILOMETER_PER_HOUR);
  if (*p != ',')
    return false;

  polar.shape[1].w = ParseDouble(p + 1, &p);
  if (*p != ',')
    return false;

  polar.shape[2].v = Units::ToSysUnit(ParseDouble(p + 1, &p), Unit::KILOMETER_PER_HOUR);
  if (*p != ',')
    return false;

  polar.shape[2].w = ParseDouble(p + 1, &p);
  polar.wing_area = (*p != ',') ? 0. : ParseDouble(p + 1, &p);
  polar.v_no = (*p != ',') ? 0. : ParseDouble(p + 1, &p);

  polar_r = polar;
  return true;
}

void
FormatPolarShape(const PolarShape &shape, char *buffer, size_t max_size) noexcept
{
  double v1, v2, v3;
  v1 = Units::ToUserUnit(shape[0].v, Unit::KILOMETER_PER_HOUR);
  v2 = Units::ToUserUnit(shape[1].v, Unit::KILOMETER_PER_HOUR);
  v3 = Units::ToUserUnit(shape[2].v, Unit::KILOMETER_PER_HOUR);

  snprintf(buffer, max_size, "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f",
           (double)v1, (double)shape[0].w,
           (double)v2, (double)shape[1].w,
           (double)v3, (double)shape[2].w);
}

void
FormatPolar(const PolarInfo &polar, char *buffer, size_t max_size,
            bool include_v_no) noexcept
{
  double v1, v2, v3;
  v1 = Units::ToUserUnit(polar.shape[0].v, Unit::KILOMETER_PER_HOUR);
  v2 = Units::ToUserUnit(polar.shape[1].v, Unit::KILOMETER_PER_HOUR);
  v3 = Units::ToUserUnit(polar.shape[2].v, Unit::KILOMETER_PER_HOUR);

  if (include_v_no)
    snprintf(buffer, max_size,
             "%.0f,%.0f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f",
             (double)polar.shape.reference_mass, (double)polar.max_ballast,
             (double)v1, (double)polar.shape[0].w,
             (double)v2, (double)polar.shape[1].w,
             (double)v3, (double)polar.shape[2].w,
             (double)polar.wing_area, (double)polar.v_no);
  else
    snprintf(buffer, max_size,
             "%.0f,%.0f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f",
             (double)polar.shape.reference_mass, (double)polar.max_ballast,
             (double)v1, (double)polar.shape[0].w,
             (double)v2, (double)polar.shape[1].w,
             (double)v3, (double)polar.shape[2].w,
             (double)polar.wing_area);
}
