/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#ifndef AIRSPACE_ROUTE_HPP
#define AIRSPACE_ROUTE_HPP

#include <utility>
#include <algorithm>
#include <set>
#include "Task/Tasks/PathSolvers/AStar.hpp"
#include "Navigation/SearchPoint.hpp"
#include "Airspace/Airspaces.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "Navigation/SearchPointVector.hpp"

typedef std::vector<GeoPoint> Route;

class AirspaceRoute {
public:
  friend class PrintHelper;

#define ROUTE_MIN_STEP 1

  AirspaceRoute(const Airspaces& master);

  ~AirspaceRoute();

  void synchronise(const Airspaces& master,
                   const GeoPoint& origin,
                   const GeoPoint& destination,
                   const AirspacePredicate &condition=AirspacePredicate::always_true);

  typedef FlatGeoPoint RoutePoint;
  typedef std::pair< const AbstractAirspace*, RoutePoint > RouteIntersection;
  typedef std::pair< RoutePoint, RoutePoint> RouteLink;

  bool solve(const GeoPoint& origin,
             const GeoPoint& destination);

  void get_solution(Route& route) const;

  void reset();

  unsigned airspace_size() const;

private:

  void link_cleared(const RouteLink &e);

  void add_candidate(const RouteLink e);

  void add_edges(const RouteLink &e);

  void add_nearby(const RouteIntersection &inx, const RouteLink& e);

  const GeoVector m_vector;
  Airspaces m_airspaces;
  AStar<RoutePoint> m_planner;

  typedef std::set< RouteLink> RouteLinkSet;
  RouteLinkSet m_unique;
  typedef std::queue< RouteLink> RouteLinkQueue;
  RouteLinkQueue m_links;

  RoutePoint m_astar_goal;

  RouteIntersection
  first_intersecting(const RouteLink& e) const;

  RouteIntersection
  first_intersecting(const GeoPoint& origin,
                     const GeoPoint& destination) const;

  const AbstractAirspace*
  inside_others(const GeoPoint& origin) const;

  /**
   * Distance function for free point
   *
   * @param p1 First node
   * @param p2 Second node
   *
   * @return Distance (flat) from origin to destination
   */
  gcc_pure
  unsigned distance(const RouteLink &e) const {
    return e.first.distance_to(e.second);
  }

  gcc_pure
  bool is_short(const RouteLink &e) const {
    return (abs(e.first.Longitude-e.second.Longitude)<ROUTE_MIN_STEP)
      && (abs(e.first.Latitude-e.second.Latitude)<ROUTE_MIN_STEP);
  }

  typedef std::pair<FlatGeoPoint,
                    FlatGeoPoint> ClearingPair;

  ClearingPair find_clearing_pair(const SearchPointVector& spv,
                                  const SearchPointVector::const_iterator start,
                                  const SearchPointVector::const_iterator end,
                                  const FlatGeoPoint dest) const;

  ClearingPair get_pairs(const SearchPointVector& spv,
                         const RoutePoint start,
                         const RoutePoint dest) const;

  ClearingPair get_backup_pairs(const SearchPointVector& spv,
                                const RoutePoint intc) const;

  SearchPointVector::const_iterator find_clearing(
    const SearchPointVector& spv,
    const SearchPointVector::const_iterator start,
    const SearchPointVector::const_iterator end,
    const RoutePoint destination,
    bool backwards) const;

  unsigned find_solution(const RoutePoint &final, Route& this_route) const;

  bool add_unique(const RoutePoint origin, const RoutePoint destination);

  Route solution_route;

  FlatGeoPoint origin_last;
  FlatGeoPoint destination_last;
  bool dirty;

  mutable unsigned long count_dij;
  mutable unsigned long count_airspace;
  mutable unsigned long count_unique;
};

#endif
