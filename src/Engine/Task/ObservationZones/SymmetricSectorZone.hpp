/*
  Copyright_License {

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

#ifndef SYMMETRICSECTORZONE_HPP
#define SYMMETRICSECTORZONE_HPP

#include "Task/Tasks/BaseTask/TaskPoint.hpp"
#include "SectorZone.hpp"

/**
 * Segment centered on bisector of incoming/outgoing legs 
 */
class SymmetricSectorZone: 
  public SectorZone 
{
public:
/** 
 * Constructor.
 * 
 * @param loc Tip of sector location
 * @param radius Radius (m) of sector
 * @param angle Angle subtended by start/end radials
 * 
 * @return Initialised object
 */
  SymmetricSectorZone(const GEOPOINT &loc,
                      const fixed radius=fixed(10000.0),
                      const fixed angle=fixed(90.0)):
    SectorZone(loc,radius),
    SectorAngle(angle) {}

  virtual ObservationZonePoint* clone(const GEOPOINT * _location=0) const {
    if (_location) {
      return new SymmetricSectorZone(*_location, Radius, SectorAngle);
    } else {
      return new SymmetricSectorZone(get_location(), Radius, SectorAngle);
    }
  }

/** 
 * Update radials when previous/next legs are modified.
 * 
 * @param previous Previous task point (origin of inbound leg)
 * @param current Taskpoint this is located at
 * @param next Following task point (destination of outbound leg)
 */
  virtual void set_legs(const TaskPoint *previous,
                        const TaskPoint *current,
                        const TaskPoint *next);

/** 
 * Test whether an OZ is equivalent to this one
 * 
 * @param other OZ to compare to
 * 
 * @return True if same type and OZ parameters
 */

  virtual bool equals(const ObservationZonePoint* other) const;

  /** 
   * Accessor for angle of sector (angle between start/end radials)
   * 
   * @return Angle (deg) of sector
   */
  fixed getSectorAngle() const {
    return SectorAngle;
  }

private:
  const fixed SectorAngle;
};


#endif
