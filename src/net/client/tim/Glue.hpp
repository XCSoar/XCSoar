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

#pragma once

#include "co/InjectTask.hxx"
#include "event/DeferEvent.hxx"
#include "time/PeriodClock.hpp"

#include <vector>

struct NMEAInfo;
struct GeoPoint;
class CurlGlobal;

/**
 * Client for ThermalInfoMap (https://thermalmap.info/api-doc.php)
 */
namespace TIM {

struct Thermal;

class Glue {
  CurlGlobal &curl;

  PeriodClock clock;

  std::vector<Thermal> thermals;

  std::vector<Thermal> new_thermals;

  Co::InjectTask inject_task;

public:
  explicit Glue(CurlGlobal &_curl) noexcept;
  ~Glue() noexcept;

  const auto &Get() const noexcept {
    return thermals;
  }

  void OnTimer(const NMEAInfo &basic) noexcept;

private:
  Co::InvokeTask Start(const GeoPoint &location);
  void OnCompletion(std::exception_ptr error) noexcept;
};

} // namespace TIM
