// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/BufferWindow.hpp"
#include "CrossSectionRenderer.hpp"
#include "Look/InfoBoxLook.hpp"

struct CrossSectionLook;
struct AirspaceLook;
struct ChartLook;
struct MoreData;
struct DerivedInfo;
struct MapSettings;
class Airspaces;
class RasterTerrain;

/**
 * A Window which renders a terrain and airspace cross-section
 */
class CrossSectionWindow
  : public BufferWindow
{
protected:
  CrossSectionRenderer renderer;

public:
  /**
   * Constructor. Initializes most class members.
   */
  CrossSectionWindow(const CrossSectionLook &look,
                     const AirspaceLook &airspace_look,
                     const ChartLook &chart_look,
                     const InfoBoxLook &info_box_look):
      renderer(look, airspace_look, chart_look, info_box_look.inverse) {}

  void ReadBlackboard(const MoreData &basic,
                      const DerivedInfo &calculated,
                      const GlideSettings &glide_settings,
                      const GlidePolar &glide_polar,
                      const MapSettings &map_settings) {
    renderer.ReadBlackboard(basic, calculated,
                            glide_settings, glide_polar,
                            map_settings);
    Invalidate();
  }

  /**
   * Set airspace database to use
   * @param _airspace_database Pointer to the airspace database or NULL
   */
  void SetAirspaces(const Airspaces *airspace_database) {
    renderer.SetAirspaces(airspace_database);
  }

  /**
   * Set RasterTerrain to use
   * @param _terrain Pointer to the RasterTerrain or NULL
   */
  void SetTerrain(const RasterTerrain *terrain) {
    renderer.SetTerrain(terrain);
  }

  /**
   * Set CrossSection range
   * @param range Range to draw [m]
   */
  void SetRange(double range) {
    renderer.SetRange(range);
  }

  /**
   * Set CrossSection direction
   * @param bearing Direction to draw
   */
  void SetDirection(Angle bearing) {
    renderer.SetDirection(bearing);
  }

  /**
   * Set CrossSection start point
   * @param _start Start GeoPoint to use for drawing
   */
  void SetStart(GeoPoint start) {
    renderer.SetStart(start);
  }

  void SetInvalid() {
    renderer.SetInvalid();
  }

protected:
  /* virtual methods from AntiFlickerWindow */
  virtual void OnPaintBuffer(Canvas &canvas) noexcept override;
};
