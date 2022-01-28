/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef FLARM_TRAFFIC_WINDOW_H
#define FLARM_TRAFFIC_WINDOW_H

#include "ui/window/PaintWindow.hpp"
#include "FLARM/List.hpp"
#include "TeamCode/Settings.hpp"
#include "Math/FastRotation.hpp"

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

  /**
   * The distance of the biggest circle in meters.
   */
  double distance = 2000;

  int selection = -1;
  int warning = -1;
  PixelPoint radar_mid;

  /**
   * The minimum distance between the window boundary and the biggest
   * circle in pixels.
   */
  const unsigned h_padding, v_padding;

  /**
   * The radius of the biggest circle in pixels.
   */
  unsigned radius;

  const bool small;

  PixelPoint sc[TrafficList::MAX_COUNT];

  bool enable_north_up = false;
  Angle heading = Angle::Zero();
  FastRotation fr;
  FastIntegerRotation fir;
  TrafficList data;
  Validity data_modified;
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
  virtual void OnResize(PixelSize new_size) override;

  /* virtual methods from class PaintWindow */
  virtual void OnPaint(Canvas &canvas) override;
};

#endif
