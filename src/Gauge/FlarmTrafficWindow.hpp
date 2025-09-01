// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "Renderer/RadarRenderer.hpp"
#include "FLARM/List.hpp"
#include "TeamCode/Settings.hpp"
#include "Math/FastRotation.hpp"
#include "Renderer/TextInBox.hpp"
#include "ui/canvas/Pen.hpp"

#include <cstdint>

class Color;
class Brush;
struct FlarmTrafficLook;

/**
 * A Window which renders FLARM traffic.
 */
class FlarmTrafficWindow : public PaintWindow {
protected:
  const FlarmTrafficLook &look;

  RadarRenderer radar_renderer;

  /**
   * The distance of the biggest circle in meters.
   */
  double distance = 2000;

  int selection = -1;
  int warning = -1;

  const bool small;

  PixelPoint sc[TrafficList::MAX_COUNT];

  bool enable_north_up = false;
  Angle heading = Angle::Zero();
  FastRotation fr;
  FastIntegerRotation fir;
  TrafficList data;
  TeamCodeSettings settings;

public:
  enum class SideInfoType : uint8_t {
    RELATIVE_ALTITUDE,
    VARIO,
  } side_display_type = SideInfoType::VARIO;

public:
  FlarmTrafficWindow(const FlarmTrafficLook &_look,
                     unsigned _h_padding, unsigned _v_padding,
                     bool _small = false) noexcept;

public:
  [[gnu::pure]]
  bool WarningMode() const noexcept;

  const FlarmTraffic *GetTarget() const noexcept {
    return selection >= 0
      ? &data.list[selection]
      : NULL;
  }

  void SetTarget(int i) noexcept;

  void SetTarget(const FlarmTraffic *traffic) noexcept {
    SetTarget(traffic != NULL ? (int)data.TrafficIndex(traffic) : -1);
  }

  void SetTarget(const FlarmId &id) noexcept {
    SetTarget(data.FindTraffic(id));
  }

  void NextTarget() noexcept;
  void PrevTarget() noexcept;
  bool SelectNearTarget(PixelPoint p, int max_distance) noexcept;

  void SetDistance(double _distance) noexcept {
    distance = _distance;
    Invalidate();
  }

  void Paint(Canvas &canvas) noexcept;

protected:
  [[gnu::pure]]
  double RangeScale(double d) const noexcept;

  void UpdateSelector(FlarmId id, PixelPoint pt) noexcept;
  void UpdateWarnings() noexcept;
  void Update(Angle new_direction, const TrafficList &new_data,
              const TeamCodeSettings &new_settings) noexcept;
  void PaintRadarNoTraffic(Canvas &canvas) const noexcept;
  void PaintRadarTarget(Canvas &canvas, const FlarmTraffic &traffic,
                        unsigned i) noexcept;
  void PaintRadarTraffic(Canvas &canvas) noexcept;

  void PaintTargetInfoSmall(Canvas &canvas, const FlarmTraffic &traffic,
                            unsigned i,
                            const Color &text_color,
                            const Brush &arrow_brush) noexcept;

  void PaintRadarPlane(Canvas &canvas) const noexcept;
  void PaintRadarBackground(Canvas &canvas) const noexcept;
  void PaintNorth(Canvas &canvas) const noexcept;

protected:
  /* virtual methods from class Window */
  void OnResize(PixelSize new_size) noexcept override;

  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;

private:
  /**
   * Renders a FLARM target that has no position data.
   * Draws an optional distance ring, a dot, and an exclamation mark.
   */
  void PaintNoPositionTarget(Canvas &canvas,
                           const PixelPoint &target_point,
                           const PixelPoint &radar_center,
                           double scale,
                           bool small,
                           const PixelSize &sx,
                           const Pen *target_pen,
                           const Color *text_color) const noexcept;
};
