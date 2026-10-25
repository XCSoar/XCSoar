// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"
#include "Client.hpp"
#include "Thermal.hpp"
#include "Observation/Database.hpp"
#include "Engine/Observation/Store.hpp"
#include "NMEA/Info.hpp"
#include "lib/curl/Global.hxx"
#include "LogFile.hpp"
#include "net/State.hpp"

#include <chrono>

namespace TIM {

Glue::Glue(CurlGlobal &_curl) noexcept
  :curl(_curl),
   task(curl.GetEventLoop())
{
}

Glue::~Glue() noexcept
{
  BeginShutdown();
}

void
Glue::BeginShutdown() noexcept
{
  {
    const std::lock_guard lock{mutex};
    observations = nullptr;
  }
  task.BeginShutdown();
}

void
Glue::SetObservations(ObservationDatabase *db) noexcept
{
  const std::lock_guard lock{mutex};
  observations = db;
}

void
Glue::OnTimer(const NMEAInfo &basic) noexcept
{
  if (task.IsShuttingDown())
    return;

  if (!basic.gps.real || !basic.location_available)
    /* we need a real GPS location */
    return;

  if (GetNetState() == NetState::DISCONNECTED)
    /* no link; do not start ThermalInfoMap request (see SkyLines Glue) */
    return;

  if (task.IsRunning())
    /* still running */
    return;

  if (!clock.CheckUpdate(std::chrono::minutes(1)))
    /* later */
    return;

  // TODO for some privacy, don't transmit exact location
  task.Start(Start(basic.location), BIND_THIS_METHOD(OnCompletion));
}

Co::InvokeTask
Glue::Start(const GeoPoint &location)
{
  auto downloaded = co_await GetThermals(curl, std::chrono::hours(1),
                                        location, 20);
  LogDebug("Downloaded {} thermals from ThermalInfoMap",
           downloaded.size());

  ThermalRows rows;
  rows.clear();
  const auto now = std::chrono::steady_clock::now();
  for (const auto &thermal : downloaded) {
    if (rows.full())
      break;
    if (!thermal.location.IsValid())
      continue;

    ObservedThermal row;
    row.origin = ObservationOrigin::TIM;
    row.location = thermal.location;
    row.ground_height = 0;
    row.lift_rate = thermal.climb_rate;
    row.received = now;
    rows.append(row);
  }

  ObservationDatabase *db;
  {
    const std::lock_guard lock{mutex};
    db = observations;
  }
  if (db != nullptr)
    db->ReplaceThermals(ObservationOrigin::TIM, rows);
}

void
Glue::OnCompletion(std::exception_ptr error) noexcept
{
  if (error)
    LogError(error, "ThermalInfoMap request failed");
}

} // namespace TIM
