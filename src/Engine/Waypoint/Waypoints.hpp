/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#ifndef WAYPOINTS_HPP
#define WAYPOINTS_HPP

#include "Util/NonCopyable.hpp"
#include "Util/SliceAllocator.hpp"
#include "Util/RadixTree.hpp"
#include "Util/QuadTree.hpp"
#include "Waypoint.hpp"

#include "Navigation/TaskProjection.hpp"

class WaypointVisitor;

/**
 * Container for waypoints using kd-tree representation internally for fast 
 * geospatial lookups.
 */
class Waypoints: private NonCopyable 
{
  /**
   * Function object used to provide access to coordinate values by
   * QuadTree.
   */
  struct WaypointAccessor {
    int GetX(const Waypoint &wp) const {
      assert(wp.flat_location_initialised);
      return wp.flat_location.Longitude;
    }

    int GetY(const Waypoint &wp) const {
      assert(wp.flat_location_initialised);
      return wp.flat_location.Latitude;
    }
  };

  /**
   * Type of KD-tree data structure for waypoint container
   */
  typedef QuadTree<Waypoint, WaypointAccessor,
                   SliceAllocator<Waypoint, 512u> > WaypointTree;

  class WaypointNameTree : public RadixTree<const Waypoint*> {
  public:
    const Waypoint *Get(const TCHAR *name) const;
    void VisitNormalisedPrefix(const TCHAR *prefix, WaypointVisitor &visitor) const;
    void Add(const Waypoint &wp);
    void Remove(const Waypoint &wp);
  };

  unsigned next_id;

  WaypointTree waypoint_tree;
  WaypointNameTree name_tree;
  TaskProjection task_projection;

  const Waypoint *m_home;

public:
  /**
   * Constructor.  Task projection is updated after call to optimise().
   * As waypoints are added they are stored temporarily before applying
   * projection so that the bounds of the projection may be obtained.
   *
   * See below for usage notes --- further work is required.
   *
   */
  Waypoints();


  /**
   * Add waypoint to internal store.  Internal copy is made.
   * optimise() must be called after inserting waypoints prior to
   * performing any queries, but can be done in batches.
   *
   * @param wp Waypoint to add to internal store
   */
  const Waypoint &append(const Waypoint &wp);

  /**
   * Erase waypoint from the internal store.  Requires optimise() to
   * be called afterwards
   *
   * @param wp Waypoint to erase from internal store
   */
  void erase(const Waypoint& wp);

  /**
   * Replace waypoint from the internal store.  Requires optimise() to
   * be called afterwards.
   *
   * @param orig Waypoint that will be replaced
   * @param replacement New waypoint
   */
  void replace(const Waypoint &orig, const Waypoint &replacement);

  /**
   * Create new waypoint (without appending it to the store),
   * with set id.  This is like a factory method.
   *
   * @param location Location of waypoint
   *
   * @return Blank object at given location, with id set
   */
  Waypoint create(const GeoPoint& location);

  /**
   * Optimise the internal search tree after adding/removing elements.
   * Also performs projection to flat earth for new elements.
   * This updates the task_projection.
   *
   * Note: currently this code doesn't check for task projections
   * being modified from multiple calls to optimise() so it should
   * only be called once (until this is fixed).
   */
  void optimise();

  /**
   * Clear the waypoint store
   */
  void clear();

  /**
   * Size of waypoints (in tree, not in temporary store) ---
   * must call optimise() before this for it to be accurate.
   *
   * @return Number of waypoints in tree
   */
  gcc_pure
  unsigned size() const {
    return waypoint_tree.size();
  }

  /**
   * Whether waypoints store is empty
   *
   * @return True if no waypoints stored
   */
  gcc_pure
  bool empty() const {
    return waypoint_tree.IsEmpty();
  }

  /**
   * Generate takeoff waypoint
   *
   * @return waypoint copy
   */
  Waypoint generate_takeoff_point(const GeoPoint& location,
                                  const fixed terrain_alt) const;

