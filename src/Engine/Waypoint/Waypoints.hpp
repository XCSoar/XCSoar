/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Util/RadixTree.hpp"
#include "Util/QuadTree.hpp"
#include "Util/Serial.hpp"
#include "Ptr.hpp"
#include "Waypoint.hpp"
#include "Geo/Flat/TaskProjection.hpp"

class WaypointVisitor;

/**
 * Container for waypoints using kd-tree representation internally for
 * fast geospatial lookups.
 */
class Waypoints {
  /**
   * Function object used to provide access to coordinate values by
   * QuadTree.
   */
  struct WaypointAccessor {
    gcc_pure
    int GetX(const WaypointPtr &wp) const {
      return wp->flat_location.x;
    }

    gcc_pure
    int GetY(const WaypointPtr &wp) const {
      return wp->flat_location.y;
    }
  };

  /**
   * Type of KD-tree data structure for waypoint container
   */
  typedef QuadTree<WaypointPtr, WaypointAccessor> WaypointTree;

  class WaypointNameTree : public RadixTree<WaypointPtr> {
  public:
    WaypointPtr Get(const TCHAR *name) const;
    void VisitNormalisedPrefix(const TCHAR *prefix, WaypointVisitor &visitor) const;
    TCHAR *SuggestNormalisedPrefix(const TCHAR *prefix,
                                   TCHAR *dest, size_t max_length) const;
    void Add(WaypointPtr wp);
    void Remove(const WaypointPtr &wp);
  };

  /**
   * This gets incremented each time the object is modified.
   */
  Serial serial;

  unsigned next_id;

  WaypointTree waypoint_tree;
  WaypointNameTree name_tree;
  TaskProjection task_projection;

  WaypointPtr home;

public:
  typedef WaypointTree::const_iterator const_iterator;

  /**
   * Constructor.  Task projection is updated after call to Optimise().
   * As waypoints are added they are stored temporarily before applying
   * projection so that the bounds of the projection may be obtained.
   *
   * See below for usage notes --- further work is required.
   *
   */
  Waypoints();

  Waypoints(const Waypoints &) = delete;

  const Serial &GetSerial() const {
    return serial;
  }

  /**
   * Add this waypoint to internal store.
   * Optimise() must be called after inserting waypoints prior to
   * performing any queries, but can be done in batches.
   *
   * @param wp Waypoint to add to internal store
   */
  void Append(WaypointPtr wp);

  /**
   * Add waypoint to internal store.  Internal copy is made.
   * Optimise() must be called after inserting waypoints prior to
   * performing any queries, but can be done in batches.
   *
   * @param wp Waypoint to add to internal store
   */
  WaypointPtr Append(Waypoint &&wp) {
    WaypointPtr ptr(new Waypoint(std::move(wp)));
    Append(ptr);
    return ptr;
  }

  /**
   * Erase waypoint from the internal store.  Requires Optimise() to
   * be called afterwards
   *
   * @param wp Waypoint to erase from internal store
   */
  void Erase(WaypointPtr &&wp);

  /**
   * Erase all waypoints with origin==WaypointOrigin::USER &&
   * type==Type::MARKER.
   */
  void EraseUserMarkers();

  /**
   * Replace waypoint from the internal store.  Requires Optimise() to
   * be called afterwards.
   *
   * @param orig Waypoint that will be replaced
   * @param replacement New waypoint
   */
  void Replace(const WaypointPtr &orig, Waypoint &&replacement);

  /**
   * Create new waypoint (without appending it to the store),
   * with set id.  This is like a factory method.
   *
   * @param location Location of waypoint
   *
   * @return Blank object at given location, with id set
   */
  Waypoint Create(const GeoPoint& location);

  /**
   * Optimise the internal search tree after adding/removing elements.
   * Also performs projection to flat earth for new elements.
   * This updates the task_projection.
   *
   * Note: currently this code doesn't check for task projections
   * being modified from multiple calls to Optimise() so it should
   * only be called once (until this is fixed).
   */
  void Optimise();

  /**
   * Prepare and enable the next Optimise() call.
   */
  void ScheduleOptimise() {
    waypoint_tree.Flatten();
    waypoint_tree.ClearBounds();
  }

  /**
   * Clear the waypoint store
   */
  void Clear();

