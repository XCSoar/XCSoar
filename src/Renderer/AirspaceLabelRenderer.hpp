// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AirspaceLabelList.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicate.hpp"

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

class AirspaceLabelRenderer
{
  const AirspaceLook &look;
  const Airspaces *airspaces = nullptr;
  const ProtectedAirspaceWarningManager *warning_manager = nullptr;

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
  }

  void SetAirspaceWarnings(const ProtectedAirspaceWarningManager *_warning_manager) noexcept {
    warning_manager = _warning_manager;
  }

  void Clear() noexcept {
    airspaces = nullptr;
    warning_manager = nullptr;
  }

private:
  void DrawInternal(Canvas &canvas,
                    const WindowProjection &projection,
                    AirspacePredicate visible,
                    const AirspaceWarningConfig &config,
                    bool draw_altitude_labels,
                    bool draw_notam_labels,
                    LabelBlock *label_block) noexcept;

  void DrawLabel(Canvas &canvas, const WindowProjection &projection,
                 const AirspaceLabelList::Label &label) noexcept;

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
