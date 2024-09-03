// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThermalBandRenderer.hpp"
#include "ChartRenderer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Computer/Settings.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/ThermalBandLook.hpp"
#include "Units/Units.hpp"
#include "Screen/Layout.hpp"
#include "Math/LeastSquares.hpp"
#include "Math/Point2D.hpp"
#include "util/StaticArray.hxx"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

#include <algorithm>

void
ThermalBandRenderer::ScaleChart(const DerivedInfo &calculated,
                                const ComputerSettings &settings_computer,
                                const TaskBehaviour &task_props,
                                ChartRenderer &chart,
                                const double hoffset) const noexcept
{
  chart.ScaleYFromValue(task_props.route_planner.safety_height_terrain);
  chart.ScaleYFromValue(calculated.common_stats.height_max_working-hoffset);

  chart.ScaleXFromValue(0);
  chart.ScaleXFromValue(0.5);
  chart.ScaleXFromValue(settings_computer.polar.glide_polar_task.GetMC());
}

void
ThermalBandRenderer::ScaleChartFromThermalBand(const ThermalBand &thermal_band,
                                               ChartRenderer &chart,
                                               const double hoffset) noexcept
{
  if (thermal_band.Valid()) {
    chart.ScaleXFromValue(thermal_band.GetMaxW());
    chart.ScaleYFromValue(thermal_band.GetCeiling()-hoffset);
  }
}


void
ThermalBandRenderer::DrawThermalProfile(const ThermalBand &thermal_band,
                                        ChartRenderer &chart,
                                        const double hoffset,
                                        const bool is_infobox,
                                        const bool active) const noexcept
{
  if (!thermal_band.Valid())
    return;

  const unsigned renderable = thermal_band.size();
  // position of thermal band

  StaticArray<DoublePoint2D, ThermalBand::max_size()> thermal_profile;

  // add elements, with smoothing filter
  for (unsigned i = 0; i < renderable; ++i) {
    const double k_smooth = 0.5;
    double k = thermal_band.GetSlice(i).n;
    double w = thermal_band.GetSlice(i).w_n * k;
    if (k) {
      if (thermal_band.Occupied(i-1)) {
        double k_this = k_smooth*thermal_band.GetSlice(i-1).n;
        w += thermal_band.GetSlice(i-1).w_n * k_this;
        k += k_this;
      }
      if (thermal_band.Occupied(i+1)) {
        double k_this = k_smooth*thermal_band.GetSlice(i+1).n;
        w += thermal_band.GetSlice(i+1).w_n * k_this;
        k += k_this;
      }
      w /= k;
    }
    thermal_profile.emplace_back(w, thermal_band.GetSliceCenter(i)-hoffset);
  }

  const Pen *pen = is_infobox ? nullptr : (active? &look.pen_active : &look.pen_inactive);
  const Brush &brush = active? look.brush_active : look.brush_inactive;

  if (!is_infobox) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    chart.DrawFilledY({thermal_profile.data(), thermal_profile.size()},
                      brush, pen);
  } else {
    chart.DrawFilledY({thermal_profile.data(), thermal_profile.size()},
                      brush, pen);
  }
}


void
ThermalBandRenderer::DrawRiskMC(const DerivedInfo &calculated,
                                const ComputerSettings &settings_computer,
                                ChartRenderer &chart,
                                const double hoffset,
                                const bool is_infobox,
                                const bool is_map) noexcept
{
  if (settings_computer.polar.glide_polar_task.GetMC()<= 0)
    return;

  LeastSquares tmp;
  tmp.Reset();
  double h_m = 0;
  double rmc = 0;
  const double dh = (chart.GetYMax()-chart.GetYMin())/32;
  if (dh <= 0)
    return;

  for (double h= chart.GetYMin(); h<= chart.GetYMax(); h += dh) {
    const double f = calculated.CalculateWorkingFraction(h+hoffset, settings_computer.task.safety_height_arrival);
    const double risk_mc =
        settings_computer.polar.glide_polar_task.GetRiskMC(f, settings_computer.task.risk_gamma);
    if ((h_m<=0) && (risk_mc > 0.3*settings_computer.polar.glide_polar_task.GetMC())) {
      h_m = h;
      rmc = risk_mc;
    }
    tmp.Update(risk_mc, h);
  }
  if (tmp.IsEmpty())
    return;

  chart.DrawLineGraph(tmp, (is_map || is_infobox)? ChartLook::STYLE_WHITE: ChartLook::STYLE_REDTHICKDASH);
  if (!is_map && !is_infobox) {
    chart.DrawLabel({rmc, h_m}, _T("MC"));
  }
}


