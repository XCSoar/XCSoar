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
#include "Util/tstring.hpp"
#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "Navigation/Aircraft.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "AirspaceAltitude.hpp"
#include "Util/tstring.hpp"
#include "AirspaceAircraftPerformance.hpp"
#include "AirspaceClass.hpp"

class AtmosphericPressure;

/**
 *  Structure to hold data for intercepts between aircraft and airspace.
 *  (interior or exterior)
 */
struct AirspaceInterceptSolution {
  /**
   *  Constructor, initialises to invalid solution
   */
  AirspaceInterceptSolution():
    distance(-fixed_one),
    altitude(-fixed_one),
    elapsed_time(-fixed_one) {};

/** 
 * Determine whether this solution is valid
 * 
 * @return True if solution is valid
 */
  bool valid() const {
    return !negative(elapsed_time);
  };

  GEOPOINT location; /**< Location of intercept point */
  fixed distance;  /**< Distance from observer to intercept point (m) */
  fixed altitude; /**< Altitude AMSL (m) of intercept point */
  fixed elapsed_time; /**< Estimated time (s) for observer to reach intercept point */
};


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
   * Checks whether an observer is inside the airspace (no altitude taken into account)
   * This is slow because it uses geodesic calculations
   * 
   * @param loc State about which to test inclusion
   * 
   * @return true if observer is inside airspace boundary
   */
  virtual bool inside(const GEOPOINT &loc) const = 0;

  /** 
   * Checks whether an observer is inside the airspace (altitude is taken into account)
   * This calls inside(state.location) so can be slow.
   * 
   * @param state State about which to test inclusion
   * 
   * @return true if aircraft is inside airspace boundaries
   */
  virtual bool inside(const AIRCRAFT_STATE& state) const;

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
                      const AirspaceClass_t _Type,
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
  AirspaceClass_t get_type() const {
    return Type;
  }

/** 
 * Test whether base is at terrain level
 * 
 * @return True if base is 0 AGL
 */
  bool is_base_terrain() const {
    return m_base.is_terrain();
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

/**
 * Find time/distance/height to airspace from an observer given a
 * simplified performance model and the boundary start/end points.  If
 * inside the airspace, this will give the time etc to exit (it cares
 * not about interior/exterior, only minimum time to reach the
 * specified location)
 *
 * @param state Aircraft state
 * @param perf Aircraft performance model
 * @param solution Solution of intercept (set if intercept possible, else untouched)
 * @param loc_start Location of first point on/in airspace to query (if provided)
 * @param loc_end Location of last point on/in airspace to query (if provided)
 * @return True if intercept found
 */
  bool intercept(const AIRCRAFT_STATE &state,
                 const AirspaceAircraftPerformance& perf,
                 AirspaceInterceptSolution &solution,
                 const GEOPOINT& loc_start,
                 const GEOPOINT& loc_end) const;

/**
 * Find time/distance/height to airspace from an observer given a
 * simplified performance model and the aircraft path vector.  If
 * inside the airspace, this will give the time etc to exit (it cares
 * not about interior/exterior, only minimum time to reach the
 * specified location)
 *
 * @param state Aircraft state
 * @param vec Path vector of aircraft
 * @param perf Aircraft performance model
 * @param solution Solution of intercept (set if intercept possible, else untouched)
 * @return True if intercept found
 */
  bool intercept(const AIRCRAFT_STATE &state,
                 const GeoVector& vec,
                 const AirspaceAircraftPerformance& perf,
                 AirspaceInterceptSolution &solution) const;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const AbstractAirspace& as);
#endif

/** 
 * Produce text version of airspace class.
 * 
 * @param concise Whether to use short form or long form
 * 
 * @return Text version of class
 */
  const tstring get_type_text(const bool concise=false) const;

/** 
 * Produce text version of name and airspace class.
 * If concise is true, only produces name.
 * 
 * @param concise Whether to use short form or long form
 * 
 * @return Text version of name/type
 */
  const tstring get_name_text(const bool concise=false) const;

/** 
 * Produce text version of base altitude.
 * 
 * @param concise Whether to use short form or long form
 * 
 * @return Text version of base altitude
 */
  const tstring get_base_text(const bool concise=false) const;

/** 
 * Produce text version of top altitude.
 * 
 * @param concise Whether to use short form or long form
 * 
 * @return Text version of top altitude
 */
  const tstring get_top_text(const bool concise=false) const;

protected:
  AIRSPACE_ALT m_base; /**< Base of airspace */
  AIRSPACE_ALT m_top; /**< Top of airspace */
  tstring Name; /**< Airspace name (identifier) */
  AirspaceClass_t Type; /**< Airspace class */
  const TaskProjection* m_task_projection; /**< Task projection (owned by container) that can be used for query speedups */

private:

/**
 * Find time/distance to specified point on the boundary from an observer
 * given a simplified performance model.  If inside the airspace, this will
 * give the time etc to exit (it cares not about interior/exterior, only minimum
 * time to reach the specified location)
 *
 * @param state Aircraft state
 * @param perf Aircraft performance model
 * @param distance Distance from aircraft to boundary 
 * @return Solution of intercept 
 */
  AirspaceInterceptSolution intercept_vertical(const AIRCRAFT_STATE &state,
                                               const AirspaceAircraftPerformance& perf,
                                               const fixed& distance) const;

/**
 * Find time/distance to specified horizontal boundary from an observer
 * given a simplified performance model.  If inside the airspace, this will
 * give the time etc to exit (it cares not about interior/exterior, only minimum
 * time to reach the specified location)
 *
 * @param state Aircraft state
 * @param perf Aircraft performance model
 * @param distance_start Distance from aircraft to start boundary 
 * @param distance_end Distance from aircraft to end boundary 
 * @param lower If true, examines lower boundary, otherwise upper boundary
 * @return Solution of intercept 
 */
  AirspaceInterceptSolution intercept_horizontal(const AIRCRAFT_STATE &state,
                                                 const AirspaceAircraftPerformance& perf,
                                                 const fixed& distance_start,
                                                 const fixed& distance_end,
                                                 const bool lower=true) const;

public:
  DEFINE_VISITABLE()
};

#endif
