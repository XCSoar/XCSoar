/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include "Protection.hpp"
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
#include "Blackboard.hpp"
#include "Components.hpp"
#include "RasterWeather.h"


void MapWindow::RenderStart(Canvas &canvas, const RECT rc)
{
  CalculateOrigin(rc, Basic(), Calculated(), 
		  SettingsComputer(),
		  SettingsMap());
  CalculateScreenPositionsWaypoints();
  CalculateScreenPositionsTask();
  CalculateScreenPositionsAirspace();
  CalculateScreenPositionsThermalSources();
  CalculateScreenPositionsGroundline();
  if (BigZoom) {
    BigZoom = false;
  }
}


void MapWindow::RenderBackground(Canvas &canvas, const RECT rc)
{
  if (!SettingsMap().EnableTerrain || !Calculated().TerrainValid
      || !terrain.isTerrainLoaded() ) {
    canvas.select(MapGfx.hBackgroundBrush);
    canvas.white_pen();
    canvas.rectangle(rc.left, rc.top, rc.right, rc.bottom);
  }

  canvas.black_brush();
  canvas.black_pen();
  canvas.select(MapWindowFont);
}

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

    //    if (dirtyEvent.test()) {
    //      // map has been dirtied since we started drawing, so hurry up
    //      BigZoom = true;
    //    }
    // TODO: implement a workaround

    DrawTerrain(canvas, *this, sunazimuth, sunelevation,
		Basic().Longitude, Basic().Latitude,
	        BigZoom, SettingsMap());

    if ((SettingsComputer().FinalGlideTerrain==2) 
	&& Calculated().TerrainValid) {
      DrawTerrainAbove(canvas, rc, buffer_canvas);
    }

    if (BigZoom) {
      BigZoom = false;
    }
  }

  if (SettingsMap().EnableTopology) {
    topology->Draw(canvas, *this, rc);
  }

  // reset label over-write preventer
  label_block.reset();
}


void MapWindow::RenderAreas(Canvas &canvas, const RECT rc)
{
  if (!TaskIsTemporary()) {
    DrawTaskAAT(canvas, rc, buffer_canvas);
  }

  // then airspace..
  if (SettingsMap().OnAirSpace > 0) {
    DrawAirSpace(canvas, rc, buffer_canvas);
  }
}

void MapWindow::RenderTrail(Canvas &canvas, const RECT rc)
{
  double TrailFirstTime = DrawTrail(canvas);
  DrawTrailFromTask(canvas, TrailFirstTime);
  DrawThermalEstimate(canvas);
}

void MapWindow::RenderTask(Canvas &canvas, const RECT rc)
{
  if (TaskAborted) {
    DrawAbortedTask(canvas);
  } else {
    DrawTask(canvas, rc);
  }
  DrawWaypoints(canvas);
  marks->Draw(canvas, *this, rc);
}


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
    DrawSpotHeights(canvas, *this, label_block);
  }
}


void MapWindow::RenderAirborne(Canvas &canvas, const RECT rc)
{
  // draw wind vector at aircraft
  if (!SettingsMap().EnablePan) {
    DrawWindAtAircraft2(canvas, Orig_Aircraft, rc);
  } else if (TargetPan) {
    DrawWindAtAircraft2(canvas, Orig_Screen, rc);
  }

  // Draw traffic
  DrawTeammate(canvas);
  DrawFLARMTraffic(canvas);

  // finally, draw you!

  if (SettingsMap().EnablePan && !TargetPan) {
    DrawCrossHairs(canvas);
  }

  if (Basic().Connected) {
    DrawAircraft(canvas);
  }
}

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

void MapWindow::Render(Canvas &canvas, const RECT rc)
{
  RenderStart(canvas, rc);
  RenderBackground(canvas, rc);
  RenderMapLayer(canvas, rc);
  RenderAreas(canvas, rc);
  RenderTrail(canvas, rc);
  RenderTask(canvas, rc);
  RenderGlide(canvas, rc);

  RenderSymbology_lower(canvas, rc);
  RenderAirborne(canvas, rc);
  RenderSymbology_upper(canvas, rc);
}

//////////////////////////////////////////////////

// TODO: make this a member 
TerrainRenderer *terrain_renderer = NULL;

void DrawTerrain(Canvas &canvas, 
		 MapWindowProjection &map_projection,
		 const double sunazimuth, const double sunelevation,
		 const double lon, const double lat,
		 const bool isBigZoom,
		 const SETTINGS_MAP &settings)
{
  // TODO feature: sun-based rendering option

  if (!terrain.isTerrainLoaded()) {
    return;
  }

  if (!terrain_renderer) {
    terrain_renderer = new TerrainRenderer(map_projection.GetMapRectBig());
  }
  terrain_renderer->SetSettings(settings.TerrainRamp, 
				settings.TerrainContrast,
				settings.TerrainBrightness);

  terrain_renderer->Draw(canvas, map_projection, sunazimuth, sunelevation,
			 lon, lat, isBigZoom);
}


void CloseTerrainRenderer() {
  if (terrain_renderer) {
    delete terrain_renderer;
  }
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

void DrawSpotHeights(Canvas &canvas, 
		     MapWindowProjection &map_projection,
		     LabelBlock &label_block) {
  // JMW testing, display of spot max/min
  if (!RASP.GetParameter())
    return;
  if (!terrain_renderer)
    return;

  canvas.select(TitleWindowFont);

  TCHAR Buffer[20];

  RASP.ValueToText(Buffer, terrain_renderer->spot_max_val);
  DrawSpotHeight_Internal(canvas, map_projection, label_block, 
			  Buffer, terrain_renderer->spot_max_pt);

  RASP.ValueToText(Buffer, terrain_renderer->spot_min_val);
  DrawSpotHeight_Internal(canvas, map_projection, label_block,
			  Buffer, terrain_renderer->spot_min_pt);
}

