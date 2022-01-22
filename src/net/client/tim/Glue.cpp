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

#include "Glue.hpp"
#include "Client.hpp"
#include "Thermal.hpp"
#include "NMEA/Info.hpp"
#include "net/http/Global.hxx"
#include "LogFile.hpp"

namespace TIM {

Glue::Glue(CurlGlobal &_curl) noexcept
  :curl(_curl),
   inject_task(curl.GetEventLoop())
{
}

Glue::~Glue() noexcept = default;

void
Glue::OnTimer(const NMEAInfo &basic) noexcept
{
  if (!basic.gps.real || !basic.location_available)
    /* we need a real GPS location */
    return;

  if (inject_task)
    /* still running */
    return;

  if (!clock.CheckUpdate(std::chrono::minutes(1)))
    /* later */
    return;

  // TODO for some privacy, don't transmit exact location
  inject_task.Start(Start(basic.location), BIND_THIS_METHOD(OnCompletion));
}

Co::InvokeTask
Glue::Start(const GeoPoint &location)
{
  new_thermals = co_await GetThermals(curl, std::chrono::hours(1),
                                      location, 20);
}

void
Glue::OnCompletion(std::exception_ptr error) noexcept
{
  if (error)
    LogError(error, "ThermalInfoMap request failed");
  else
    LogDebug("Downloaded %u thermals from ThermalInfoMap",
             unsigned(new_thermals.size()));

  thermals = std::move(new_thermals);
}

} // namespace TIM
