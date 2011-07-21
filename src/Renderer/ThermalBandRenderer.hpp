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

#ifndef THERMAL_BAND_RENDERER_HPP
#define THERMAL_BAND_RENDERER_HPP

#include "Screen/Point.hpp"
#include "Math/fixed.hpp"

struct ThermalBandLook;
struct ChartLook;
class Chart;
class Canvas;
struct MoreData;
struct DERIVED_INFO;
struct SETTINGS_COMPUTER;
class OrderedTaskBehaviour;
class TaskBehaviour;

class ThermalBandRenderer {
  const ThermalBandLook &look;
  const ChartLook &chart_look;

public:
  ThermalBandRenderer(const ThermalBandLook &_look,
                      const ChartLook &_chart_look)
    :look(_look), chart_look(_chart_look) {}

  void DrawThermalBand(const MoreData& basic,
                       const DERIVED_INFO& calculated,
                       const SETTINGS_COMPUTER &settings_computer,
                       Canvas &canvas,
                       const PixelRect &rc,
                       const TaskBehaviour& task_props,
                       const bool is_map,
                       const OrderedTaskBehaviour* ordered_props=NULL) const;

  void DrawThermalBandSpark(const MoreData &basic,
                            const DERIVED_INFO& calculated,
                            const SETTINGS_COMPUTER &settings_computer,
                            Canvas &canvas,
                            const PixelRect &rc,
                            const TaskBehaviour& task_props) const;

protected:
  void _DrawThermalBand(const MoreData &basic,
                        const DERIVED_INFO& calculated,
                        const SETTINGS_COMPUTER &settings_computer,
                        Chart &chart,
                        const TaskBehaviour& task_props,
                        const bool is_infobox,
                        const OrderedTaskBehaviour* ordered_props) const;

  void scale_chart(const DERIVED_INFO &calculated,
                   const SETTINGS_COMPUTER &settings_computer,
                   Chart &chart) const;
};

#endif
