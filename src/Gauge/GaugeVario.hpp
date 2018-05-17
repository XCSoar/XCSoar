/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef GAUGE_VARIO_H
#define GAUGE_VARIO_H

#include "Screen/AntiFlickerWindow.hpp"
#include "Screen/BulkPoint.hpp"
#include "Blackboard/FullBlackboard.hpp"
#include "Math/Point2D.hpp"

struct VarioLook;
class ContainerWindow;

class GaugeVario : public AntiFlickerWindow
{
  static constexpr unsigned NARROWS = 3;
  static constexpr int YOFFSET = 36;

  /** 5 m/s */
  static constexpr int GAUGEVARIORANGE = 5;

  /** degrees total sweep */
  static constexpr int GAUGEVARIOSWEEP = 90;

  static constexpr int gmax = GAUGEVARIOSWEEP + 2;
  static constexpr int gmin = -gmax;

  struct DrawInfo {
    bool initialised;
    PixelRect rc;
    PixelPoint text_position;
    double last_value;
    TCHAR last_text[32];
    Unit last_unit;
  };

  const FullBlackboard &blackboard;

  const VarioLook &look;

private:
  const unsigned nlength0, nlength1, nwidth, nline;

  IntPoint2D offset;

  bool dirty;

  bool layout_initialised;
  bool needle_initialised;
  bool ballast_initialised;
  bool bugs_initialised;

  PixelPoint top_position;
  PixelPoint middle_position;
  PixelPoint bottom_position;

  DrawInfo value_top;
  DrawInfo value_middle;
  DrawInfo value_bottom;
  DrawInfo label_top;
  DrawInfo label_middle;
  DrawInfo label_bottom;

  BulkPixelPoint polys[(gmax * 2 + 1) * 3];
  BulkPixelPoint lines[gmax * 2 + 1];

public:
  GaugeVario(const FullBlackboard &blackboard,
             ContainerWindow &parent, const VarioLook &look,
             PixelRect rc, const WindowStyle style=WindowStyle());

protected:
  const MoreData &Basic() const {
    return blackboard.Basic();
  }

  const DerivedInfo &Calculated() const {
    return blackboard.Calculated();
  }

  const ComputerSettings &GetComputerSettings() const {
    return blackboard.GetComputerSettings();
  }

  const GlidePolar &GetGlidePolar() const {
    return GetComputerSettings().polar.glide_polar_task;
  }

  const VarioSettings &Settings() const {
    return blackboard.GetUISettings().vario;
  }

protected:
  /* virtual methods from class Window */
  virtual void OnResize(PixelSize new_size) override;

  /* virtual methods from class AntiFlickerWindow */
  virtual void OnPaintBuffer(Canvas &canvas) override;

private:
  void RenderZero(Canvas &canvas);
  void RenderValue(Canvas &canvas, int x, int y,
                   DrawInfo *diValue, DrawInfo *diLabel,
                   double Value, const TCHAR *Label);
  void RenderSpeedToFly(Canvas &canvas, int x, int y);
  void RenderBallast(Canvas &canvas);
  void RenderBugs(Canvas &canvas);
  int  ValueToNeedlePos(double Value);
  void RenderNeedle(Canvas &canvas, int i, bool average, bool clear);
  void RenderVarioLine(Canvas &canvas, int i, int sink, bool clear);
  void RenderClimb(Canvas &canvas);

  void MakePolygon(const int i);
  void MakeAllPolygons();
  BulkPixelPoint *getPolygon(const int i);
};

#endif
