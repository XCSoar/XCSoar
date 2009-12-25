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
#include "Airspace.hpp"
#include "AbstractAirspace.hpp"

void 
Airspace::destroy()
{
  if (pimpl_airspace) {
    delete pimpl_airspace;
  }
}

Airspace::Airspace(AbstractAirspace& airspace,
                   const TaskProjection& tp):
  FlatBoundingBox(airspace.get_bounding_box(tp)),
  pimpl_airspace(&airspace)
{
  pimpl_airspace->set_task_projection(tp);
}


bool 
Airspace::inside(const AIRCRAFT_STATE &loc) const
{
  if (pimpl_airspace) {
    return pimpl_airspace->inside(loc);
  } else {
    return false;
  }
}


bool 
Airspace::inside(const GEOPOINT &loc) const
{
  if (pimpl_airspace) {
    return pimpl_airspace->inside(loc);
  } else {
    return false;
  }
}


bool 
Airspace::intersects(const FlatRay& ray) const
{
  return FlatBoundingBox::intersects(ray);
}


AirspaceIntersectionVector
Airspace::intersects(const GEOPOINT& g1, 
                     const GeoVector &vec) const
{
  if (pimpl_airspace) {
    return pimpl_airspace->intersects(g1, vec);
  } else {
    AirspaceIntersectionVector null;
    return null;
  }
}


void
Airspace::Accept(BaseVisitor &visitor) const
{
  if (pimpl_airspace) {
    pimpl_airspace->Accept(visitor);
  }
}

void 
Airspace::set_ground_level(const fixed alt) const
{
  if (pimpl_airspace) 
    pimpl_airspace->set_ground_level(alt);
}

void 
Airspace::set_flight_level(const AtmosphericPressure &press) const
{
  if (pimpl_airspace) 
    pimpl_airspace->set_flight_level(press);
}

