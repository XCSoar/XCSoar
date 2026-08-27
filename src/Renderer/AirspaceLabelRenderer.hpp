// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AirspaceLabelList.hpp"
#include "AirspaceLabelPlacement.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicate.hpp"
#include "ui/dim/Rect.hpp"
#include "util/Serial.hpp"

#include <optional>
#include <unordered_map>

struct AirspaceLook;
struct MoreData;
struct DerivedInfo;
struct AirspaceComputerSettings;
struct AirspaceRendererSettings;
struct AirspaceWarningConfig;
class Airspaces;
class ProtectedAirspaceWarningManager;
class Canvas;
class LabelBlock;
class WindowProjection;
struct PixelPoint;

class AirspaceLabelRenderer
{
  const AirspaceLook &look;
  const Airspaces *airspaces = nullptr;
  const ProtectedAirspaceWarningManager *warning_manager = nullptr;

  /** Last successful slot per airspace, valid until the store or layout changes. */
  std::unordered_map<AirspaceLabelList::Identity, AirspaceLabelCandidate>
    placement_cache;
  Serial placement_cache_serial;
  Serial placement_cache_font_serial;
  PixelSize placement_cache_screen_size{};

public:
  explicit AirspaceLabelRenderer(const AirspaceLook &_look) noexcept
    :look(_look) {}

  const AirspaceLook &GetLook() const noexcept {
    return look;
  }

  const Airspaces *GetAirspaces() const noexcept {
    return airspaces;
  }

  const ProtectedAirspaceWarningManager *GetWarningManager() const noexcept {
    return warning_manager;
  }

  void SetAirspaces(const Airspaces *_airspaces) noexcept {
    airspaces = _airspaces;
    placement_cache.clear();
  }

  void SetAirspaceWarnings(
    const ProtectedAirspaceWarningManager *_warning_manager) noexcept {
    warning_manager = _warning_manager;
  }

  void Clear() noexcept {
    airspaces = nullptr;
    warning_manager = nullptr;
    placement_cache.clear();
  }

private:
  void DrawInternal(Canvas &canvas,
                    const WindowProjection &projection,
                    AirspacePredicate visible,
                    const AirspaceRendererSettings &settings,
                    const AirspaceWarningConfig &config,
                    bool draw_altitude_labels,
                    bool draw_notam_labels,
                    LabelBlock *label_block) noexcept;

  std::optional<AirspaceLabelCandidate>
  DrawLabel(Canvas &canvas, PixelPoint anchor,
            const PixelRect &map_rect,
            const AirspaceLabelList::Label &label,
            const AirspaceRendererSettings &settings,
            LabelBlock *label_block,
            std::optional<AirspaceLabelCandidate> preferred_candidate) noexcept;

public:
  /**
   * Draw labels that are visible according to standard rules.
   *
   * @param label_block Optional label block for overlap prevention;
   * nullptr to skip overlap checking.
   */
  void Draw(Canvas &canvas,
            const WindowProjection &projection,
            const MoreData &basic, const DerivedInfo &calculated,
            const AirspaceComputerSettings &computer_settings,
            const AirspaceRendererSettings &settings,
            LabelBlock *label_block = nullptr) noexcept;
};
