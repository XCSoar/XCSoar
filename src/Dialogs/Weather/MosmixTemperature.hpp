// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Atmosphere/Temperature.hpp"
#include "Geo/GeoPoint.hpp"
#include "co/InjectTask.hxx"
#include "ui/event/Notify.hpp"

#include <exception>
#include <functional>
#include <optional>

/**
 * Fetches today's forecast maximum temperature in the background.
 *
 * The flight setup dialog is opened to set the QNH as much as
 * anything else, often in a hurry and sometimes in the air, so it
 * must not wait for a network.  It opens on the value it has, and the
 * field is filled in if and when an answer arrives.
 *
 * Owned by the widget that wants the answer.  Destroying it cancels
 * the fetch, so the callback cannot reach a window that is gone.
 */
class ForecastTemperatureFetcher final {
  /* constructed on first use: Co::InjectTask wants an
     #EventLoop, and there may be none to give it */
  std::optional<Co::InjectTask> task;

  UI::Notify complete_notify{[this]{ OnCompleteNotify(); }};

  std::function<void(Temperature)> on_result;

  std::optional<Temperature> result;
  std::exception_ptr error;

  /** the day the answer is about, remembered for the profile */
  unsigned year = 0, month = 0, day = 0;

  /* read in Start() on the main thread: Run() is a coroutine on the
     AsioThread's event loop, where CommonInterface must not be touched */
  GeoPoint location = GeoPoint::Invalid();

  Co::InvokeTask Run();
  void OnCompletion(std::exception_ptr error) noexcept;
  void OnCompleteNotify() noexcept;

public:
  ForecastTemperatureFetcher() noexcept = default;

  ~ForecastTemperatureFetcher() noexcept {
    if (task)
      task->Cancel();
  }

  ForecastTemperatureFetcher(const ForecastTemperatureFetcher &) = delete;
  ForecastTemperatureFetcher &
  operator=(const ForecastTemperatureFetcher &) = delete;

  /**
   * Start fetching, if a fetch is due at all.
   *
   * @param callback run on the UI thread when a temperature arrives;
   * never run when none does
   */
  void Start(std::function<void(Temperature)> &&callback) noexcept;
};
