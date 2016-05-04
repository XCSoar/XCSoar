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

#include "ThermalBandRenderer.hpp"
#include "ChartRenderer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Computer/Settings.hpp"
#include "Screen/Canvas.hpp"
#include "Look/ThermalBandLook.hpp"
#include "Units/Units.hpp"
#include "Screen/Layout.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

#include <algorithm>

void
ThermalBandRenderer::ScaleChart(const DerivedInfo &calculated,
                                 const ComputerSettings &settings_computer,
                                 const TaskBehaviour& task_props,
                                 ChartRenderer &chart) const
{
  chart.ScaleYFromValue(task_props.route_planner.safety_height_terrain);
  chart.ScaleYFromValue(calculated.thermal_band.max_thermal_height);

  chart.ScaleXFromValue(0);
  chart.ScaleXFromValue(0.5);
  chart.ScaleXFromValue(settings_computer.polar.glide_polar_task.GetMC());
}


void
ThermalBandRenderer::_DrawThermalBand(const MoreData &basic,
                                      const DerivedInfo& calculated,
                                      const ComputerSettings &settings_computer,
                                      ChartRenderer &chart,
                                      const TaskBehaviour& task_props,
                                      const bool is_infobox,
                                      const OrderedTaskSettings *ordered_props) const
{
  const ThermalBandInfo &thermal_band = calculated.thermal_band;

  // calculate height above safety height
  const auto hoffset = calculated.GetTerrainBaseFallback();
  const auto hsafety = task_props.route_planner.safety_height_terrain;

  double h = 0;
  if (basic.NavAltitudeAvailable()) {
    h = basic.nav_altitude - hoffset;
    chart.ScaleYFromValue(h);
  }

  bool draw_start_height = false;
  double hstart = 0;

  draw_start_height = ordered_props
    && calculated.ordered_task_stats.task_valid
    && ordered_props->start_constraints.max_height != 0
    && calculated.terrain_valid;
  if (draw_start_height) {
    hstart = ordered_props->start_constraints.max_height;
    if (ordered_props->start_constraints.max_height_ref == AltitudeReference::AGL &&
        calculated.terrain_valid)
      hstart += calculated.terrain_altitude;

    hstart -= hoffset;
    chart.ScaleYFromValue(hstart);
  }

  // no thermalling has been done above safety height
  if (thermal_band.max_thermal_height <= 0)
    return;

  // calculate averages
  int numtherm = 0;

  double Wmax = 0;
  double Wav = 0;
  double Wt[ThermalBandInfo::N_BUCKETS];
  double ht[ThermalBandInfo::N_BUCKETS];

  for (unsigned i = 0; i < ThermalBandInfo::N_BUCKETS; ++i) {
    if (thermal_band.thermal_profile_n[i] < 6) 
      continue;

    if (thermal_band.thermal_profile_w[i] > 0) {
      // height of this thermal point [0,mth]
      // requires 5 items in bucket before displaying, to eliminate kinks
      auto wthis = thermal_band.thermal_profile_w[i] / thermal_band.thermal_profile_n[i];
      ht[numtherm] = i * thermal_band.max_thermal_height
          / ThermalBandInfo::N_BUCKETS + hsafety;
      Wt[numtherm] = wthis;
      Wmax = std::max(Wmax, wthis);
      Wav+= wthis;
      numtherm++;
    }
  }
  chart.ScaleXFromValue(Wmax);
  if (!numtherm)
    return;
  chart.ScaleXFromValue(1.5 * Wav / numtherm);

  if ((!draw_start_height) && (numtherm<=1))
    // don't display if insufficient statistics
    // but do draw if start height needs to be drawn
    return;

  const Pen *fpen = is_infobox ? nullptr : &look.pen;

  // position of thermal band
  if (numtherm > 1) {
    std::vector<std::pair<double, double>> thermal_profile;
    thermal_profile.reserve(numtherm);
    for (int i = 0; i < numtherm; ++i)
      thermal_profile.emplace_back(Wt[i], ht[i]);

    if (!is_infobox) {
#ifdef ENABLE_OPENGL
      const ScopeAlphaBlend alpha_blend;
#endif
      chart.DrawFilledY(thermal_profile, look.brush, fpen);
    } else
      chart.DrawFilledY(thermal_profile, look.brush, fpen);
  }

  DrawWorkingBand(calculated, chart, hoffset);

  // position of MC
  if (basic.NavAltitudeAvailable()) {
    const Pen &pen = is_infobox && look.inverse
      ? look.white_pen : look.black_pen;
    chart.DrawLine(0, h,
                   settings_computer.polar.glide_polar_task.GetMC(), h, pen);

    if (is_infobox && look.inverse)
      chart.GetCanvas().SelectWhiteBrush();
    else
      chart.GetCanvas().SelectBlackBrush();
    chart.DrawDot(settings_computer.polar.glide_polar_task.GetMC(),
                  h, Layout::Scale(2));
  }
}

void
ThermalBandRenderer::DrawThermalBand(const MoreData &basic,
                                     const DerivedInfo& calculated,
                                     const ComputerSettings &settings_computer,
                                     Canvas &canvas,
                                     const PixelRect &rc,
                                     const TaskBehaviour& task_props,
                                     const bool is_map,
                                     const OrderedTaskSettings *ordered_props) const
{
  ChartRenderer chart(chart_look, canvas, rc, !is_map);

  if (!is_map && calculated.thermal_band.max_thermal_height <= 0) {
    // no climbs below safety height
    chart.DrawNoData();
    return;
  }
  ScaleChart(calculated, settings_computer, task_props, chart);
  _DrawThermalBand(basic, calculated, settings_computer,
                   chart, task_props, false, ordered_props);

  if (!is_map) {
    chart.DrawXGrid(Units::ToSysVSpeed(1), 1, ChartRenderer::UnitFormat::NUMERIC);
    chart.DrawYGrid(Units::ToSysAltitude(1000), 1000, ChartRenderer::UnitFormat::NUMERIC);
    chart.DrawXLabel(_T("w"), Units::GetVerticalSpeedName());
    chart.DrawYLabel(_T("h AGL"), Units::GetAltitudeName());
  }
}

void
ThermalBandRenderer::DrawThermalBandSpark(const MoreData &basic,
                                          const DerivedInfo& calculated,
                                          const ComputerSettings &settings_computer,
                                          Canvas &canvas,
                                          const PixelRect &rc,
                                          const TaskBehaviour &task_props) const
{
  ChartRenderer chart(chart_look, canvas, rc, false);
  ScaleChart(calculated, settings_computer, task_props, chart);
  _DrawThermalBand(basic, calculated, settings_computer,
                   chart, task_props, true, nullptr);
}

void
ThermalBandRenderer::DrawWorkingBand(const DerivedInfo& calculated_info,
                                     ChartRenderer &chart,
                                     const double hoffset) const
{
  const auto h_max = calculated_info.common_stats.height_max_working-hoffset;
  if ((h_max> chart.GetYMin()) && (h_max< chart.GetYMax())) {
    chart.DrawLine(0, h_max, chart.GetXMax(), h_max, look.working_band_pen);
  }
  const auto h_min = calculated_info.common_stats.height_min_working-hoffset;
  if ((h_min> chart.GetYMin()) && (h_min< chart.GetYMax())) {
    chart.DrawLine(0, h_min, chart.GetXMax(), h_min, look.working_band_pen);
  }
  chart.DrawLine(0, chart.GetYMin(), chart.GetXMax()*0.5, chart.GetYMin(), look.working_band_pen);
}

