// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "Ptr.hpp"
#include "Waypoint.hpp"
#include "Geo/Flat/TaskProjection.hpp"
#include "util/RadixTree.hpp"
#include "util/QuadTree.hxx"
#include "util/Serial.hpp"
#include "util/tstring_view.hxx"

#include <functional>

using WaypointVisitor = std::function<void(const WaypointPtr &)>;

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
    [[gnu::pure]]
    int GetX(const WaypointPtr &wp) const noexcept {
      return wp->flat_location.x;
    }

    [[gnu::pure]]
    int GetY(const WaypointPtr &wp) const noexcept {
      return wp->flat_location.y;
    }
  };

  /**
   * Type of KD-tree data structure for waypoint container
   */
  using WaypointTree = QuadTree<WaypointPtr, WaypointAccessor>;

  class WaypointNameTree : public RadixTree<WaypointPtr> {
  public:
    [[gnu::pure]]
    WaypointPtr Get(tstring_view name) const noexcept;

    void VisitNormalisedPrefix(tstring_view prefix, const WaypointVisitor &visitor) const;
    TCHAR *SuggestNormalisedPrefix(tstring_view prefix,
                                   TCHAR *dest, size_t max_length) const noexcept;
    void Add(WaypointPtr wp) noexcept;
    void Remove(const WaypointPtr &wp) noexcept;
  };

  /**
   * This gets incremented each time the object is modified.
   */
  Serial serial;

  unsigned next_id = 1;

  WaypointTree waypoint_tree;
  WaypointNameTree name_tree;
  TaskProjection task_projection;

  WaypointPtr home;

