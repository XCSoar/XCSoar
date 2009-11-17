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
#include "MapWindow.h"
#include "SettingsUser.hpp"
#include "SettingsTask.hpp"
#include "SettingsComputer.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/LabelBlock.hpp"
#include "Marks.h"
#include "TerrainRenderer.h"
#include "RasterTerrain.h"
#include "Task.h"
#include "TopologyStore.h"
#include "Components.hpp"
#include "RasterWeather.h"

/**
 * Calculates the screen positions of all important features
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 */
void MapWindow::RenderStart(Canvas &canvas, const RECT rc)
{
  // Calculate screen position of the aircraft
  CalculateOrigin(rc, Basic(), Calculated(),
		  SettingsComputer(),
		  SettingsMap());

  // Calculate screen positions of the visible waypoints
  CalculateScreenPositionsWaypoints();

  CalculateScreenPositionsTask();

  // Calculate screen positions of the airspaces
  CalculateScreenPositionsAirspace();

  // Calculate screen positions of the thermal sources
  CalculateScreenPositionsThermalSources();

  // Calculate screen positions of the final glide groundline
  CalculateScreenPositionsGroundline();

  if (BigZoom) {
    BigZoom = false;
  }
}

/**
 * Renders a white background (if no terrain) and resets brush, pen and font
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 */
void MapWindow::RenderBackground(Canvas &canvas, const RECT rc)
{
  // If (no other background chosen) create white background
  if (!SettingsMap().EnableTerrain || !Calculated().TerrainValid
      || !terrain.isTerrainLoaded() ) {
    canvas.select(MapGfx.hBackgroundBrush);
    canvas.white_pen();
    canvas.rectangle(rc.left, rc.top, rc.right, rc.bottom);
  }

  // Select black brush/pen and the MapWindowFont
  canvas.black_brush();
  canvas.black_pen();
  canvas.select(MapWindowFont);
}

/**
 * Renders the terrain background, the groundline and the topology
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 */
void MapWindow::RenderMapLayer(Canvas &canvas, const RECT rc)
{
  if ((SettingsMap().EnableTerrain && (Calculated().TerrainValid)
       && terrain.isTerrainLoaded())
      || RASP.GetParameter()) {
    double sunelevation = 40.0;
    double sunazimuth = DisplayAngle-Calculated().WindBearing;

    // draw sun from constant angle if very low wind speed
    if (Calculated().WindSpeed<0.5) {
      sunazimuth = DisplayAngle + 45.0;
    }

    // if (dirtyEvent.test()) {
    //   // map has been dirtied since we started drawing, so hurry up
    //   BigZoom = true;
    // }
    // TODO: implement a workaround

    if (terrain.isTerrainLoaded()) {
      // TODO feature: sun-based rendering option

      if (!terrain_renderer) {
        // defer rendering until first draw because
        // the buffer size, smoothing etc is set by the
        // loaded terrain properties
        terrain_renderer = new TerrainRenderer(MapRectBig);
      }
      terrain_renderer->SetSettings(SettingsMap().TerrainRamp,
                                    SettingsMap().TerrainContrast,
                                    SettingsMap().TerrainBrightness);

      // Draw the terrain
      terrain_renderer->Draw(canvas, *this, sunazimuth,
                             sunelevation, Basic().Location, BigZoom);
    }

    if ((SettingsComputer().FinalGlideTerrain==2) && Calculated().TerrainValid) {
      // Draw the groundline (and shading)
      DrawTerrainAbove(canvas, rc, buffer_canvas);
    }

    if (BigZoom) {
      BigZoom = false;
    }
  }

  if (SettingsMap().EnableTopology) {
    // Draw the topology
    topology->Draw(canvas, *this, rc);
  }

  // reset label over-write preventer
  label_block.reset();
}

/**
 * Render the AAT areas and airspace
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 */
void MapWindow::RenderAreas(Canvas &canvas, const RECT rc)
{
  // Draw AAT areas
  if (!task.TaskIsTemporary()) {
    DrawTaskAAT(canvas, rc, buffer_canvas);
  }

  // Draw airspace on top
  if (SettingsMap().OnAirSpace > 0) {
    DrawAirSpace(canvas, rc, buffer_canvas);
  }
}

/**
 * Renders the snail trail feature
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 */
void MapWindow::RenderTrail(Canvas &canvas, const RECT rc)
{
  double TrailFirstTime = DrawTrail(canvas);
  DrawTrailFromTask(canvas, TrailFirstTime);
  DrawThermalEstimate(canvas);
}

/**
 * Renders the task, the waypoints and the marks
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 */
void MapWindow::RenderTask(Canvas &canvas, const RECT rc)
{
  if (task.isTaskAborted()) {
    DrawAbortedTask(canvas);
  } else {
    DrawTask(canvas, rc);
  }
  DrawWaypoints(canvas);
  marks->Draw(canvas, *this, rc);
}

