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

#include "ThermalBandRenderer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "SettingsComputer.hpp"
#include "Screen/Chart.hpp"
#include "Screen/Canvas.hpp"
#include "Look/ThermalBandLook.hpp"
#include "Appearance.hpp"
#include <algorithm>
#include "Engine/Task/TaskBehaviour.hpp"
#include "Units/Units.hpp"
#include "Screen/Layout.hpp"

void
ThermalBandRenderer::scale_chart(const DERIVED_INFO &calculated,
                                 const SETTINGS_COMPUTER &settings_computer,
                                 Chart &chart) const
{
  chart.ScaleYFromValue(fixed_zero);
  chart.ScaleYFromValue(calculated.thermal_band.MaxThermalHeight);

  chart.ScaleXFromValue(fixed_zero);
  chart.ScaleXFromValue(fixed_half);
  chart.ScaleXFromValue(settings_computer.glide_polar_task.GetMC());
}


void
ThermalBandRenderer::_DrawThermalBand(const MoreData &basic,
                                      const DERIVED_INFO& calculated,
                                      const SETTINGS_COMPUTER &settings_computer,
                                      Chart &chart,
                                      const TaskBehaviour& task_props,
                                      const bool is_infobox,
                                      const OrderedTaskBehaviour *ordered_props) const
{
  const ThermalBandInfo &thermal_band = calculated.thermal_band;

  // calculate height above safety height
  fixed hoffset = task_props.route_planner.safety_height_terrain +
                   calculated.TerrainBase;
  fixed h = basic.NavAltitude - hoffset;
  chart.ScaleYFromValue(h);

  bool draw_start_height = false;
  fixed hstart = fixed_zero;

  draw_start_height = ordered_props
    && calculated.common_stats.ordered_valid
    && (ordered_props->start_max_height != 0)
    && calculated.TerrainValid;
  if (draw_start_height) {
    if (ordered_props->start_max_height_ref == 0) {
      hstart = fixed(ordered_props->start_max_height) + calculated.TerrainAlt;
    } else {
      hstart = fixed(ordered_props->start_max_height);
    }
    hstart -= hoffset;
    chart.ScaleYFromValue(hstart);
  }

  // no thermalling has been done above safety height
  if (!positive(calculated.thermal_band.MaxThermalHeight))
    return;

  // calculate averages
  int numtherm = 0;

  fixed Wmax = fixed_zero;
  fixed Wav = fixed_zero;
  fixed Wt[ThermalBandInfo::NUMTHERMALBUCKETS];
  fixed ht[ThermalBandInfo::NUMTHERMALBUCKETS];

  for (unsigned i = 0; i < ThermalBandInfo::NUMTHERMALBUCKETS; ++i) {
    if (thermal_band.ThermalProfileN[i] < 6) 
      continue;

    if (positive(thermal_band.ThermalProfileW[i])) {
      // height of this thermal point [0,mth]
      // requires 5 items in bucket before displaying, to eliminate kinks
      fixed wthis = thermal_band.ThermalProfileW[i] / thermal_band.ThermalProfileN[i];
      ht[numtherm] = i * calculated.thermal_band.MaxThermalHeight 
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
    std::vector< std::pair<fixed, fixed> > ThermalProfile; 
    for (int i = 0; i < numtherm; ++i) {
      ThermalProfile.push_back(std::make_pair(Wt[i], ht[i]));
    }
    chart.DrawFilledY(ThermalProfile, look.brush, fpen);
  }

  // position of thermal band
  const Pen &pen = is_infobox && Appearance.InverseInfoBox
    ? look.white_pen : look.black_pen;
  chart.DrawLine(fixed_zero, h,
                 settings_computer.glide_polar_task.GetMC(), h, pen);

  if (is_infobox && Appearance.InverseInfoBox)
    chart.get_canvas().white_brush();
  else
    chart.get_canvas().black_brush();
  chart.DrawDot(settings_computer.glide_polar_task.GetMC(), h, IBLSCALE(2));

  /*
  RasterPoint GliderBand[5] = { { 0, 0 }, { 23, 0 }, { 22, 0 }, { 24, 0 }, { 0, 0 } };
  GliderBand[0].y = IBLSCALE(4) + iround(TBSCALEY * (fixed_one - hglider)) + rc.top;
  GliderBand[1].y = GliderBand[0].y;
  GliderBand[1].x =
      max(iround((mc / Wmax) * IBLSCALE(TBSCALEX)), IBLSCALE(4)) + rc.left;

  GliderBand[2].x = GliderBand[1].x - IBLSCALE(4);
  GliderBand[2].y = GliderBand[0].y - IBLSCALE(4);
  GliderBand[3].x = GliderBand[1].x;
  GliderBand[3].y = GliderBand[1].y;
  GliderBand[4].x = GliderBand[1].x - IBLSCALE(4);
  GliderBand[4].y = GliderBand[0].y + IBLSCALE(4);

  canvas.select(look.pen);

  canvas.polyline(GliderBand, 2);
  canvas.polyline(GliderBand + 2, 3); // arrow head

  if (draw_start_height) {
    canvas.select(Graphics::hpFinalGlideBelow);
    GliderBand[0].y = IBLSCALE(4) + iround(TBSCALEY * (fixed_one - hstart)) + rc.top;
    GliderBand[1].y = GliderBand[0].y;
    canvas.polyline(GliderBand, 2);
  }
  */
}

void 
ThermalBandRenderer::DrawThermalBand(const MoreData &basic,
                                     const DERIVED_INFO& calculated,
                                     const SETTINGS_COMPUTER &settings_computer,
                                     Canvas &canvas, 
                                     const PixelRect &rc,
                                     const TaskBehaviour& task_props,
                                     const bool is_map,
                                     const OrderedTaskBehaviour *ordered_props) const
{
  Chart chart(chart_look, canvas, rc);
  if (is_map) {
    chart.PaddingBottom = 0;
    chart.PaddingLeft = 0;
  }
  scale_chart(calculated, settings_computer, chart);
  _DrawThermalBand(basic, calculated, settings_computer,
                   chart, task_props, false, ordered_props);

  if (!is_map) {
    chart.DrawXGrid(Units::ToSysVSpeed(fixed_one), fixed_zero,
                    ChartLook::STYLE_THINDASHPAPER, fixed_one, true);
    chart.DrawYGrid(Units::ToSysAltitude(fixed(1000)),
                    fixed_zero,
                    ChartLook::STYLE_THINDASHPAPER,
                    fixed(1000), true);
    chart.DrawXLabel(_T("w"));
    chart.DrawYLabel(_T("h AGL"));
  }
}

void 
ThermalBandRenderer::DrawThermalBandSpark(const MoreData &basic,
                                          const DERIVED_INFO& calculated,
                                          const SETTINGS_COMPUTER &settings_computer,
                                          Canvas &canvas, 
                                          const PixelRect &rc,
                                          const TaskBehaviour &task_props) const
{
  Chart chart(chart_look, canvas, rc);
  chart.PaddingBottom = 0;
  chart.PaddingLeft = IBLSCALE(3);
  scale_chart(calculated, settings_computer, chart);
  _DrawThermalBand(basic, calculated, settings_computer,
                   chart, task_props, true, NULL);
}
