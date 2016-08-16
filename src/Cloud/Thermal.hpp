/*
Copyright_License {

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

#ifndef XCSOAR_CLOUD_THERMAL_HPP
#define XCSOAR_CLOUD_THERMAL_HPP

#include "Geo/Boost/GeoPoint.hpp"

#include <boost/intrusive/list.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/asio/ip/udp.hpp>

#include <memory>
#include <chrono>

class Serialiser;
class Deserialiser;
namespace SkyLinesTracking { struct Thermal; }

/**
 * A client which has submitted data to us recently.
 */
struct CloudThermal
  : std::enable_shared_from_this<CloudThermal>,
    boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>
{
  const uint64_t client_key;

  /**
   * Time when this thermal was measured or received.  (Montonic
   * server-side clock.)
   */
  std::chrono::steady_clock::time_point time;

  AGeoPoint bottom_location, top_location;

  double lift;

  CloudThermal(uint64_t _client_key,
               const AGeoPoint &_bottom_location,
               const AGeoPoint &_top_location,
               double _lift)
    :client_key(_client_key),
     time(std::chrono::steady_clock::now()),
     bottom_location(_bottom_location), top_location(_top_location),
     lift(_lift) {}

  gcc_pure
  SkyLinesTracking::Thermal Pack() const;

  void Save(Serialiser &s) const;
  static CloudThermal Load(Deserialiser &s);
};

using CloudThermalPtr = std::shared_ptr<CloudThermal>;

/**
 * Helper for boost::geometry::index::rtree.
 */
struct CloudThermalIndexable {
  typedef GeoPoint result_type;

  gcc_pure
  result_type operator()(const CloudThermalPtr &client) const {
    return client->top_location;
  }
};

class CloudThermalContainer {
  typedef boost::geometry::index::rtree<CloudThermalPtr, boost::geometry::index::rstar<16>,
                                        CloudThermalIndexable> Tree;

  typedef boost::intrusive::list<CloudThermal,
                                 boost::intrusive::constant_time_size<false>> List;

  /**
   * A geospatial container of all thermals, for fast geographic
   * lookups.
   */
  Tree rtree;

  /**
   * A linked list of thermals, sorted by time, with newer items at
   * the front.
   */
  List list;

public:
  CloudThermalContainer();
  ~CloudThermalContainer();

  void clear();

  bool empty() const {
    return list.empty();
  }

  /**
   * For iteration over the list of all clients in unspecified order.
   * The iterators get invalidated by all modifying calls.
   */
  List::const_iterator begin() const {
    return list.begin();
  }

  List::const_iterator end() const {
    return list.end();
  }

  /**
   * Create a new #CloudThermal, or refresh the existing one.
   */
  CloudThermal &Make(uint64_t client_key,
                     const AGeoPoint &bottom_location,
                     const AGeoPoint &top_location,
                     double lift);

  void Insert(CloudThermal &client);

  /**
   * Remove a #CloudThermal and its data.  Be careful - the given reference
   * is invalidated, unless the caller holds another #CloudThermalPtr.
   */
  void Remove(CloudThermal &client);

  void Expire(std::chrono::steady_clock::time_point before);

  typedef Tree::const_query_iterator query_iterator;
  typedef boost::iterator_range<query_iterator> query_iterator_range;

  gcc_pure
  query_iterator_range QueryWithinRange(GeoPoint location, double range) const;

  void Save(Serialiser &s) const;
  void Load(Deserialiser &s);
};

#endif
