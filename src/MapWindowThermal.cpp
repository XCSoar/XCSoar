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

#include "MapWindow.hpp"
#include "Math/Earth.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/ProtectedTaskManager.hpp"

void
MapWindow::CalculateScreenPositionsThermalSources()
{
  for (int i = 0; i < MAX_THERMAL_SOURCES; i++) {
    if (!positive(Calculated().ThermalSources[i].LiftRate)) {
      ThermalSources[i].Visible = false;
      continue;
    }

    fixed dh = Basic().NavAltitude -
                Calculated().ThermalSources[i].GroundHeight;
    if (negative(dh)) {
      ThermalSources[i].Visible = false;
      continue;
    }

    fixed t = -dh / Calculated().ThermalSources[i].LiftRate;
    GeoPoint loc;
    FindLatitudeLongitude(Calculated().ThermalSources[i].Location,
                          Basic().wind.bearing, Basic().wind.norm * t, &loc);
    ThermalSources[i].Visible =
        render_projection.LonLat2ScreenIfVisible(loc,
                                                 &ThermalSources[i].Screen);
  }
}

void
MapWindow::DrawThermalEstimate(Canvas &canvas) const
{
  const MapWindowProjection &projection = render_projection;

  if (projection.GetDisplayMode() == dmCircling) {
    if (positive(Calculated().ThermalEstimate_R)) {
      POINT sc;
      if (projection.LonLat2ScreenIfVisible(Calculated().ThermalEstimate_Location, &sc)) {
        Graphics::hBmpThermalSource.draw(canvas, bitmap_canvas, sc.x, sc.y);
      }
    }
  } else if (projection.GetMapScaleKM() <= fixed_four) {
    for (int i = 0; i < MAX_THERMAL_SOURCES; i++) {
      if (ThermalSources[i].Visible) 
        Graphics::hBmpThermalSource.draw(canvas, bitmap_canvas,
                                      ThermalSources[i].Screen.x,
                                      ThermalSources[i].Screen.y);
    }
  }
}

void
MapWindow::DrawThermalBand(Canvas &canvas, const RECT &rc) const
{
  POINT GliderBand[5] = { { 0, 0 }, { 23, 0 }, { 22, 0 }, { 24, 0 }, { 0, 0 } };

  if ((Calculated().task_stats.total.solution_remaining.AltitudeDifference > fixed(50))
      && render_projection.GetDisplayMode() == dmFinalGlide)
    return;

  const ThermalBandInfo &thermal_band = Calculated().thermal_band;

  // JMW TODO accuracy: gather proper statistics
  // note these should/may also be relative to ground
  int i;
  fixed mth = thermal_band.MaxThermalHeight;
  fixed maxh, minh;
  fixed h;
  fixed Wt[NUMTHERMALBUCKETS];
  fixed ht[NUMTHERMALBUCKETS];
  fixed Wmax = fixed_zero;
  int TBSCALEY = ((rc.bottom - rc.top) / 2) - IBLSCALE(30);
#define TBSCALEX 20

  // calculate height above safety altitude
  fixed hoffset = SettingsComputer().safety_height_terrain +
                   Calculated().TerrainBase;
  h = Basic().NavAltitude - hoffset;

  bool draw_start_height = false;
  fixed hstart = fixed_zero;
  
  OrderedTaskBehaviour task_props;
  if (task != NULL)
    task_props = task->get_ordered_task_behaviour();

  draw_start_height = Calculated().common_stats.ordered_valid
                      && (task_props.start_max_height != 0)
                      && Calculated().TerrainValid;
  if (draw_start_height) {
    if (task_props.start_max_height_ref == 0) {
      hstart = fixed(task_props.start_max_height) + Calculated().TerrainAlt;
    } else {
      hstart = fixed(task_props.start_max_height);
    }
    hstart -= hoffset;
  }

  // calculate top/bottom height
  maxh = max(h, mth);
  minh = min(h, fixed_zero);

  if (draw_start_height) {
    maxh = max(maxh, hstart);
    minh = min(minh, hstart);
  }

  // no thermalling has been done above safety altitude
  if (mth <= fixed_one)
    return;
  if (maxh <= minh)
    return;

  // normalised heights
  fixed hglider = (h - minh) / (maxh - minh);
  hstart = (hstart - minh) / (maxh - minh);

  // calculate averages
  int numtherm = 0;

  const fixed mc = get_glide_polar().get_mc();
  Wmax = max(fixed_half, mc);

  for (i = 0; i < NUMTHERMALBUCKETS; i++) {
    fixed wthis = fixed_zero;
    // height of this thermal point [0,mth]
    fixed hi = i * mth / NUMTHERMALBUCKETS;
    fixed hp = ((hi - minh) / (maxh - minh));

    if (thermal_band.ThermalProfileN[i] > 5) {
      // now requires 10 items in bucket before displaying,
      // to eliminate kinks
      wthis = thermal_band.ThermalProfileW[i] / thermal_band.ThermalProfileN[i];
    }
    if (positive(wthis)) {
      ht[numtherm] = hp;
      Wt[numtherm] = wthis;
      Wmax = max(Wmax, wthis * 2 / 3);
      numtherm++;
    }
  }

  if ((!draw_start_height) && (numtherm<=1))
    // don't display if insufficient statistics
    // but do draw if start height needs to be drawn
    return;

  // position of thermal band
  if (numtherm > 1) {
    canvas.select(Graphics::hpThermalBand);
    canvas.select(Graphics::hbThermalBand);

    POINT ThermalProfile[NUMTHERMALBUCKETS + 2];
    for (i = 0; i < numtherm; i++) {
      ThermalProfile[1 + i].x =
          (iround((Wt[i] / Wmax) * IBLSCALE(TBSCALEX))) + rc.left;

      ThermalProfile[1 + i].y =
          IBLSCALE(4) + iround(TBSCALEY * (fixed_one - ht[i])) + rc.top;
    }
    ThermalProfile[0].x = rc.left;
    ThermalProfile[0].y = ThermalProfile[1].y;
    ThermalProfile[numtherm + 1].x = rc.left;
    ThermalProfile[numtherm + 1].y = ThermalProfile[numtherm].y;

    canvas.polygon(ThermalProfile, numtherm + 2);
  }

  // position of thermal band

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

  canvas.select(Graphics::hpThermalBandGlider);

  canvas.polyline(GliderBand, 2);
  canvas.polyline(GliderBand + 2, 3); // arrow head

  if (draw_start_height) {
    canvas.select(Graphics::hpFinalGlideBelow);
    GliderBand[0].y = IBLSCALE(4) + iround(TBSCALEY * (fixed_one - hstart)) + rc.top;
    GliderBand[1].y = GliderBand[0].y;
    canvas.polyline(GliderBand, 2);
  }
}
