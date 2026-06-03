// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/BufferWindow.hpp"
#include "ForwardViewRenderer.hpp"

struct CrossSectionLook;
struct AirspaceLook;
struct TopographyLook;
struct MoreData;
struct DerivedInfo;
struct MapSettings;
class Airspaces;
class RasterTerrain;
class TopographyStore;

class ForwardViewWindow : public BufferWindow {
  ForwardViewRenderer renderer;

public:
  ForwardViewWindow(const CrossSectionLook &look,
                    const AirspaceLook &airspace_look,
                    const TopographyLook &topography_look,
                    bool inverse) noexcept;

  void ReadBlackboard(const MoreData &basic,
                      const DerivedInfo &calculated,
                      const GlideSettings &glide_settings,
                      const GlidePolar &glide_polar,
                      const MapSettings &map_settings,
                      RoughTimeDelta utc_offset) noexcept;

  void SetAirspaces(const Airspaces *airspaces) noexcept {
    renderer.SetAirspaces(airspaces);
  }

  void SetTerrain(const RasterTerrain *terrain) noexcept {
    renderer.SetTerrain(terrain);
  }

  void SetTopography(TopographyStore *topography) noexcept {
    renderer.SetTopography(topography);
  }

  void SetView(GeoPoint start, Angle track, double range) noexcept {
    renderer.SetView(start, track, range);
  }

  void SetMotionTarget(GeoPoint fix, Angle track, double ground_speed,
                       double range) noexcept {
    renderer.SetMotionTarget(fix, track, ground_speed, range);
  }

  void TickMotion() noexcept {
    renderer.TickMotion();
  }

  bool TickMotionForRepaint() noexcept {
    return renderer.TickMotionForRepaint();
  }

  void SetInvalid() noexcept {
    renderer.SetInvalid();
  }

  void SetWireframe(bool wireframe) noexcept {
    renderer.SetWireframe(wireframe);
  }

protected:
  void OnPaintBuffer(Canvas &canvas) noexcept override;
};
