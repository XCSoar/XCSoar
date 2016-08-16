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

#include "Thermal.hpp"
#include "Serialiser.hpp"
#include "Geo/Boost/RangeBox.hpp"
#include "Tracking/SkyLines/Protocol.hpp"
#include "Tracking/SkyLines/Assemble.hpp"
#include "Tracking/SkyLines/Import.hpp"

CloudThermalContainer::CloudThermalContainer()
{
}

CloudThermalContainer::~CloudThermalContainer()
{
  clear();
}

void
CloudThermalContainer::clear()
{
  while (!list.empty())
    Remove(list.back());
}

CloudThermal &
CloudThermalContainer::Make(uint64_t client_key,
                            const AGeoPoint &bottom_location,
                            const AGeoPoint &top_location,
                            double lift)
{
  auto thermal = std::make_shared<CloudThermal>(client_key, bottom_location,
                                                top_location, lift);
  Insert(*thermal);
  return *thermal;
}

void
CloudThermalContainer::Insert(CloudThermal &thermal)
{
  list.push_front(thermal);
  rtree.insert(thermal.shared_from_this());
}

void
CloudThermalContainer::Remove(CloudThermal &thermal)
{
  list.erase(list.iterator_to(thermal));
  rtree.remove(thermal.shared_from_this());
}

void
CloudThermalContainer::Expire(std::chrono::steady_clock::time_point before)
{
  while (!list.empty() && list.back().time < before)
    Remove(list.back());
}

CloudThermalContainer::query_iterator_range
CloudThermalContainer::QueryWithinRange(GeoPoint location, double range) const
{
  const auto q = boost::geometry::index::intersects(BoostRangeBox(location, range));
  return {rtree.qbegin(q), rtree.qend()};
}

SkyLinesTracking::Thermal
CloudThermal::Pack() const
{
  // TODO: fill "time" properly
  return SkyLinesTracking::MakeThermal(0, bottom_location,
                                       bottom_location.altitude,
                                       top_location,
                                       top_location.altitude,
                                       lift);
}

void
CloudThermal::Save(Serialiser &s) const
{
  s.Write8(1);
  s.Write64(client_key);
  s << time;
  s.WriteT(Pack());
}

CloudThermal
CloudThermal::Load(Deserialiser &s)
{
  s.Read8();
  const unsigned client_key = s.Read64();

  std::chrono::steady_clock::time_point time;
  s >> time;

  SkyLinesTracking::Thermal t;
  s.ReadT(t);

  CloudThermal thermal(client_key,
                       AGeoPoint(SkyLinesTracking::ImportGeoPoint(t.bottom_location),
                                 FromBE16(t.bottom_altitude)),
                       AGeoPoint(SkyLinesTracking::ImportGeoPoint(t.top_location),
                                 FromBE16(t.top_altitude)),
                       FromBE16(t.lift) / 256.);
  thermal.time = time;
  return thermal;
}

void
CloudThermalContainer::Save(Serialiser &s) const
{
  s.Write8(1);

  for (const auto &thermal : list) {
    s.Write8(1);
    thermal.Save(s);
  }

  s.Write8(0);
  s.Write8(0);
}

void
CloudThermalContainer::Load(Deserialiser &s)
{
  s.Read8();

  while (s.Read8() != 0) {
    auto thermal = std::make_shared<CloudThermal>(CloudThermal::Load(s));
    Insert(*thermal);
  }

  s.Read8();
}
