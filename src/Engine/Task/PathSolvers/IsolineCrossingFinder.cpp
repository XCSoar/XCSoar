// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IsolineCrossingFinder.hpp"
#include "Geo/GeoEllipse.hpp"
#include "Task/Ordered/Points/AATPoint.hpp"
#include "Util/Tolerances.hpp"

IsolineCrossingFinder::IsolineCrossingFinder(const AATPoint& _aap,
                                             const GeoEllipse &_ell,
                                             const double _xmin,
                                             const double _xmax) noexcept
  :ZeroFinder(_xmin, _xmax, TOLERANCE_ISOLINE_CROSSING),
   aap(_aap),
   ell(_ell)
{
}

double
IsolineCrossingFinder::f(const double t) noexcept
{
  const GeoPoint a = ell.Parametric(t);
  AircraftState s;
  s.location = a;

  // note: use of isInSector is slow!
  // if (aap.isInSector(s)) ->  attract solutions away from t
  // else                   ->  attract solutions towards t
  return (aap.IsInSector(s) ? 1. : -1.) - fabs(t);
}

#define bsgn(x) (x < 1. ? false : true)

bool
IsolineCrossingFinder::valid([[maybe_unused]] const double x) noexcept
{
/*
  const bool bsgn_0 = bsgn(f(x));
  const bool bsgn_m = bsgn(f(x-2*tolerance));
  const bool bsgn_p = bsgn(f(x+2*tolerance));

  \todo this is broken, so assume it's ok for now
  return (bsgn_0 != bsgn_m) || (bsgn_0 != bsgn_p);
*/
  return true;
}

double
IsolineCrossingFinder::solve() noexcept
{
  const auto sol = find_zero((xmax + xmin) / 2.);
  if (valid(sol)) {
    return sol;
  } else {
    return -1;
  }
}