/**
 * Render final glide through terrain marker and RASP spot heights (?)
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 */
void MapWindow::RenderGlide(Canvas &canvas, const RECT rc)
{
  // draw red cross on glide through terrain marker
  if (SettingsComputer().FinalGlideTerrain && Calculated().TerrainValid) {
    DrawGlideThroughTerrain(canvas);
  }
  /*
  if ( (!TargetPan) && (!EnablePan) && (VisualGlide>0) ) {
    DrawGlideCircle(canvas, rc);
  }
  */
  if ((SettingsMap().EnableTerrain && (Calculated().TerrainValid))
      || RASP.GetParameter()) {
    DrawSpotHeights(canvas);
  }
}

/**
 * Renders the aircraft, the FLARM targets and the wind arrow
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 */
void MapWindow::RenderAirborne(Canvas &canvas, const RECT rc)
{
  // Draw wind vector at aircraft
  if (!SettingsMap().EnablePan) {
    DrawWindAtAircraft2(canvas, Orig_Aircraft, rc);
  } else if (SettingsMap().TargetPan) {
    DrawWindAtAircraft2(canvas, Orig_Screen, rc);
  }

  // Draw traffic
  DrawTeammate(canvas);
  DrawFLARMTraffic(canvas);

  // Draw center screen cross hair in pan mode
  if (SettingsMap().EnablePan && !SettingsMap().TargetPan) {
    DrawCrossHairs(canvas);
  }

  // Finally, draw you!
  if (Basic().Connected) {
    DrawAircraft(canvas);
  }
}

/**
 * Renders the upper symbology (compass, map scale, flight mode icon,
 * thermal band, final glide bar and gps status)
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 */
void MapWindow::RenderSymbology_upper(Canvas &canvas, const RECT rc)
{
  // overlays
  DrawCDI();

  canvas.select(MapWindowFont);
  DrawMapScale(canvas, rc, BigZoom);
  DrawMapScale2(canvas, rc);
  DrawCompass(canvas, rc);

  // JMW Experimental only! EXPERIMENTAL
#if 0
  if (EnableAuxiliaryInfo) {
    DrawHorizon(canvas, rc);
  }
#endif

  DrawFlightMode(canvas, rc);
  DrawThermalBand(canvas, rc);
  DrawFinalGlide(canvas,rc);
  //  DrawSpeedToFly(canvas, rc);
  DrawGPSStatus(canvas, rc);
}

/**
 * Renders lower symbology (track lines, bearing, etc)
 * @param canvas
 * @param rc
 */
void MapWindow::RenderSymbology_lower(Canvas &canvas, const RECT rc)
{
  if (Basic().Connected) {
    // TODO enhancement: don't draw offtrack indicator if showing spot heights
    DrawProjectedTrack(canvas);
    DrawOffTrackIndicator(canvas);
    DrawBestCruiseTrack(canvas);
  }
  DrawBearing(canvas, Basic().Connected);
}

/**
 * Renders all the components of the moving map
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 */
void MapWindow::Render(Canvas &canvas, const RECT rc)
{
  // Calculate screen positions
  RenderStart(canvas, rc);

  // Render a clean background and reset pen, brush and font
  RenderBackground(canvas, rc);

  // Render terrain, groundline and topology
  RenderMapLayer(canvas, rc);

  // Render the AAT areas and airspace
  RenderAreas(canvas, rc);

  // Render the snail trail
  RenderTrail(canvas, rc);

  // Render task and waypoints
  RenderTask(canvas, rc);

  RenderGlide(canvas, rc);

  // Render lower symbology
  RenderSymbology_lower(canvas, rc);

  // Render aircraft symbol (and FLARM traffic)
  RenderAirborne(canvas, rc);

  // Render upper symbology
  RenderSymbology_upper(canvas, rc);
}

static void DrawSpotHeight_Internal(Canvas &canvas,
				    MapWindowProjection &map_projection,
				    LabelBlock &label_block,
				    TCHAR *Buffer, POINT pt) {
  int size = _tcslen(Buffer);
  if (size==0) {
    return;
  }
  POINT orig = map_projection.GetOrigScreen();
  RECT brect;
  SIZE tsize = canvas.text_size(Buffer);

  pt.x+= 2+orig.x;
  pt.y+= 2+orig.y;
  brect.left = pt.x;
  brect.right = brect.left+tsize.cx;
  brect.top = pt.y;
  brect.bottom = brect.top+tsize.cy;

  if (!label_block.check(brect))
    return;

  canvas.text(pt.x, pt.y, Buffer);
}

#include "RasterWeather.h"

void MapWindow::DrawSpotHeights(Canvas &canvas) {
  // JMW testing, display of spot max/min
  if (!RASP.GetParameter())
    return;
  if (!terrain_renderer)
    return;

  canvas.select(TitleWindowFont);

  TCHAR Buffer[20];

  RASP.ValueToText(Buffer, terrain_renderer->spot_max_val);
  DrawSpotHeight_Internal(canvas, *this, label_block,
			  Buffer, terrain_renderer->spot_max_pt);

  RASP.ValueToText(Buffer, terrain_renderer->spot_min_val);
  DrawSpotHeight_Internal(canvas, *this, label_block,
			  Buffer, terrain_renderer->spot_min_pt);
}