public:
  using const_iterator = WaypointTree::const_iterator;

  /**
   * Constructor.  Task projection is updated after call to Optimise().
   * As waypoints are added they are stored temporarily before applying
   * projection so that the bounds of the projection may be obtained.
   *
   * See below for usage notes --- further work is required.
   *
   */
  Waypoints() noexcept;

  Waypoints(const Waypoints &) = delete;

  const Serial &GetSerial() const noexcept {
    return serial;
  }

  /**
   * Add this waypoint to internal store.
   * Optimise() must be called after inserting waypoints prior to
   * performing any queries, but can be done in batches.
   *
   * @param wp Waypoint to add to internal store
   */
  void Append(WaypointPtr wp) noexcept;

  /**
   * Add waypoint to internal store.  Internal copy is made.
   * Optimise() must be called after inserting waypoints prior to
   * performing any queries, but can be done in batches.
   *
   * @param wp Waypoint to add to internal store
   */
  WaypointPtr Append(Waypoint &&wp) noexcept {
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
  void Erase(WaypointPtr &&wp) noexcept;

  /**
   * Erase all waypoints with origin==WaypointOrigin::USER &&
   * type==Type::MARKER.
   */
  void EraseUserMarkers() noexcept;

  /**
   * Replace waypoint from the internal store.  Requires Optimise() to
   * be called afterwards.
   *
   * @param orig Waypoint that will be replaced
   * @param replacement New waypoint
   */
  void Replace(const WaypointPtr &orig, Waypoint &&replacement) noexcept;

  /**
   * Create new waypoint (without appending it to the store),
   * with set id.  This is like a factory method.
   *
   * @param location Location of waypoint
   *
   * @return Blank object at given location, with id set
   */
  Waypoint Create(const GeoPoint& location) noexcept;

  /**
   * Optimise the internal search tree after adding/removing elements.
   * Also performs projection to flat earth for new elements.
   * This updates the task_projection.
   *
   * Note: currently this code doesn't check for task projections
   * being modified from multiple calls to Optimise() so it should
   * only be called once (until this is fixed).
   */
  void Optimise() noexcept;

  /**
   * Prepare and enable the next Optimise() call.
   */
  void ScheduleOptimise() noexcept {
    waypoint_tree.Flatten();
    waypoint_tree.ClearBounds();
  }

  /**
   * Clear the waypoint store
   */
  void Clear() noexcept;

  /**
   * Size of waypoints (in tree, not in temporary store) ---
   * must call Optimise() before this for it to be accurate.
   *
   * @return Number of waypoints in tree
   */
  [[gnu::pure]]
  unsigned size() const noexcept {
    return waypoint_tree.size();
  }

  /**
   * Whether waypoints store is empty
   *
   * @return True if no waypoints stored
   */
  [[gnu::pure]]
  bool IsEmpty() const noexcept {
    return waypoint_tree.IsEmpty();
  }

  /**
   * Generate takeoff waypoint
   *
   * @return waypoint copy
   */
  Waypoint GenerateTakeoffPoint(const GeoPoint &location,
                                double terrain_alt) const noexcept;

  /**
   * Create a takeoff point or replaces previous.
   * This modifies the waypoint database.
   */
  void AddTakeoffPoint(const GeoPoint& location,
                       double terrain_alt) noexcept;

  /**
   * Return the current home waypoint.  May be nullptr if none is
   * configured.
   */
  WaypointPtr GetHome() const noexcept {
    return home;
  }

  /**
   * Find first home waypoint
   *
   * @return Pointer to waypoint if found (or nullptr if not)
   */
  [[gnu::pure]]
  WaypointPtr FindHome() noexcept;

  /**
   * Set single home waypoint (clearing all others as home)
   *
   * @param id Id of waypoint to set as home
   * @return True on success (id was found)
   */
  bool SetHome(unsigned id) noexcept;

  /**
   * Look up waypoint by ID.
   *
   * @param id Id of waypoint to find in internal tree
   *
   * @return Pointer to waypoint if found (or nullptr if not)
   */
  [[gnu::pure]]
  WaypointPtr LookupId(unsigned id) const noexcept;

  /**
   * Look up closest waypoint by location within range
   *
   * @param loc Location of waypoint to find in internal tree
   * @param range Threshold for range
   *
   * @return Pointer to waypoint if found (or nullptr if none found)
   */
  [[gnu::pure]]
  WaypointPtr LookupLocation(const GeoPoint &loc,
                             double range = 0) const noexcept;

  /**
   * Look up waypoint by name (returns first match)
   *
   * @param name Name of waypoint to find in internal tree
   *
   * @return Pointer to waypoint if found (or nullptr if not)
   */
  [[gnu::pure]]
  WaypointPtr LookupName(tstring_view name) const noexcept;

  /**
   * Check if a waypoint with same name and approximate location
   * is already in the database.  If not, is appended to the database.
   *
   * @param waypoint Waypoint to check against (replaced)
   *
   * @return reference to waypoint in tree (either existing or appended)
   */
  WaypointPtr CheckExistsOrAppend(WaypointPtr waypoint) noexcept;

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
                        WaypointVisitor visitor) const;

  /**
   * Call visitor function on waypoints with the specified name
   * prefix.
   */
  void VisitNamePrefix(tstring_view prefix, WaypointVisitor visitor) const;

  /**
   * Returns a set of possible characters following the specified
   * prefix.
   */
  [[gnu::pure]]
  TCHAR *SuggestNamePrefix(tstring_view prefix,
                           TCHAR *dest, size_t max_length) const noexcept {
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
  [[gnu::pure]]
  WaypointPtr GetNearest(const GeoPoint &loc, double range) const noexcept;

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
  [[gnu::pure]]
  WaypointPtr GetNearestLandable(const GeoPoint &loc,
                                 double range) const noexcept;

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
  [[gnu::pure]]
  WaypointPtr GetNearestIf(const GeoPoint &loc, double range,
                           bool (*predicate)(const Waypoint &)) const noexcept;

  /**
   * Access first waypoint in store, for use in iterators.
   *
   * @return First waypoint in store
   */
  const_iterator begin() const noexcept {
    return waypoint_tree.begin();
  }

  /**
   * Access end waypoint in store, for use in iterators as end point.
   *
   * @return End waypoint in store
   */
  const_iterator end() const noexcept {
    return waypoint_tree.end();
  }
};