  /**
   * Create a takeoff point or replaces previous.
   * This modifies the waypoint database.
   */
  void add_takeoff_point(const GeoPoint& location,
                         const fixed terrain_alt);

  /**
   * Return the current home waypoint.  May be NULL if none is
   * configured.
   */
  const Waypoint *GetHome() const {
    return m_home;
  }

  /**
   * Find first home waypoint
   *
   * @return Pointer to waypoint if found (or NULL if not)
   */
  const Waypoint *find_home();

  /**
   * Set single home waypoint (clearing all others as home)
   *
   * @param id Id of waypoint to set as home
   * @return True on success (id was found)
   */
  bool set_home(const unsigned id);

  /**
   * Look up waypoint by ID.
   *
   * @param id Id of waypoint to find in internal tree
   *
   * @return Pointer to waypoint if found (or NULL if not)
   */
  const Waypoint* lookup_id(const unsigned id) const;

  /**
   * Look up closest waypoint by location within range
   *
   * @param loc Location of waypoint to find in internal tree
   * @param range Threshold for range
   *
   * @return Pointer to waypoint if found (or NULL if none found)
   */
  const Waypoint* lookup_location(const GeoPoint &loc,
                                  const fixed range= fixed_zero) const;

  /**
   * Look up waypoint by name (returns first match)
   *
   * @param name Name of waypoint to find in internal tree
   *
   * @return Pointer to waypoint if found (or NULL if not)
   */
  const Waypoint* lookup_name(const TCHAR *name) const;

  const Waypoint* lookup_name(const tstring &name) const {
    return lookup_name(name.c_str());
  }

 /** 
  * Check if a waypoint with same name and approximate location
  * is already in the database.  If not, is appended to the database.
  * 
  * @param waypoint Waypoint to check against (replaced)
  * 
  * @return True if found.  False if appended
  */
  bool check_exists_or_append(Waypoint& waypoint);

  /**
   * Call visitor function on waypoints within approximate range
   * (square range box) to search location.  Possible use by screen display
   * functions.
   *
   * @param loc Location from which to search
   * @param range Distance in meters of search radius
   * @param visitor Visitor to be called on waypoints within range
   */
  void visit_within_range(const GeoPoint &loc, const fixed range,
                          WaypointVisitor& visitor) const;

  /**
   * Call visitor function on waypoints with the specified name
   * prefix.
   */
  void visit_name_prefix(const TCHAR *prefix, WaypointVisitor& visitor) const;

  /**
   * Returns a set of possible characters following the specified
   * prefix.
   */
  TCHAR *suggest_name_prefix(const TCHAR *prefix,
                             TCHAR *dest, size_t max_length) const {
    return name_tree.suggest(prefix, dest, max_length);
  }

public:
  typedef WaypointTree::const_iterator const_iterator;

  /**
   * Looks up nearest waypoint to the search location.
   * Performs search according to flat-earth internal representation,
   * so is approximate.
   *
   * @param loc Location from which to search
   *
   * @return Null if none found, otherwise pointer to nearest
   */
  const Waypoint *get_nearest(const GeoPoint &loc, fixed range) const;

  /**
   * Looks up nearest landable waypoint to the
   * search location within the given range.
   * Performs search according to flat-earth internal representation,
   * so is approximate.
   *
   * @param loc Location from which to search
   *
   * @return Null if none found, otherwise pointer to nearest
   */
  const Waypoint *get_nearest_landable(const GeoPoint &loc, fixed range) const;

  /**
   * Access first waypoint in store, for use in iterators.
   *
   * @return First waypoint in store
   */
  const_iterator begin() const {
    return waypoint_tree.begin();
  }

  /**
   * Access end waypoint in store, for use in iterators as end point.
   *
   * @return End waypoint in store
   */
  const_iterator end() const {
    return waypoint_tree.end();
  }
};

#endif
