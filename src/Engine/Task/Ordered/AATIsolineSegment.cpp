// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AATIsolineSegment.hpp"
#include "Task/PathSolvers/IsolineCrossingFinder.hpp"
#include "Util/Tolerances.hpp"

AATIsolineSegment::AATIsolineSegment(const AATPoint &ap,
                                     const FlatProjection &projection) noexcept
  :AATIsoline(ap, projection)
{
  IsolineCrossingFinder icf_up(ap, ell, 0, 0.5);
  IsolineCrossingFinder icf_down(ap, ell, -0.5, 0);

  t_up = icf_up.solve();
  t_down = icf_down.solve();

  if (t_up < -0.5 || t_down < -0.5) {
    t_up = 0;
    t_down = 0;
    // single solution only
  }
}

bool
AATIsolineSegment::IsValid() const noexcept
{
  return t_up > t_down + TOLERANCE_ISOLINE_CROSSING * 2;
}

GeoPoint
AATIsolineSegment::Parametric(const double t) const noexcept
{
  const auto r = t * (t_up - t_down) + t_down;
  return ell.Parametric(r);
}
