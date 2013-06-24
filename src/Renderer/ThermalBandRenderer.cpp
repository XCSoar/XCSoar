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

#include "ThermalBandRenderer.hpp"
#include "ChartRenderer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Computer/Settings.hpp"
#include "Screen/Canvas.hpp"
#include "Look/ThermalBandLook.hpp"
#include <algorithm>
#include "Engine/Task/TaskBehaviour.hpp"
#include "Units/Units.hpp"
#include "Screen/Layout.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

void
ThermalBandRenderer::ScaleChart(const DerivedInfo &calculated,
                                 const ComputerSettings &settings_computer,
                                 ChartRenderer &chart) const
{
  chart.ScaleYFromValue(fixed(0));
  chart.ScaleYFromValue(calculated.thermal_band.max_thermal_height);

  chart.ScaleXFromValue(fixed(0));
  chart.ScaleXFromValue(fixed(0.5));
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
  fixed hoffset = task_props.route_planner.safety_height_terrain +
    calculated.GetTerrainBaseFallback();

  fixed h = fixed(0);
  if (basic.NavAltitudeAvailable()) {
    h = basic.nav_altitude - hoffset;
    chart.ScaleYFromValue(h);
  }

  bool draw_start_height = false;
  fixed hstart = fixed(0);

  draw_start_height = ordered_props
    && calculated.ordered_task_stats.task_valid
    && ordered_props->start_constraints.max_height != 0
    && calculated.terrain_valid;
  if (draw_start_height) {
    hstart = fixed(ordered_props->start_constraints.max_height);
    if (ordered_props->start_constraints.max_height_ref == AltitudeReference::AGL &&
        calculated.terrain_valid)
      hstart += calculated.terrain_altitude;

    hstart -= hoffset;
    chart.ScaleYFromValue(hstart);
  }

  // no thermalling has been done above safety height
  if (!positive(calculated.thermal_band.max_thermal_height))
    return;

  // calculate averages
  int numtherm = 0;

  fixed Wmax = fixed(0);
  fixed Wav = fixed(0);
  fixed Wt[ThermalBandInfo::NUMTHERMALBUCKETS];
  fixed ht[ThermalBandInfo::NUMTHERMALBUCKETS];

  for (unsigned i = 0; i < ThermalBandInfo::NUMTHERMALBUCKETS; ++i) {
    if (thermal_band.thermal_profile_n[i] < 6) 
      continue;

    if (positive(thermal_band.thermal_profile_w[i])) {
      // height of this thermal point [0,mth]
      // requires 5 items in bucket before displaying, to eliminate kinks
      fixed wthis = thermal_band.thermal_profile_w[i] / thermal_band.thermal_profile_n[i];
      ht[numtherm] = i * calculated.thermal_band.max_thermal_height 
        / ThermalBandInfo::NUMTHERMALBUCKETS;
      Wt[numtherm] = wthis;
      Wmax = std::max(Wmax, wthis);
      Wav+= wthis;
      numtherm++;
    }
  }
  chart.ScaleXFromValue(Wmax);
  if (!numtherm)
    return;
  chart.ScaleXFromValue(fixed(1.5)*Wav/numtherm);

  if ((!draw_start_height) && (numtherm<=1))
    // don't display if insufficient statistics
    // but do draw if start height needs to be drawn
    return;

  const Pen *fpen = is_infobox ? NULL : &look.pen;

  // position of thermal band
  if (numtherm > 1) {
    std::vector< std::pair<fixed, fixed> > thermal_profile;
    thermal_profile.reserve(numtherm);
    for (int i = 0; i < numtherm; ++i)
      thermal_profile.emplace_back(Wt[i], ht[i]);

    if (!is_infobox) {
#ifdef ENABLE_OPENGL
      const GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
      chart.DrawFilledY(thermal_profile, look.brush, fpen);
    } else
      chart.DrawFilledY(thermal_profile, look.brush, fpen);
  }

  // position of thermal band
  if (basic.NavAltitudeAvailable()) {
    const Pen &pen = is_infobox && look.inverse
      ? look.white_pen : look.black_pen;
    chart.DrawLine(fixed(0), h,
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
  ChartRenderer chart(chart_look, canvas, rc);
  if (is_map) {
    chart.padding_bottom = 0;
    chart.padding_left = 0;
  }
  ScaleChart(calculated, settings_computer, chart);
  _DrawThermalBand(basic, calculated, settings_computer,
                   chart, task_props, false, ordered_props);

  if (!is_map) {
    chart.DrawXGrid(Units::ToSysVSpeed(fixed(1)),
                    ChartLook::STYLE_THINDASHPAPER, fixed(1), true);
    chart.DrawYGrid(Units::ToSysAltitude(fixed(1000)),
                    ChartLook::STYLE_THINDASHPAPER,
                    fixed(1000), true);
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
  ChartRenderer chart(chart_look, canvas, rc);
  chart.padding_bottom = 0;
  chart.padding_left = Layout::Scale(3);
  ScaleChart(calculated, settings_computer, chart);
  _DrawThermalBand(basic, calculated, settings_computer,
                   chart, task_props, true, NULL);
}
