// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
