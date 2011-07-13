/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Screen/BufferWindow.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Point.hpp"
#include "FullBlackboard.hpp"

struct VarioLook;
class ContainerWindow;
class UnitSymbol;

struct DrawInfo_t {
  bool    InitDone;
  PixelRect recBkg;
  RasterPoint orgText;
  fixed lastValue;
  TCHAR   lastText[32];
  const UnitSymbol *last_unit_symbol;
};

class GaugeVario : public BufferWindow
{
  enum {
    NARROWS = 3,
    YOFFSET = 36,

    /** 5 m/s */
    GAUGEVARIORANGE = 5,

    /** degrees total sweep */
    GAUGEVARIOSWEEP = 90,

    gmax = GAUGEVARIOSWEEP + 2,
  };

  const FullBlackboard &blackboard;

  const VarioLook &look;

public:
  bool ShowAvgText;
  bool ShowMc;
  bool ShowSpeedToFly;
  bool ShowBallast;
  bool ShowBugs;
  bool ShowGross;
  bool ShowAveNeedle;

private:
  int nlength0, nlength1, nwidth, nline;
  int xoffset;
  int yoffset;

  Pen sinkThickPen;
  Pen liftThickPen;
  Pen blankThickPen;

  Bitmap hBitmapClimb;

  bool dirty;

  bool layout_initialised;
  bool needle_initialised;
  bool ballast_initialised;
  bool bugs_initialised;
  RasterPoint orgTop;
  RasterPoint orgMiddle;
  RasterPoint orgBottom;

  Bitmap hDrawBitMap;
  DrawInfo_t diValueTop;
  DrawInfo_t diValueMiddle;
  DrawInfo_t diValueBottom;
  DrawInfo_t diLabelTop;
  DrawInfo_t diLabelMiddle;
  DrawInfo_t diLabelBottom;
  const UnitSymbol *unit_symbol;

  RasterPoint polys[(gmax * 2 + 1) * 3];
  RasterPoint lines[gmax * 2 + 1];

public:
  GaugeVario(const FullBlackboard &blackboard,
             ContainerWindow &parent, const VarioLook &look,
             int left, int top, unsigned width, unsigned height,
             const WindowStyle style=WindowStyle());

protected:
  const MoreData &Basic() const {
    return blackboard.Basic();
  }

  const DERIVED_INFO &Calculated() const {
    return blackboard.Calculated();
  }

  const SETTINGS_COMPUTER &SettingsComputer() const {
    return blackboard.SettingsComputer();
  }

protected:
  virtual bool on_resize(unsigned width, unsigned height);
  virtual void on_paint_buffer(Canvas &canvas);

private:
  void RenderZero(Canvas &canvas);
  void RenderValue(Canvas &canvas, int x, int y,
                   DrawInfo_t *diValue, DrawInfo_t *diLabel,
                   fixed Value, const TCHAR *Label);
  void RenderSpeedToFly(Canvas &canvas, int x, int y);
  void RenderBallast(Canvas &canvas);
  void RenderBugs(Canvas &canvas);
  int  ValueToNeedlePos(fixed Value);
  void RenderNeedle(Canvas &canvas, int i, bool average, bool clear);
  void RenderVarioLine(Canvas &canvas, int i, int sink, bool clear);
  void RenderClimb(Canvas &canvas);

  void MakePolygon(const int i);
  void MakeAllPolygons();
  RasterPoint *getPolygon(const int i);
};

#endif
