// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"
#include "Client.hpp"
#include "Thermal.hpp"
#include "NMEA/Info.hpp"
#include "lib/curl/Global.hxx"
#include "LogFile.hpp"
#include "net/State.hpp"

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
  task.BeginShutdown();
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
  auto new_thermals = co_await GetThermals(curl, std::chrono::hours(1),
                                           location, 20);
  LogDebug("Downloaded {} thermals from ThermalInfoMap",
           new_thermals.size());

  const auto lock = Lock();
  thermals = std::move(new_thermals);
}

void
Glue::OnCompletion(std::exception_ptr error) noexcept
{
  if (error)
    LogError(error, "ThermalInfoMap request failed");
}

} // namespace TIM
