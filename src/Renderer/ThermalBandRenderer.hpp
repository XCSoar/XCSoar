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
  constexpr ThermalBandRenderer(const ThermalBandLook &_look,
                                const ChartLook &_chart_look) noexcept
    :look(_look), chart_look(_chart_look) {}

  void DrawThermalBand(const MoreData &basic,
                       const DerivedInfo &calculated,
                       const ComputerSettings &settings_computer,
                       Canvas &canvas,
                       const PixelRect &rc,
                       const TaskBehaviour &task_props,
                       const bool is_map,
                       const OrderedTaskSettings *ordered_props=nullptr) const noexcept;

  void DrawThermalBandSpark(const MoreData &basic,
                            const DerivedInfo &calculated,
                            const ComputerSettings &settings_computer,
                            Canvas &canvas,
                            const PixelRect &rc,
                            const TaskBehaviour &task_props) const noexcept;

protected:
  void _DrawThermalBand(const MoreData &basic,
                        const DerivedInfo &calculated,
                        const ComputerSettings &settings_computer,
                        ChartRenderer &chart,
                        const TaskBehaviour &task_props,
                        const bool is_infobox,
                        const bool is_map,
                        const OrderedTaskSettings* ordered_props) const noexcept;

  void ScaleChart(const DerivedInfo &calculated,
                  const ComputerSettings &settings_computer,
                  const TaskBehaviour &task_props,
                  ChartRenderer &chart,
                  const double hoffset) const noexcept;

  void DrawWorkingBand(const DerivedInfo &calculated,
                       ChartRenderer &chart,
                       const double hoffset) const noexcept;

  double GetHeightOffset(const DerivedInfo &calculated,
                         const TaskBehaviour &task_props) const noexcept;

  void DrawThermalProfile(const ThermalBand &thermal_band,
                          ChartRenderer &chart,
                          double hoffset,
                          bool alpha_shade,
                          bool active) const noexcept;

  static void ScaleChartFromThermalBand(const ThermalBand &thermal_band,
                                        ChartRenderer &chart,
                                        double hoffset) noexcept;

  static void DrawRiskMC(const DerivedInfo &calculated,
                         const ComputerSettings &settings_computer,
                         ChartRenderer &chart,
                         double hoffset,
                         bool is_infobox,
                         bool is_map) noexcept;
};
