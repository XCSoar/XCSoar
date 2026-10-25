// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/AsyncTask.hpp"
#include "thread/Mutex.hxx"
#include "time/PeriodClock.hpp"

struct NMEAInfo;
struct GeoPoint;
class CurlGlobal;
class ObservationDatabase;

/**
 * Client for ThermalInfoMap (https://thermalmap.info/api-doc.php)
 */
namespace TIM {

class Glue {
  CurlGlobal &curl;

  PeriodClock clock;

  mutable Mutex mutex;

  ObservationDatabase *observations = nullptr;

  Net::AsyncTask task;

public:
  explicit Glue(CurlGlobal &_curl) noexcept;
  ~Glue() noexcept;

  void BeginShutdown() noexcept;

  void SetObservations(ObservationDatabase *db) noexcept;

  void OnTimer(const NMEAInfo &basic) noexcept;

private:
  Co::InvokeTask Start(const GeoPoint &location);
  void OnCompletion(std::exception_ptr error) noexcept;
};

} // namespace TIM
