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
#ifndef OBSERVATIONZONECLIENT_HPP
#define OBSERVATIONZONECLIENT_HPP

#include "ObservationZonePoint.hpp"

/**
 * Class holding an ObzervationZonePoint, directing calls to it
 */
class ObservationZoneClient: public virtual ObservationZone
{
public:
  /**
   * Constructor.  Transfers ownership of the OZ to this object.
   *
   * @param _oz The OZ to store
   */
  ObservationZoneClient(ObservationZonePoint* _oz):m_oz(_oz) {};

  virtual ~ObservationZoneClient() {
    delete m_oz;
  };

/** 
 * Accessor for OZ (for modifying parameters etc)
 *
 * @return Observation zone
 */
  ObservationZonePoint* get_oz() const {
    return m_oz;
  }

/** 
 * Test whether aircraft is inside observation zone.
 * 
 * @param ref Aircraft state to test
 * 
 * @return True if aircraft is inside observation zone
 */
  virtual bool isInSector(const AIRCRAFT_STATE &ref) const
  {
    return m_oz->isInSector(ref);
  }

/** 
 * Generate a random location inside the OZ (to be used for testing)
 * 
 * @param mag proportional magnitude of error from center (0-1)
 *
 * @return Location of point
 */
  GEOPOINT randomPointInSector(const fixed mag) const {
    return m_oz->randomPointInSector(mag);
  }

/** 
 * Calculate distance reduction for achieved task point,
 * to calcuate scored distance.
 * 
 * @return Distance reduction once achieved
 */
  virtual fixed score_adjustment() const {
    return m_oz->score_adjustment();
  }

protected:

/** 
 * Calculate boundary point from parametric border
 * 
 * @param t t value (0,1) of parameter
 * 
 * @return Boundary point
 */
  GEOPOINT get_boundary_parametric(fixed t) const
  {
    return m_oz->get_boundary_parametric(t);
  }

/** 
 * Check transition constraints 
 * 
 * @param ref_now Current aircraft state
 * @param ref_last Previous aircraft state
 * 
 * @return True if constraints are satisfied
 */
  virtual bool transition_constraint(const AIRCRAFT_STATE & ref_now, 
                                     const AIRCRAFT_STATE & ref_last) {
    return m_oz->transition_constraint(ref_now, ref_last);
  }

/**
 * Set previous/next taskpoints to allow OZ to update its geometry
 *
 * @param previous Origin tp of inbound leg
 * @param current Tp of this OZ
 * @param next Destination of outbound leg
 */
  virtual void set_legs(const TaskPoint *previous,
                        const TaskPoint *current,
                        const TaskPoint *next);

private:
  ObservationZonePoint* m_oz;

};


#endif
