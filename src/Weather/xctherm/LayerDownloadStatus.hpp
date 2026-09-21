// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/Features.hpp"

#ifdef HAVE_HTTP

#include "util/StaticString.hxx"

#include <cstdint>
#include <memory>
#include <string>

struct XCThermDownloadJob;

namespace XCTherm {

/**
 * Per-layer download metadata shown on the XC Therm weather page.
 */
struct LayerDownloadInfo {
  enum Status { NONE, PENDING, DONE, FAILED, CANCELED };
  Status status = NONE;
  double wire_mb = 0.0;
  double speed_mbs = 0.0;
  unsigned span_hours = 0;
  unsigned future_hours = 0;
  unsigned new_downloads = 0;
  unsigned pending_index = 0;
  unsigned pending_total = 0;
  uint64_t pending_bytes_now = 0;
  uint64_t pending_bytes_total = 0;
  unsigned retry_attempt = 0;
  unsigned retry_seconds_left = 0;
  std::string download_time;
  std::string issued_utc;
};

[[gnu::pure]]
LayerDownloadInfo *GetLayerDownloadInfo(unsigned model) noexcept;

void
MarkLayerPending(unsigned model, unsigned layer_index,
                 unsigned span_hours) noexcept;

void
CopyJobProgress(LayerDownloadInfo &row,
                const XCThermDownloadJob &job) noexcept;

void
RehydrateLayerRowsFromCache(unsigned model) noexcept;

StaticString<200>
FormatLayerStatus(unsigned model, unsigned layer_index,
                  unsigned span_hours_setting) noexcept;

enum class FinishDownloadResult : uint_least8_t {
  CANCELED,
  FAILED,
  PARTIAL,
  OK,
};

/**
 * Apply a finished span job to the status table and map preview.
 * The caller shows any error or partial-slice message.
 */
FinishDownloadResult
ApplyFinishedDownload(const std::shared_ptr<XCThermDownloadJob> &job) noexcept;

} // namespace XCTherm

#endif
