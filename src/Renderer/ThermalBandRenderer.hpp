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

#ifndef THERMAL_BAND_RENDERER_HPP
#define THERMAL_BAND_RENDERER_HPP

struct PixelRect;
struct ThermalBandLook;
struct ChartLook;
class ChartRenderer;
class Canvas;
struct MoreData;
struct DerivedInfo;
struct ComputerSettings;
struct OrderedTaskSettings;
struct TaskBehaviour;
class ThermalBand;

class ThermalBandRenderer {
  const ThermalBandLook &look;
  const ChartLook &chart_look;

public:
  ThermalBandRenderer(const ThermalBandLook &_look,
                      const ChartLook &_chart_look)
    :look(_look), chart_look(_chart_look) {}

  void DrawThermalBand(const MoreData& basic,
                       const DerivedInfo& calculated,
                       const ComputerSettings &settings_computer,
                       Canvas &canvas,
                       const PixelRect &rc,
                       const TaskBehaviour& task_props,
                       const bool is_map,
                       const OrderedTaskSettings *ordered_props=nullptr) const;

  void DrawThermalBandSpark(const MoreData &basic,
                            const DerivedInfo& calculated,
                            const ComputerSettings &settings_computer,
                            Canvas &canvas,
                            const PixelRect &rc,
                            const TaskBehaviour& task_props) const;

protected:
  void _DrawThermalBand(const MoreData &basic,
                        const DerivedInfo& calculated,
                        const ComputerSettings &settings_computer,
                        ChartRenderer &chart,
                        const TaskBehaviour& task_props,
                        const bool is_infobox,
                        const bool is_map,
                        const OrderedTaskSettings* ordered_props) const;

  void ScaleChart(const DerivedInfo &calculated,
                  const ComputerSettings &settings_computer,
                  const TaskBehaviour& task_props,
                  ChartRenderer &chart,
                  const double hoffset) const;

  void DrawWorkingBand(const DerivedInfo& calculated,
                       ChartRenderer &chart,
                       const double hoffset) const;

  double GetHeightOffset(const DerivedInfo& calculated,
                         const TaskBehaviour& task_props) const;

  void DrawThermalProfile(const ThermalBand &thermal_band,
                          ChartRenderer &chart,
                          const double hoffset,
                          const bool alpha_shade,
                          const bool active) const;

  static void ScaleChartFromThermalBand(const ThermalBand &thermal_band,
                                        ChartRenderer &chart,
                                        const double hoffset);

  static void DrawRiskMC(const DerivedInfo& calculated,
                         const ComputerSettings &settings_computer,
                         ChartRenderer &chart,
                         const double hoffset,
                         const bool is_infobox,
                         const bool is_map);
};

#endif
