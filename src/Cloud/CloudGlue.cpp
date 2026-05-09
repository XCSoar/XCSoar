// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CloudGlue.hpp"
#include "Data.hpp"
#include "Dump.hpp"
#include "Sender.hpp"
#include "net/ToString.hxx"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <cstdio>

namespace {

/** SkyLines traffic time is ms since UTC midnight; clamp to one day. */
constexpr uint32_t
MsSinceMidnightUtc(std::chrono::milliseconds time_of_day) noexcept
{
  const auto c = time_of_day.count();
  if (c <= 0)
    return 0;

  constexpr auto day_ms = 86400000LL;
  if (c >= day_ms)
    return static_cast<uint32_t>(day_ms - 1);

  return static_cast<uint32_t>(c);
}

/**
 * One stdout line for this client: TAG, address, hex key, public id
 * (tab-separated), then tail_fmt / tail_args. fflush when stdout is piped.
 */
inline void
PrintCloudClientLogLine(const CloudClient &client, std::string_view tag,
                        fmt::string_view tail_fmt,
                        fmt::format_args tail_args)
{
  fmt::print(stdout,
             "{}\t{}\t{:x}\t{}\t",
             tag,
             ToString(client.address),
             client.key,
             client.id);
  fmt::vprint(stdout, tail_fmt, tail_args);
  fflush(stdout);
}

} // namespace

void
CloudGlue::OnFix(const SkyLinesTracking::Server::Client &c,
                 std::chrono::milliseconds time_of_day,
                 const GeoPoint &location, int altitude,
                 std::chrono::steady_clock::time_point now,
                 bool &schedule_expire)
{
  CloudClient *client = nullptr;
  schedule_expire = false;

  auto &clients = data.clients;

  if (location.IsValid()) {
    const bool was_empty = clients.empty();

    client = &clients.Make(c.address, c.key, location, altitude);
    client->traffic_time_ms = MsSinceMidnightUtc(time_of_day);

    const auto location_fmt = fmt::streamed(client->location);
    PrintCloudClientLogLine(*client, "FIX",
                           "{}\t{}m\n",
                           fmt::make_format_args(location_fmt,
                                                 client->altitude));

    if (was_empty)
      schedule_expire = true;
  } else {
    client = clients.Find(c.key);
    if (client != nullptr)
      clients.Refresh(*client, c.address);
  }

  if (client == nullptr)
    return;

  for (const auto &i : clients.QueryWithinRange(location,
                                                 policy.traffic_query_range_m)) {
    if (i->key == c.key)
      continue;

    if (now > i->wants_traffic)
      continue;

    TrafficResponseSender s(server, i->address, i->key);
    s.Add(client->id, client->traffic_time_ms,
          client->location, client->altitude);
    s.Flush();
  }
}

void
CloudGlue::OnTrafficRequest(const SkyLinesTracking::Server::Client &c,
                            bool near,
                            std::chrono::steady_clock::time_point now)
{
  if (!near)
    return;

  auto &clients = data.clients;

  auto *client = clients.Find(c.key);
  if (client == nullptr)
    return;

  client->wants_traffic = now + policy.subscription_ttl;

  const auto min_stamp = now - policy.live_max_traffic_age;

  TrafficResponseSender s(server, c.address, c.key);

  unsigned n = 0;
  for (const auto &traffic :
       clients.QueryWithinRange(client->location,
                                policy.traffic_query_range_m)) {
    if (traffic.get() == client)
      continue;

    if (traffic->stamp < min_stamp)
      continue;

    s.Add(traffic->id, traffic->traffic_time_ms,
          traffic->location, traffic->altitude);

    if (++n > policy.max_traffic_per_response)
      break;
  }

  s.Flush();
}

void
CloudGlue::OnWaveSubmit(const SkyLinesTracking::Server::Client &c,
                        [[maybe_unused]] std::chrono::milliseconds time_of_day,
                        const GeoPoint &a, const GeoPoint &b,
                        int bottom_altitude,
                        int top_altitude,
                        double lift)
{
  auto *client = data.clients.Find(c.key);
  if (client == nullptr)
    return;

  const auto a_fmt = fmt::streamed(a);
  const auto b_fmt = fmt::streamed(b);
  PrintCloudClientLogLine(*client, "WAVE",
                         "{}\t{}\t{}-{}m\t{}m/s\n",
                         fmt::make_format_args(a_fmt,
                                               b_fmt,
                                               bottom_altitude,
                                               top_altitude,
                                               lift));
}

void
CloudGlue::OnThermalSubmit(const SkyLinesTracking::Server::Client &c,
                           [[maybe_unused]] std::chrono::milliseconds time_of_day,
                           const GeoPoint &bottom_location,
                           int bottom_altitude,
                           const GeoPoint &top_location,
                           int top_altitude,
                           double lift,
                           std::chrono::steady_clock::time_point now)
{
  auto *client = data.clients.Find(c.key);
  if (client == nullptr)
    return;

  const auto top_fmt = fmt::streamed(top_location);
  PrintCloudClientLogLine(*client, "THERMAL",
                         "{}\t{}-{}m\t{}m/s\n",
                         fmt::make_format_args(top_fmt,
                                               bottom_altitude,
                                               top_altitude,
                                               lift));

  auto &thermals = data.thermals;
  auto &clients = data.clients;

  const auto &thermal =
    thermals.Make(c.key,
                  AGeoPoint(bottom_location, bottom_altitude),
                  AGeoPoint(top_location, top_altitude),
                  lift);

  for (const auto &i : clients.QueryWithinRange(bottom_location,
                                                policy.thermal_query_range_m)) {
    if (i->key == c.key)
      continue;

    if (now > i->wants_thermals)
      continue;

    ThermalResponseSender s(server, i->address, i->key);
    s.Add(thermal.Pack());
    s.Flush();
  }
}

void
CloudGlue::OnThermalRequest(const SkyLinesTracking::Server::Client &c,
                            std::chrono::steady_clock::time_point now)
{
  auto &clients = data.clients;
  auto &thermals = data.thermals;

  auto *client = clients.Find(c.key);
  if (client == nullptr)
    return;

  client->wants_thermals = now + policy.subscription_ttl;

  const auto min_time = now - policy.live_max_thermal_age;

  ThermalResponseSender s(server, c.address, c.key);

  unsigned n = 0;
  for (const auto &thermal :
       thermals.QueryWithinRange(client->location,
                                 policy.thermal_query_range_m)) {
    if (thermal->client_key == c.key)
      continue;

    if (thermal->time < min_time)
      continue;

    s.Add(thermal->Pack());

    if (++n > policy.max_thermal_per_response)
      break;
  }

  s.Flush();
}
