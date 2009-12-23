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

#include "SectorZone.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

static const fixed fixed_nearlyone(0.999);

GEOPOINT SectorZone::get_boundary_parametric(fixed t) const
{ 
  AIRCRAFT_STATE state;
  state.Location = GeoVector(Radius*fixed_nearlyone, t*fixed_360).end_point(get_location());

  if (isInSector(state)) {
    return state.Location;
  } else {
    return get_location();
  }
}

fixed SectorZone::score_adjustment() const
{
  return fixed_zero;
}

void 
SectorZone::updateSector() 
{
  SectorStart = GeoVector(Radius, StartRadial).end_point(get_location());
  SectorEnd = GeoVector(Radius, EndRadial).end_point(get_location());
}


bool 
SectorZone::isInSector(const AIRCRAFT_STATE &ref) const
{
  GeoVector f(get_location(), ref.Location);

  return (f.Distance<=Radius) && angleInSector(f.Bearing);
}

void 
SectorZone::setStartRadial(const fixed x) 
{
  StartRadial = x;
  updateSector();
}

void 
SectorZone::setEndRadial(const fixed x) 
{
  EndRadial = x;
  updateSector();
}  

bool 
SectorZone::angleInSector(const fixed b) const
{
  if (StartRadial<EndRadial) {
    return ((b<=EndRadial) && (b>=StartRadial));
  } else {
    return ((b<=EndRadial) || (b>=StartRadial));
  }
}

bool
SectorZone::equals(const ObservationZonePoint* other) const
{
  if (CylinderZone::equals(other)) {
    if (const SectorZone* z = dynamic_cast<const SectorZone*>(other)) {
      return (StartRadial == z->getStartRadial()) &&
        (EndRadial == z->getEndRadial());
    }
  }
  return false;
}
