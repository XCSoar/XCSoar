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
#ifndef WAYPOINTS_HPP
#define WAYPOINTS_HPP

#include "Util/NonCopyable.hpp"
#include <kdtree++/kdtree.hpp>
#include "WaypointEnvelope.hpp"
#ifdef DO_PRINT
#include <iostream>
#endif
#include <deque>

#include "Navigation/TaskProjection.hpp"

class WaypointVisitor;

/**
 * Container for waypoints using kd-tree representation internally for fast 
 * geospatial lookups.
 */
class Waypoints: private NonCopyable 
{
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
  void append(const Waypoint& wp);

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
 * @param wp Waypoint to erase from internal store
 */
  void replace(const Waypoint& orig, Waypoint& replacement);

/** 
 * Create new waypoint (without appending it to the store),
 * with set id.  This is like a factory method.
 * 
 * @param location Location of waypoint
 * 
 * @return Blank object at given location, with id set
 */
  Waypoint create(const GEOPOINT& location);

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
 * 
 */
  void clear();

/** 
 * Size of waypoints (in tree, not in temporary store) ---
 * must call optimise() before this for it to be accurate.
 * 
 * @return Number of waypoints in tree
 */
  unsigned size() const;

/** 
 * Whether waypoints store is empty
 * 
 * @return True if no waypoints stored
 */
  bool empty() const;

/** 
 * Set whether first waypoint file will be writable
 * (this is used for create() method of waypoints not
 *  generated from the Waypointparser)
 *
 * @param set Set/unset writable
 */
  void set_file0_writable(const bool set);

/** 
 * Determine whether a waypoint can be edited
 * based on the writability of the file it is assigned to 
 *
 * @param wp Waypoint to check
 * 
 * @return True if waypoint can be edited
 */
  bool get_writable(const Waypoint& wp) const;

/** 
 * Find first home waypoint
 * 
 * @return Pointer to waypoint if found (or NULL if not)
 */
  const Waypoint* find_home() const;

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
 * Look up waypoint by location (returns first match)
 * 
 * @param loc Location of waypoint to find in internal tree
 * @param range Threshold for range
 * 
 * @return Pointer to waypoint if found (or NULL if none found)
 */
  const Waypoint* lookup_location(const GEOPOINT &loc,
                                  const fixed range= fixed_zero) const;

/** 
 * Look up waypoint by name (returns first match)
 * 
 * @param loc Name of waypoint to find in internal tree
 * 
 * @return Pointer to waypoint if found (or NULL if not)
 */
  const Waypoint* lookup_name(const tstring &name) const;

/** 
 * Call visitor function on waypoints within approximate range
 * (square range box) to search location.  Possible use by screen display
 * functions.
 * 
 * @param loc Location from which to search
 * @param range Distance in meters of search radius
 * @param visitor Visitor to be called on waypoints within range
 */
  void visit_within_range(const GEOPOINT &loc, const fixed range,
                          WaypointVisitor& visitor) const;

/** 
 * Call visitor function on waypoints within radius
 * to search location.
 * 
 * @param loc Location from which to search
 * @param range Distance in meters of search radius
 * @param visitor Visitor to be called on waypoints within range
 */
  void visit_within_radius(const GEOPOINT &loc, const fixed range,
                           WaypointVisitor& visitor) const;

  /**
   * Type of KD-tree data structure for waypoint container
   */
  typedef KDTree::KDTree<2, 
                         WaypointEnvelope, 
                         WaypointEnvelope::kd_get_location
                         > WaypointTree;

/** 
 * Looks up nearest waypoint to the search location.
 * Performs search according to flat-earth internal representation,
 * so is approximate.
 * 
 * @param loc Location from which to search
 * 
 * @return Iterator to absolute nearest waypoint 
 */
  WaypointTree::const_iterator find_nearest(const GEOPOINT &loc) const;

/** 
 * Look up waypoint by ID.
 * 
 * @param id Id of waypoint to find in internal tree
 * 
 * @return Iterator to matching waypoint (or end if not found)
 */
  WaypointTree::const_iterator find_id(const unsigned id) const;

/** 
 * Access first waypoint in store, for use in iterators.
 * 
 * @return First waypoint in store
 */
  WaypointTree::const_iterator begin() const;

/** 
 * Access end waypoint in store, for use in iterators as end point.
 * 
 * @return End waypoint in store
 */
  WaypointTree::const_iterator end() const;

private:

/** 
 * Find waypoints within approximate range (square range box)
 * to search location.  Possible use by screen display functions.
 * 
 * @param loc Location from which to search
 * @param range Distance in meters of search radius
 * 
 * @return Vector of waypoints within square range
 */
  std::vector< WaypointEnvelope >
    find_within_range(const GEOPOINT &loc, const fixed range) const;

  WaypointTree waypoint_tree;
  TaskProjection task_projection;

  std::deque< WaypointEnvelope > tmp_wps;

  bool m_file0_writable;
};

#endif
