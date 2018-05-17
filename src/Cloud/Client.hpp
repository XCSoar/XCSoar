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

#ifndef XCSOAR_CLOUD_CLIENT_HPP
#define XCSOAR_CLOUD_CLIENT_HPP

#include "Geo/Boost/GeoPoint.hpp"

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/asio/ip/udp.hpp>

#include <memory>
#include <chrono>

class Serialiser;
class Deserialiser;

/**
 * A client which has submitted data to us recently.
 */
struct CloudClient
  : std::enable_shared_from_this<CloudClient>,
    boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>,
    boost::intrusive::set_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>,
    boost::intrusive::unordered_set_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>
{
  /**
   * Last known IP address.
   */
  boost::asio::ip::udp::endpoint endpoint;

  /**
   * "Internal" id of this client, i.e. the secret key from
   * #Header::key.
   */
  const uint64_t key;

  /**
   * "Public" id of this client, to be used for
   * #TrafficResponsePacket::Traffic::pilot_id.
   */
  const unsigned id;

  /**
   * Time when we most recently received data.  (Montonic server-side
   * clock.)
   */
  std::chrono::steady_clock::time_point stamp;

  /**
   * The client wishes to receive traffic information until this time
   * stamp.
   */
  std::chrono::steady_clock::time_point wants_traffic =
    std::chrono::steady_clock::time_point::min();

  /**
   * The client wishes to receive thermal information until this time
   * stamp.
   */
  std::chrono::steady_clock::time_point wants_thermals =
    std::chrono::steady_clock::time_point::min();

  /**
   * Last known location.  This is always "defined", because clients
   * without a location are not tracked.
   */
  GeoPoint location;

  /**
   * Last known altitude.
   */
  int altitude;

  struct KeyHash {
    constexpr std::size_t operator()(uint64_t key) const {
      return key;
    }

    gcc_pure
    std::size_t operator()(const CloudClient &client) const {
      return client.key;
    }
  };

  struct KeyEqual {
    gcc_pure
    bool operator()(const CloudClient &a, const CloudClient &b) const {
      return a.key == b.key;
    }

    gcc_pure
    bool operator()(uint64_t a, const CloudClient &b) const {
      return a == b.key;
    }
  };

  struct IdCompare {
    gcc_pure
    bool operator()(const CloudClient &a, const CloudClient &b) const {
      return a.id < b.id;
    }

    gcc_pure
    bool operator()(unsigned a, const CloudClient &b) const {
      return a < b.id;
    }
  };

  CloudClient(const boost::asio::ip::udp::endpoint &_endpoint, uint64_t _key,
              unsigned _id,
              const GeoPoint &_location, int _altitude)
    :endpoint(_endpoint), key(_key), id(_id),
     stamp(std::chrono::steady_clock::now()),
     location(_location), altitude(_altitude) {}

  void Refresh(const boost::asio::ip::udp::endpoint &_endpoint) {
      endpoint = _endpoint;
      stamp = std::chrono::steady_clock::now();
  }

  void Save(Serialiser &s) const;
  static CloudClient Load(Deserialiser &s);
};

using CloudClientPtr = std::shared_ptr<CloudClient>;

/**
 * Helper for boost::geometry::index::rtree.
 */
struct CloudClientIndexable {
  typedef GeoPoint result_type;

  gcc_pure
  result_type operator()(const CloudClientPtr &client) const {
    return client->location;
  }
};

class CloudClientContainer {
  typedef boost::geometry::index::rtree<CloudClientPtr, boost::geometry::index::rstar<16>,
                                        CloudClientIndexable> Tree;

  typedef boost::intrusive::list<CloudClient,
                                 boost::intrusive::constant_time_size<false>> List;

  typedef boost::intrusive::unordered_set<CloudClient,
                                          boost::intrusive::hash<CloudClient::KeyHash>,
                                          boost::intrusive::equal<CloudClient::KeyEqual>,
                                          boost::intrusive::constant_time_size<false>> KeySet;

  typedef boost::intrusive::set<CloudClient,
                                boost::intrusive::compare<CloudClient::IdCompare>,
                                boost::intrusive::constant_time_size<false>> IdSet;

  /**
   * A geospatial container of all clients, for fast geographic
   * lookups.
   */
  Tree rtree;

  /**
   * A linked list of clients, sorted by last fix, with fresh items at
   * the front.
   */
  List list;

  /**
   * Map (secret) key to #CloudClient.
   */
  KeySet key_set;

  /**
   * Map public id to #CloudClient.
   */
  IdSet id_set;

  /**
   * The public id assigned to the next new #CloudClient.
   */
  unsigned next_id = 1;

  static constexpr size_t N_KEY_BUCKETS = 65521;
  typename KeySet::bucket_type key_buckets[N_KEY_BUCKETS];

public:
  CloudClientContainer();
  ~CloudClientContainer();

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
   * Look up a client by its secret key.  Note that this does not
   * increment the reference counter.
   */
  gcc_pure
  CloudClient *Find(uint64_t key);

  /**
   * Create a new #CloudClient, or refresh the existing one.
   */
  CloudClient &Make(const boost::asio::ip::udp::endpoint &endpoint,
                    uint64_t key, const GeoPoint &location, int altitude);

  void Refresh(CloudClient &client,
               const boost::asio::ip::udp::endpoint &endpoint);

  void Refresh(CloudClient &client,
               const boost::asio::ip::udp::endpoint &endpoint,
               const GeoPoint &location, int altitude);

  void Insert(CloudClient &client);

  /**
   * Remove a #CloudClient and its data.  Be careful - the given reference
   * is invalidated, unless the caller holds another #CloudClientPtr.
   */
  void Remove(CloudClient &client);

  void Expire(std::chrono::steady_clock::time_point before);

  typedef Tree::const_query_iterator query_iterator;
  typedef boost::iterator_range<query_iterator> query_iterator_range;

  gcc_pure
  query_iterator_range QueryWithinRange(GeoPoint location, double range) const;

  void Save(Serialiser &s) const;
  void Load(Deserialiser &s);
};

#endif
