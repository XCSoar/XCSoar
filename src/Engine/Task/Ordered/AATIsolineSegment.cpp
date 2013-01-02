/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "AATIsolineSegment.hpp"
#include "Task/PathSolvers/IsolineCrossingFinder.hpp"
#include "Util/Tolerances.hpp"

AATIsolineSegment::AATIsolineSegment(const AATPoint &ap,
                                     const TaskProjection &projection)
  :AATIsoline(ap, projection)
{
  IsolineCrossingFinder icf_up(ap, ell, fixed(0), fixed(0.5));
  IsolineCrossingFinder icf_down(ap, ell, -fixed(0.5), fixed(0));

  t_up = icf_up.solve();
  t_down = icf_down.solve();

  if ((t_up < -fixed(0.5)) || (t_down < -fixed(0.5))) {
    t_up = fixed(0);
    t_down = fixed(0);
    // single solution only
  }
}

bool
AATIsolineSegment::IsValid() const
{
  return t_up > t_down + fixed(TOLERANCE_ISOLINE_CROSSING) * 2;
}

GeoPoint
AATIsolineSegment::Parametric(const fixed t) const
{
  const fixed r = t * (t_up - t_down) + t_down;
  return ell.Parametric(r);
}
