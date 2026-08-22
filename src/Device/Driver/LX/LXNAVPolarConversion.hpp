// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/GlideSolvers/PolarCoefficients.hpp"

#include <string_view>

/**
 * LXNAV PLXV0 POLAR coefficients use a normalised airspeed where v==1
 * corresponds to 100 km/h.  XCSoar stores the glide parabola with true
 * airspeed in m/s: w = a*V^2 + b*V + c.
 */
namespace LXNAVPolar {

static constexpr double V_REF_MS = 100.0 / 3.6;

/**
 * Convert SI polar coefficients to values sent in PLXV0,POLAR NMEA fields.
 */
inline void
ToNmeaPolar(const PolarCoefficients &pc,
            double &a_lx, double &b_lx, double &c) noexcept
{
  const double v = V_REF_MS;
  a_lx = pc.a * v * v;
  b_lx = pc.b * v;
  c = pc.c;
}

/**
 * Convert PLXV0,POLAR NMEA coefficient fields to SI polar coefficients.
 */
[[gnu::const]]
inline PolarCoefficients
FromNmeaPolar(double a_lx, double b_lx, double c) noexcept
{
  const double v = V_REF_MS;
  return PolarCoefficients(a_lx / (v * v), b_lx / v, c);
}

/**
 * True if a recorded NMEA line is a PLXV0 POLAR write whose a,b,c
 * fields are empty (partial write).  Empty coefficient fields zero
 * the polar on LXNAV S-series varios (#2397).
 *
 * Accepts lines with or without leading '$' / trailing checksum.
 */
[[gnu::pure]]
inline bool
IsPartialPolarWrite(std::string_view line) noexcept
{
  using namespace std::string_view_literals;

  if (line.starts_with('$'))
    line.remove_prefix(1);

  const auto star = line.find('*');
  if (star != std::string_view::npos)
    line = line.substr(0, star);

  constexpr auto prefix = "PLXV0,POLAR,W,"sv;
  if (!line.starts_with(prefix))
    return false;

  /* Body after "PLXV0,POLAR,W," — empty a,b,c are three leading
     commas (partial crew/empty-weight writes use this form). */
  const auto body = line.substr(prefix.size());
  return body.starts_with(",,,"sv);
}

} // namespace LXNAVPolar
