/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Client.hpp"
#include "Serialiser.hpp"
#include "Geo/Boost/RangeBox.hpp"
#include "Tracking/SkyLines/Protocol.hpp"
#include "Tracking/SkyLines/Assemble.hpp"
#include "Tracking/SkyLines/Import.hpp"
#include "net/AddressInfo.hxx"
#include "net/Resolver.hxx"

#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/strategies/strategies.hpp>

CloudClientContainer::CloudClientContainer()
  :key_set(typename KeySet::bucket_traits(key_buckets, N_KEY_BUCKETS)) {}

CloudClientContainer::~CloudClientContainer()
{
  clear();
}

void
CloudClientContainer::clear()
{
  while (!list.empty())
    Remove(list.back());
}

CloudClient *
CloudClientContainer::Find(uint64_t key)
{
  auto i = key_set.find(key, key_set.hash_function(), key_set.key_eq());
  return i != key_set.end()
    ? &*i
    : nullptr;
}

CloudClient &
CloudClientContainer::Make(SocketAddress address,
                           uint64_t key,
                           const GeoPoint &location, int altitude)
{
  KeySet::insert_commit_data hint;
  auto result = key_set.insert_check(key, key_set.hash_function(),
                                     key_set.key_eq(), hint);
  if (result.second) {
    auto client = std::make_shared<CloudClient>(address, key, next_id++,
                                                location, altitude);
    Insert(*client);
    return *client;
  } else {
    auto &client = *result.first;
    Refresh(client, address, location, altitude);
    return client;
  }
}

void
CloudClientContainer::Refresh(CloudClient &client,
                              SocketAddress address)
{
  client.Refresh(address);

  list.erase(list.iterator_to(client));
  list.push_front(client);
}

void
CloudClientContainer::Refresh(CloudClient &client,
                              SocketAddress address,
                              const GeoPoint &location, int altitude)
{
  Refresh(client, address);

  if (location != client.location) {
    auto ptr = client.shared_from_this();
    rtree.remove(ptr);
    client.location = location;
    rtree.insert(ptr);
  }

  client.altitude = altitude;
}

void
CloudClientContainer::Insert(CloudClient &client)
{
  list.push_front(client);
  key_set.insert(client);
  id_set.push_back(client);
  rtree.insert(client.shared_from_this());
}

void
CloudClientContainer::Remove(CloudClient &client)
{
  list.erase(list.iterator_to(client));
  key_set.erase(key_set.iterator_to(client));
  id_set.erase(id_set.iterator_to(client));
  rtree.remove(client.shared_from_this());
}

void
CloudClientContainer::Expire(std::chrono::steady_clock::time_point before)
{
  while (!list.empty() && list.back().stamp < before)
    Remove(list.back());
}

CloudClientContainer::query_iterator_range
CloudClientContainer::QueryWithinRange(GeoPoint location, double range) const
{
  const auto q = boost::geometry::index::intersects(BoostRangeBox(location, range));
  return {rtree.qbegin(q), rtree.qend()};
}

inline Serialiser &
operator<<(Serialiser &s, SocketAddress address)
{
  char host[NI_MAXHOST], serv[NI_MAXSERV];
  int ret = getnameinfo(address.GetAddress(), address.GetSize(),
                        host, sizeof(host), serv, sizeof(serv),
                        NI_NUMERICHOST|NI_NUMERICSERV);
  s.WriteString(ret == 0 ? host : "unkown");
  s.Write16(address.GetPort());
  return s;
}

inline Deserialiser &
operator>>(Deserialiser &s, AllocatedSocketAddress &address)
{
  static constexpr auto hints =
    MakeAddrInfo(AI_NUMERICHOST, AF_UNSPEC, SOCK_DGRAM);

  const auto node = s.ReadString();
  char service[32];
  snprintf(service, sizeof(service), "%u", s.Read16());

  const auto ai = Resolve(node.c_str(), service, &hints);
  address = ai.front();

  return s;
}

void
CloudClient::Save(Serialiser &s) const
{
  s.Write32(id);

  s << stamp;

  uint32_t flags = SkyLinesTracking::FixPacket::FLAG_LOCATION;
  if (altitude != -1)
    flags |= SkyLinesTracking::FixPacket::FLAG_ALTITUDE;

  s.WriteT(SkyLinesTracking::MakeFix(key, flags, 0,
                                     location, Angle::Zero(), 0, 0,
                                     altitude, 0, 0));

  s << address;

  s.Write8(0);
}

CloudClient
CloudClient::Load(Deserialiser &s)
{
  const unsigned id = s.Read32();

  std::chrono::steady_clock::time_point stamp;
  s >> stamp;

  SkyLinesTracking::FixPacket fix;
  s.ReadT(fix);

  AllocatedSocketAddress address;
  s >> address;

  s.Read8();

  CloudClient client(std::move(address), FromBE64(fix.header.key),
                     id,
                     SkyLinesTracking::ImportGeoPoint(fix.location),
                     (int16_t)FromBE16(fix.altitude));
  client.stamp = stamp;
  return client;
}

void
CloudClientContainer::Save(Serialiser &s) const
{
  s.Write32(next_id);

  for (const auto &client : list) {
    s.Write8(1);
    client.Save(s);
  }

  s.Write8(0);
  s.Write8(0);
}

void
CloudClientContainer::Load(Deserialiser &s)
{
  next_id = s.Read32();

  while (s.Read8() != 0) {
    auto client = std::make_shared<CloudClient>(CloudClient::Load(s));
    Insert(*client);
  }

  s.Read8();
}