void
ThermalBandRenderer::_DrawThermalBand(const MoreData &basic,
                                      const DerivedInfo &calculated,
                                      const ComputerSettings &settings_computer,
                                      ChartRenderer &chart,
                                      const TaskBehaviour &task_props,
                                      const bool is_infobox,
                                      const bool is_map,
                                      const OrderedTaskSettings *ordered_props) const noexcept
{
  // all heights here are relative to ground
  const auto hoffset = calculated.GetTerrainBaseFallback();

  ScaleChart(calculated, settings_computer, task_props, chart, hoffset);

  double h = 0;
  if (basic.NavAltitudeAvailable()) {
    h = basic.nav_altitude - hoffset;
    chart.ScaleYFromValue(h);
  }

  bool draw_start_height = false;

  draw_start_height = ordered_props
    && calculated.ordered_task_stats.task_valid
    && ordered_props->start_constraints.max_height != 0
    && calculated.terrain_valid;
  if (draw_start_height) {
    double hstart = ordered_props->start_constraints.max_height;
    if (ordered_props->start_constraints.max_height_ref == AltitudeReference::AGL &&
        calculated.terrain_valid)
      hstart += calculated.terrain_altitude;

    chart.ScaleYFromValue(hstart-hoffset);
  }

  // draw thermal profiles
  ScaleChartFromThermalBand(calculated.thermal_encounter_collection,
                            chart, hoffset);
  ScaleChartFromThermalBand(calculated.thermal_encounter_band,
                            chart, hoffset);

  const bool active = calculated.thermal_encounter_band.Valid();
  DrawThermalProfile(calculated.thermal_encounter_collection,
                     chart, hoffset, is_infobox, !active);
  if (active) {
    DrawThermalProfile(calculated.thermal_encounter_band,
                       chart, hoffset, is_infobox, true);
  }

  // risk mc
  if (h>settings_computer.task.safety_height_arrival) {
    DrawRiskMC(calculated, settings_computer, chart, hoffset, is_infobox, is_map);
  }

  // draw working band lines
  DrawWorkingBand(calculated, chart, hoffset);

  // position of MC
  if (basic.NavAltitudeAvailable()) {
    const Pen &pen = is_infobox && look.inverse
      ? look.white_pen : look.black_pen;
    chart.DrawLine({0, h},
                   {settings_computer.polar.glide_polar_task.GetMC(), h},
                   pen);

    if (is_infobox && look.inverse)
      chart.GetCanvas().SelectWhiteBrush();
    else
      chart.GetCanvas().SelectBlackBrush();
    chart.DrawDot({settings_computer.polar.glide_polar_task.GetMC(), h},
                  Layout::Scale(2));
  }
}

void
ThermalBandRenderer::DrawThermalBand(const MoreData &basic,
                                     const DerivedInfo &calculated,
                                     const ComputerSettings &settings_computer,
                                     Canvas &canvas,
                                     const PixelRect &rc,
                                     const TaskBehaviour &task_props,
                                     const bool is_map,
                                     const OrderedTaskSettings *ordered_props) const noexcept
{
  ChartRenderer chart(chart_look, canvas, rc, !is_map);
  if (!is_map) {
    chart.SetXLabel(_T("w"), Units::GetVerticalSpeedName());
    chart.SetYLabel(_T("h AGL"), Units::GetAltitudeName());
  }

  chart.Begin();

  if (!is_map && (calculated.common_stats.height_max_working <= 0)) {
    // no climbs recorded
    chart.DrawNoData();
    chart.Finish();
    return;
  }
  _DrawThermalBand(basic, calculated, settings_computer,
                   chart, task_props, false, is_map, ordered_props);

  if (!is_map) {
    chart.DrawXGrid(Units::ToSysVSpeed(1), 1, ChartRenderer::UnitFormat::NUMERIC);
    chart.DrawYGrid(Units::ToSysAltitude(250), 250, ChartRenderer::UnitFormat::NUMERIC);
  }

  chart.Finish();
}

void
ThermalBandRenderer::DrawThermalBandSpark(const MoreData &basic,
                                          const DerivedInfo &calculated,
                                          const ComputerSettings &settings_computer,
                                          Canvas &canvas,
                                          const PixelRect &rc,
                                          const TaskBehaviour &task_props) const noexcept
{
  ChartRenderer chart(chart_look, canvas, rc, false);
  chart.Begin();

  _DrawThermalBand(basic, calculated, settings_computer,
                   chart, task_props, true, false, nullptr);

  chart.Finish();
}

void
ThermalBandRenderer::DrawWorkingBand(const DerivedInfo &calculated_info,
                                     ChartRenderer &chart,
                                     const double hoffset) const noexcept
{
  const auto h_max = calculated_info.common_stats.height_max_working-hoffset;
  if ((h_max> chart.GetYMin()) && (h_max< chart.GetYMax())) {
    chart.DrawLine({0, h_max}, {chart.GetXMax(), h_max}, look.working_band_pen);
  }
  const auto h_min = calculated_info.common_stats.height_min_working-hoffset;
  if ((h_min> chart.GetYMin()) && (h_min< chart.GetYMax())) {
    chart.DrawLine({0, h_min}, {chart.GetXMax(), h_min}, look.working_band_pen);
  }
  chart.DrawLine({0, chart.GetYMin()}, {chart.GetXMax()*0.5, chart.GetYMin()}, look.working_band_pen);
}
