// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Renderer/AirspaceRendererSettings.hpp"

struct AirspaceLook;
class Canvas;
class ChartRenderer;
class Airspaces;
struct GeoPoint;
struct GeoVector;
struct AircraftState;

/**
 * A Window which renders a terrain and airspace cross-section
 */
class AirspaceXSRenderer
{
  AirspaceRendererSettings settings;

  const AirspaceLook &look;

public:
  AirspaceXSRenderer(const AirspaceLook &_look): look(_look) {}

  void Draw(Canvas &canvas, const ChartRenderer &chart,
            const Airspaces &database,
            const GeoPoint &start, const GeoVector &vec,
            const AircraftState &state) const;

  void SetSettings(const AirspaceRendererSettings &_settings) {
    settings = _settings;
  }
};
