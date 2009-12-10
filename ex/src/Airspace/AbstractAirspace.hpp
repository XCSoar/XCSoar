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
#ifndef ABSTRACTAIRSPACE_HPP
#define ABSTRACTAIRSPACE_HPP

#include "Util/GenericVisitor.hpp"
#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "Navigation/Aircraft.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "AirspaceAltitude.hpp"
#include "Util/tstring.hpp"
#include "AirspaceAircraftPerformance.hpp"

class AtmosphericPressure;

#include <vector>
typedef std::vector< std::pair<GEOPOINT,GEOPOINT> > AirspaceIntersectionVector;

/**
 * Abstract base class for airspace regions
 */
class AbstractAirspace:
  public BaseVisitable<>
{
public:

  /** 
   * Compute bounding box enclosing the airspace.  Rounds up/down
   * so discretisation ensures bounding box is indeed enclosing.
   * 
   * @param task_projection Projection used for flat-earth representation
   * 
   * @return Enclosing bounding box
   */
  virtual const FlatBoundingBox 
    get_bounding_box(const TaskProjection& task_projection) = 0;

/** 
 * Set task projection for internal use
 * 
 * @param task_projection Global task projection (owned by Airspaces)
 */
  void set_task_projection(const TaskProjection& task_projection) {
    m_task_projection = &task_projection;
  };

/** 
 * Get arbitrary center or reference point for use in determining
 * overall center location of all airspaces
 * 
 * @return Location of reference point
 */
  virtual const GEOPOINT get_center() const = 0;

  /** 
   * Checks whether an observer is inside the airspace.
   * This is slow because it uses geodesic calculations
   * 
   * @param loc State about which to test inclusion
   * 
   * @return true if aircraft is inside airspace boundary
   */
  virtual bool inside(const GEOPOINT &loc) const = 0;

  /** 
   * Checks whether a line intersects with the airspace.
   * Can be approximate by using flat-earth representation internally.
   * 
   * @param g1 Location of origin of search vector
   * @param vec Line from origin
   * @param fill_end whether to create fake point for orphaned entry point
   * 
   * @return Vector of intersection pairs if the line intersects the airspace
   */
  virtual AirspaceIntersectionVector intersects(const GEOPOINT& g1, 
                                                const GeoVector &vec,
                                                const bool fill_end=false) const = 0;

/** 
 * Find location of closest point on boundary to a reference
 * 
 * @param loc Reference location of observer
 * 
 * @return Location of closest point of boundary to reference 
 */
  virtual GEOPOINT closest_point(const GEOPOINT& loc) const
    = 0;

/**
 * Find time/distance to specified point on the boundary from an observer
 * given a simplified performance model.  If inside the airspace, this will
 * give the time etc to exit (it cares not about interior/exterior, only minimum
 * time to reach the specified location)
 *
 * @param state Aircraft state
 * @param loc Location of point on/in airspace to query
 * @param perf Aircraft performance model
 * @param time_to_intercept On entry, maximum time, on exit, 
 *                          time of this intercept (if within range)
 * @param intercept_height Height of intercept (m), if within time limit
 * @return True if intercept possible
 */
  bool intercept_vertical(const AIRCRAFT_STATE &state,
                          const GEOPOINT& loc,
                          const AirspaceAircraftPerformance& perf,
                          fixed &time_to_intercept,
                          fixed &intercept_height) const;

  /** 
   * Set terrain altitude for AGL-referenced airspace altitudes 
   * 
   * @param alt Height above MSL of terrain (m) at center
   */
  void set_ground_level(const fixed alt);

  /** 
   * Set QNH pressure for FL-referenced airspace altitudes 
   * 
   * @param press Atmospheric pressure model and QNH
   */
  void set_flight_level(const AtmosphericPressure &press);

  /**
   * Set fundamental properties of airspace
   *
   * @param _Name Name of airspace
   * @param _Type Type/class
   * @param _base Lower limit
   * @param _top Upper limit
   */
  void set_properties(const tstring &_Name,
                      const int _Type,
                      const AIRSPACE_ALT &_base,
                      const AIRSPACE_ALT &_top) {
    Name = _Name;
    Type = _Type;
    m_base = _base;
    m_top = _top;
  }

  /** 
   * Get type of airspace
   * 
   * @return Type/class of airspace
   */
  int get_type() const {
    return Type;
  }

/** 
 * Get base altitude
 * 
 * @return Altitude AMSL (m) of base
 */
  fixed get_base_altitude() const {
    return m_base.Altitude;
  }

/** 
 * Get top altitude
 * 
 * @return Altitude AMSL (m) of top
 */
  fixed get_top_altitude() const {
    return m_top.Altitude;
  }

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const AbstractAirspace& as);
#endif

protected:
  AIRSPACE_ALT m_base; /**< Base of airspace */
  AIRSPACE_ALT m_top; /**< Top of airspace */
  tstring Name; /**< Airspace name (identifier) */
  int Type; /**< Airspace class */
  const TaskProjection* m_task_projection; /**< Task projection (owned by container) that can be used for query speedups */

#ifdef OLD_TASK
  AIRSPACE_ACK Ack;
  unsigned char WarningLevel; // 0= no warning, 1= predicted incursion, 2= entered
#endif

public:
  DEFINE_VISITABLE()
};

#endif
