// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/Ptr.hpp"
#include "MapWindow/Preview/MapPreviewWindow.hpp"

class ProtectedAirspaceWarningManager;
struct MapLook;

/**
 * Small moving-map preview for #dlgAirspaceWarnings: terrain, topography,
 * airspaces (with warning state), task / waypoints / aircraft like the main map,
 * and a highlight outline on the selected warning airspace.
 */
class AirspaceWarningPreviewWindow final : public MapPreviewWindow {
public:
  AirspaceWarningPreviewWindow(const MapLook &map_look,
                               ProtectedAirspaceWarningManager &warnings
                               ) noexcept;

  ~AirspaceWarningPreviewWindow() noexcept override;

  void SetHighlight(ConstAirspacePtr airspace) noexcept;

protected:
  void PaintOverlays(Canvas &canvas) noexcept override;
};
