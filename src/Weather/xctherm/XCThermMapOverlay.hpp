// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/xctherm/XCThermGeoJSON.hpp"
#include "util/StaticString.hxx"

#include <functional>
#include <memory>
#include <string>

struct XCThermDownloadJob;
struct XCThermSettings;

namespace XCTherm {

/**
 * Parse @p geojson and install it as the main map XCTherm overlay.
 *
 * @param label user-visible layer name (e.g. @c "5000m AMSL")
 * @param parameter API parameter for map-item metadata (optional)
 * @param forecast_utc valid UTC hour for cache lookup (optional)
 */
void ApplyForecastToMap(const std::string &geojson, const char *label,
                        const char *parameter = nullptr,
                        unsigned forecast_utc = 0) noexcept;

/**
 * Install an already-parsed forecast as the main map overlay.
 */
void ApplyForecastLayerToMap(XCThermGeoJSON::ForecastLayer &&forecast,
                             const char *label,
                             const char *parameter = nullptr,
                             unsigned forecast_utc = 0) noexcept;

void ClearMapOverlay() noexcept;

/**
 * Dedicated XCTherm page lifecycle (mirrors EDL pan suspension).
 */
bool EnterDedicatedPage() noexcept;
void LeaveDedicatedPage() noexcept;
void SuspendDedicatedPageForPan() noexcept;
void ResumeDedicatedPageAfterPan() noexcept;
bool IsDedicatedPageSuspendedForPan() noexcept;

/**
 * Compact label for the active forecast overlay (map-scale PAN string).
 */
StaticString<64> GetPanOverlayLabel() noexcept;

/**
 * Apply a cached slice to the map.
 * @return false when nothing is cached for @p layer_index at @p utc_hour.
 */
bool ApplyCachedLayerOverlay(unsigned layer_index,
                             unsigned utc_hour) noexcept;

/**
 * Apply the best cached hour for @p layer_index (auto-time :45 rule).
 */
bool ApplyCachedLayerOverlayAuto(unsigned layer_index) noexcept;

/**
 * Apply the activated (or first cached) layer from cache to the map.
 */
void RestoreActiveLayerOverlay() noexcept;

/**
 * Install the first parseable slice from a finished download job.
 */
void ApplyJobPreviewToMap(const std::shared_ptr<XCThermDownloadJob> &job) noexcept;

/**
 * Start a span download on the network glue.
 * @return nullptr when prerequisites are missing or glue is busy.
 */
std::shared_ptr<XCThermDownloadJob>
StartSpanDownload(
  const XCThermSettings &settings, unsigned layer_index,
  std::function<void(std::shared_ptr<XCThermDownloadJob>)> on_finished);

/**
 * Fetch index.json on the network thread when needed.
 */
void RequestBackgroundIndexFetch(std::function<void()> on_ready = nullptr);

/**
 * Start a span download for the activated layer when it has no cache.
 */
void MaybeFetchActiveLayerSpan(
  std::function<void(std::shared_ptr<XCThermDownloadJob>)> on_finished =
    nullptr);

/**
 * Restore overlay from cache and ensure index / active-layer data.
 */
void ActivatePageOverlay() noexcept;

/** Allow a new auto-fetch after the activated layer changed. */
void ResetAutoFetchAttempt() noexcept;

} // namespace XCTherm
