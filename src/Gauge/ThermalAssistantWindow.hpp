/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef THERMAL_ASSISTENT_WINDOW_HPP
#define THERMAL_ASSISTENT_WINDOW_HPP

#include "Screen/BufferWindow.hpp"
#include "NMEA/Derived.hpp"

class ThermalAssistantWindow : public BufferWindow {
protected:
  static const Color hcBackground;
  static const Color hcCircles;
  static const Color hcStandard;
  static const Color hcPolygonBrush;
  static const Color hcPolygonPen;

  Brush hbRadar, hbPolygon;
  Pen hpPlane, hpRadar, hpPolygon;
  Pen hpInnerCircle;
  Pen hpOuterCircle;
  Font hfLabels, hfNoTraffic;

  /**
   * The distance of the biggest circle in meters.
   */
  fixed max_lift;

  POINT mid;

  /**
   * The minimum distance between the window boundary and the biggest
   * circle in pixels.
   */
  unsigned padding;

  /**
   * The radius of the biggest circle in pixels.
   */
  unsigned radius;

  bool small;

  Angle direction;
  DERIVED_INFO derived;
  POINT lift_points[37];
  POINT lift_point_avg;

public:
  ThermalAssistantWindow(unsigned _padding, bool _small = false);

public:
  bool LeftTurn() const;

  void Update(const Angle &_direction, const DERIVED_INFO &_derived);

protected:
  fixed RangeScale(fixed d) const;

  void UpdateLiftPoints();
  void UpdateLiftMax();
  void PaintRadarPlane(Canvas &canvas) const;
  void PaintRadarBackground(Canvas &canvas) const;
  void PaintPoints(Canvas &canvas) const;
  void PaintAdvisor(Canvas &canvas) const;
  void PaintNotCircling(Canvas &canvas) const;
  void Paint();

protected:
  virtual bool on_create();
  virtual bool on_resize(unsigned width, unsigned height);
};

#endif