  /**
   * Size of waypoints (in tree, not in temporary store) ---
   * must call Optimise() before this for it to be accurate.
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
  bool IsEmpty() const {
    return waypoint_tree.IsEmpty();
  }

  /**
   * Generate takeoff waypoint
   *
   * @return waypoint copy
   */
  Waypoint GenerateTakeoffPoint(const GeoPoint& location,
                                double terrain_alt) const;

  /**
   * Create a takeoff point or replaces previous.
   * This modifies the waypoint database.
   */
  void AddTakeoffPoint(const GeoPoint& location,
                       double terrain_alt);

  /**
   * Return the current home waypoint.  May be nullptr if none is
   * configured.
   */
  WaypointPtr GetHome() const {
    return home;
  }

  /**
   * Find first home waypoint
   *
   * @return Pointer to waypoint if found (or nullptr if not)
   */
  gcc_pure
  WaypointPtr FindHome();

  /**
   * Set single home waypoint (clearing all others as home)
   *
   * @param id Id of waypoint to set as home
   * @return True on success (id was found)
   */
  bool SetHome(const unsigned id);

  /**
   * Look up waypoint by ID.
   *
   * @param id Id of waypoint to find in internal tree
   *
   * @return Pointer to waypoint if found (or nullptr if not)
   */
  gcc_pure
  WaypointPtr LookupId(const unsigned id) const;

  /**
   * Look up closest waypoint by location within range
   *
   * @param loc Location of waypoint to find in internal tree
   * @param range Threshold for range
   *
   * @return Pointer to waypoint if found (or nullptr if none found)
   */
  gcc_pure
  WaypointPtr LookupLocation(const GeoPoint &loc,
                             const double range = 0) const;

  /**
   * Look up waypoint by name (returns first match)
   *
   * @param name Name of waypoint to find in internal tree
   *
   * @return Pointer to waypoint if found (or nullptr if not)
   */
  gcc_pure
  WaypointPtr LookupName(const TCHAR *name) const;

  gcc_pure
  WaypointPtr LookupName(const tstring &name) const {
    return LookupName(name.c_str());
  }

  /**
   * Check if a waypoint with same name and approximate location
   * is already in the database.  If not, is appended to the database.
   *
   * @param waypoint Waypoint to check against (replaced)
   *
   * @return reference to waypoint in tree (either existing or appended)
   */
  WaypointPtr CheckExistsOrAppend(WaypointPtr waypoint);

  /**
   * Call visitor function on waypoints within approximate range
   * (square range box) to search location.  Possible use by screen display
   * functions.
   *
   * @param loc Location from which to search
   * @param range Distance in meters of search radius
   * @param visitor Visitor to be called on waypoints within range
   */
  void VisitWithinRange(const GeoPoint &loc, double range,
                        WaypointVisitor &visitor) const;

  /**
   * Call visitor function on waypoints with the specified name
   * prefix.
   */
  void VisitNamePrefix(const TCHAR *prefix, WaypointVisitor& visitor) const;

  /**
   * Returns a set of possible characters following the specified
   * prefix.
   */
  gcc_pure
  TCHAR *SuggestNamePrefix(const TCHAR *prefix,
                           TCHAR *dest, size_t max_length) const {
    return name_tree.SuggestNormalisedPrefix(prefix, dest, max_length);
  }

  /**
   * Looks up nearest waypoint to the search location.
   * Performs search according to flat-earth internal representation,
   * so is approximate.
   *
   * @param loc Location from which to search
   *
   * @return Null if none found, otherwise pointer to nearest
   */
  gcc_pure
  WaypointPtr GetNearest(const GeoPoint &loc, double range) const;

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
  gcc_pure
  WaypointPtr GetNearestLandable(const GeoPoint &loc, double range) const;

  /**
   * Looks up nearest waypoint to the search location.
   * Performs search according to flat-earth internal representation,
   * so is approximate.
   *
   * @param loc Location from which to search
   * @param predicate Callback that checks whether the waypoint
   * is suitable for the request
   *
   * @return Null if none found, otherwise pointer to nearest
   */
  gcc_pure
  WaypointPtr GetNearestIf(const GeoPoint &loc, double range,
                           bool (*predicate)(const Waypoint &)) const;

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
