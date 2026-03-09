// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Settings.hpp"
#include "Client.hpp"
#include "time/PeriodClock.hpp"
#include "co/InjectTask.hxx"

struct MoreData;
struct DerivedInfo;
class CurlGlobal;

namespace PureTrack {

class Glue final {
  PeriodClock clock;
  Settings settings;
  Client client;
  Co::InjectTask inject_task;

public:
  explicit Glue(CurlGlobal &curl) noexcept;

  void SetSettings(const Settings &_settings) noexcept {
    settings = _settings;
  }

  void OnTimer(const MoreData &basic, const DerivedInfo &calculated);

private:
  Co::InvokeTask Tick(Settings settings, Sample sample);
  void OnCompletion(std::exception_ptr error) noexcept;
};

} // namespace PureTrack
