// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
