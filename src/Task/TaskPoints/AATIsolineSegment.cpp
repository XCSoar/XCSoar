/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Task/Tasks/PathSolvers/IsolineCrossingFinder.hpp"

AATIsolineSegment::AATIsolineSegment(const AATPoint& ap):
  AATIsoline(ap)
{
  IsolineCrossingFinder icf_up(ap, ell, fixed_zero, fixed_half);
  IsolineCrossingFinder icf_down(ap, ell, -fixed_half, fixed_zero);

  t_up = icf_up.solve();
  t_down = icf_down.solve();
  if ((t_up<-fixed_half) || (t_down<-fixed_half)) {
    t_up = fixed_zero;
    t_down = fixed_zero;
    // single solution only
  }
}

bool
AATIsolineSegment::valid() const
{
  return (t_up>t_down);
}

GEOPOINT 
AATIsolineSegment::parametric(const fixed t) const
{
  const fixed r = t*(t_up-t_down)+t_down;
  return ell.parametric(r);
}
