/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_TRAFFIC_CONTROL_HPP
#define XCSOAR_TRAFFIC_CONTROL_HPP

#include "FlarmTrafficWindow.hpp"
#include "UIUtil/GestureManager.hpp"

/**
 * A Window which renders FLARM traffic, with user interaction.
 */
class FlarmTrafficControl : public FlarmTrafficWindow {
protected:
  bool enable_auto_zoom;
  unsigned zoom;
  Angle task_direction;
  GestureManager gestures;

public:
  FlarmTrafficControl(const FlarmTrafficLook &look);

protected:
  void CalcAutoZoom();

public:
  void Update(Angle new_direction, const TrafficList &new_data,
              const TeamCodeSettings &new_settings);
  void UpdateTaskDirection(bool show_task_direction, Angle bearing);

  bool GetNorthUp() const {
    return enable_north_up;
  }

  void SetNorthUp(bool enabled);

  void ToggleNorthUp() {
    SetNorthUp(!GetNorthUp());
  }

  bool GetAutoZoom() const {
    return enable_auto_zoom;
  }

  static unsigned GetZoomDistance(unsigned zoom);

  void SetZoom(unsigned _zoom) {
    zoom = _zoom;
    SetDistance(fixed(GetZoomDistance(_zoom)));
  }

  void SetAutoZoom(bool enabled);

  void ToggleAutoZoom() {
    SetAutoZoom(!GetAutoZoom());
  }

  bool CanZoomOut() const {
    return zoom < 4;
  }

  bool CanZoomIn() const {
    return zoom > 0;
  }

  void ZoomOut();
  void ZoomIn();

  void SwitchData();
  void OpenDetails();

protected:
  void PaintTrafficInfo(Canvas &canvas) const;
  void PaintClimbRate(Canvas &canvas, PixelRect rc, fixed climb_rate) const;
  void PaintDistance(Canvas &canvas, PixelRect rc, fixed distance) const;
  void PaintRelativeAltitude(Canvas &canvas, PixelRect rc,
                             fixed relative_altitude) const;
  void PaintID(Canvas &canvas, PixelRect rc, const FlarmTraffic &traffic) const;
  void PaintTaskDirection(Canvas &canvas) const;

protected:
  bool OnMouseGesture(const TCHAR* gesture);

  /* virtual methods from class Window */
  virtual void OnCreate() override;
  virtual bool OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys) override;
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseDouble(PixelScalar x, PixelScalar y) override;
  virtual bool OnKeyDown(unsigned key_code) override;

  /* virtual methods from class PaintWindow */
  virtual void OnPaint(Canvas &canvas) override;
};

#endif
