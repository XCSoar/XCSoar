// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/xctherm/XCThermGeoJSON.hpp"
#include "co/Task.hxx"

#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <string>

/**
 * Cross-thread state for a multi-hour XCTherm span download.
 */
struct XCThermDownloadJob {
  unsigned model = 0;
  int target_index = -1;
  std::string target_label;
  std::string param;
  unsigned span_hours = 0;
  unsigned current_utc = 12;

  std::atomic<bool> cancel{false};
  std::atomic<bool> done{false};
  std::atomic<unsigned> current_offset{0};
  std::atomic<uint64_t> bytes_now{0};
  std::atomic<uint64_t> bytes_total{0};
  std::atomic<uint64_t> total_wire_bytes{0};
  std::atomic<unsigned> succeeded_or_cached{0};
  std::atomic<unsigned> newly_downloaded{0};
  std::atomic<unsigned> retry_attempt{0};
  std::atomic<unsigned> retry_seconds_left{0};
  std::atomic<bool> any_slot_missing{false};
  std::atomic<bool> index_no_parameters{false};

  std::mutex result_mutex;
  XCThermGeoJSON::ForecastLayer first_forecast;
  std::string latest_run_date;
  std::string latest_run_hour;
  std::chrono::steady_clock::time_point started_at;
  std::chrono::steady_clock::time_point finished_at;

  std::exception_ptr error_eptr;
};

class CurlGlobal;
struct XCThermSettings;

/**
 * Build a span download job for @p layer_index (region from @p settings).
 */
std::shared_ptr<XCThermDownloadJob>
MakeXCThermSpanJob(const XCThermSettings &settings, unsigned layer_index,
                   unsigned current_utc) noexcept;

/**
 * Multi-hour download loop on @p curl's event loop. Updates @p job
 * atomics and sets @c done when finished.
 */
Co::Task<void>
RunXCThermDownload(CurlGlobal &curl,
                    const std::shared_ptr<XCThermDownloadJob> &job);
