// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Sender.hpp"
#include "Tracking/SkyLines/Export.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/CRC16CCITT.hpp"

template<typename Traits>
void
SkyLinesBatchResponseSender<Traits>::Flush() noexcept
{
  if (n_items == 0)
    return;

  const size_t size =
    sizeof(data.header) + sizeof(typename Traits::Item) * n_items;

  Traits::SetCount(data.header, n_items);
  n_items = 0;

  data.header.header.crc = 0;
  data.header.header.crc = ToBE16(UpdateCRC16CCITT(&data, size, 0));
  server.SendBuffer(address, {(const std::byte *)&data, size});
}

template class SkyLinesBatchResponseSender<CloudSenderTraits::ThermalBatch>;
template class SkyLinesBatchResponseSender<CloudSenderTraits::TrafficBatch>;

void
TrafficResponseSender::Add(uint32_t pilot_id, uint32_t time,
                             GeoPoint location, int altitude)
{
  typename CloudSenderTraits::TrafficBatch::Item traffic{};
  traffic.pilot_id = ToBE32(pilot_id);
  traffic.time = ToBE32(time);
  traffic.location = SkyLinesTracking::ExportGeoPoint(location);
  traffic.altitude = ToBE16(altitude);
  traffic.reserved = 0;
  traffic.reserved2 = 0;

  SkyLinesBatchResponseSender::Add(traffic);
}
