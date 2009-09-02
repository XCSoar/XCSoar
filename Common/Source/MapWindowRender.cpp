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
#include "Interface.hpp"
#include "RasterWeather.h"

void MapWindow::RenderMapWindowBg(Canvas &canvas, const RECT rc)
{
  // do slow calculations before clearing the screen
  // to reduce flicker
  CalculateWaypointReachable();
  CalculateScreenPositionsAirspace();
  CalculateScreenPositionsThermalSources();
  CalculateScreenPositionsGroundline();

  if (!EnableTerrain || !DerivedDrawInfo.TerrainValid
      || !terrain.isTerrainLoaded() ) {

    // JMW this isn't needed any more unless we're not drawing terrain

    // display border and fill background..
    canvas.select(MapGfx.hBackgroundBrush);
    canvas.white_pen();
    canvas.rectangle(rc.left, rc.top, rc.right, rc.bottom);
  }

  canvas.black_brush();
  canvas.black_pen();
  canvas.select(MapWindowFont);

  // ground first...

  if (BigZoom) {
    BigZoom = false;
  }

  if ((EnableTerrain && (DerivedDrawInfo.TerrainValid)
       && terrain.isTerrainLoaded())
      || RASP.RenderWeatherParameter) {
    double sunelevation = 40.0;
    double sunazimuth = DisplayAngle-DerivedDrawInfo.WindBearing;

    // draw sun from constant angle if very low wind speed
    if (DerivedDrawInfo.WindSpeed<0.5) {
      sunazimuth = DisplayAngle + 45.0;
    }

    if (dirtyEvent.test()) {
      // map has been dirtied since we started drawing, so hurry up
      BigZoom = true;
    }
    DrawTerrain(canvas, *this, sunazimuth, sunelevation,
		DrawInfo.Longitude, DrawInfo.Latitude,
	        BigZoom);

    if ((FinalGlideTerrain==2) && DerivedDrawInfo.TerrainValid) {
      DrawTerrainAbove(canvas, rc, buffer_canvas);
    }

    if (BigZoom) {
      BigZoom = false;
    }
  }

  if (EnableTopology) {
    topology->Draw(canvas, *this, rc);
  }

  // reset label over-write preventer
  label_block.reset();

  if (!TaskIsTemporary()) {
    DrawTaskAAT(canvas, rc, buffer_canvas);
  }

  // then airspace..
  if (OnAirSpace > 0) {
    // VENTA3 default is true, always true at startup no regsave
    DrawAirSpace(canvas, rc, buffer_canvas);
  }

  if(TrailActive) {
    // TODO enhancement: For some reason, the shadow drawing of the
    // trail doesn't work in portrait mode.  No idea why.
    if (1) {
      double TrailFirstTime = DrawTrail(canvas, rc);
      DrawTrailFromTask(canvas, rc, TrailFirstTime);
    } else {
      /*
      //  Draw trail with white outline --- very slow!
      // clear background bitmap
      SelectObject(hDCTemp, GetStockObject(WHITE_BRUSH));
      Rectangle(hDCTemp, rc.left, rc.top, rc.right, rc.bottom);

      SetBkColor(hDCMask, RGB(0xff,0xff,0xff));

      // draw trail on background bitmap
      DrawTrail(hDCTemp, Orig_Aircraft, rc);
      DrawTrailFromTask(hDCTemp, rc);

      // make mask
      BitBlt(hDCMask, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
      hDCTemp, rc.left, rc.top, SRCCOPY);

      BitBlt(draw_canvasBg, rc.left, rc.top, rc.right, rc.bottom,
      hDCMask, 1, 1, SRCAND);
      BitBlt(draw_canvasBg, rc.left, rc.top, rc.right, rc.bottom,
      hDCTemp, rc.left, rc.top, SRCPAINT);
      BitBlt(draw_canvasBg, rc.left, rc.top, rc.right, rc.bottom,
      hDCTemp, rc.left, rc.top, SRCAND);
      */
    }
  }

  DrawThermalEstimate(canvas, rc);

  if (TaskAborted) {
    DrawAbortedTask(canvas, rc);
  } else {
    DrawTask(canvas, rc);
  }

  // draw red cross on glide through terrain marker
  if (FinalGlideTerrain && DerivedDrawInfo.TerrainValid) {
    DrawGlideThroughTerrain(canvas, rc);
  }

  DrawWaypoints(canvas, rc);

  DrawTeammate(canvas, rc);

  if ((EnableTerrain && (DerivedDrawInfo.TerrainValid))
      || RASP.RenderWeatherParameter) {
    DrawSpotHeights(canvas, *this, label_block);
  }

  if (extGPSCONNECT) {
    // TODO enhancement: don't draw offtrack indicator if showing spot heights
    DrawProjectedTrack(canvas, rc);
    DrawOffTrackIndicator(canvas, rc);
    DrawBestCruiseTrack(canvas);
  }
  DrawBearing(canvas, rc, extGPSCONNECT);


  // draw wind vector at aircraft
  if (!EnablePan) {
    DrawWindAtAircraft2(canvas, Orig_Aircraft, rc);
  } else if (TargetPan) {
    DrawWindAtAircraft2(canvas, Orig_Screen, rc);
  }

  // Draw traffic
  DrawFLARMTraffic(canvas, rc);

  // finally, draw you!

  if (EnablePan && !TargetPan) {
    DrawCrossHairs(canvas, rc);
  }

  if (extGPSCONNECT) {
    DrawAircraft(canvas);
  }

  /*
  if ( (!TargetPan) && (!EnablePan) && (VisualGlide>0) ) {
    DrawGlideCircle(canvas, rc);
  }
  */

  // marks on top...
  marks->Draw(canvas, *this, rc);
}


void MapWindow::RenderMapWindow(Canvas &canvas, const RECT rc)
{
  bool drawmap = false;
  HFONT hfOld;

  DWORD fpsTime = ::GetTickCount();

  // only redraw map part every 800 s unless triggered
  if (((fpsTime-fpsTime0)>800)||(fpsTime0== 0)||(user_asked_redraw)) {
    fpsTime0 = fpsTime;
    drawmap = true;
    user_asked_redraw = false;
  }
  MapWindow::UpdateTimeStats(true);

  CalculateOrigin(rc, DrawInfo, DerivedDrawInfo);
  CalculateScreenPositionsWaypoints();
  CalculateScreenPositionsTask();

  RenderMapWindowBg(canvas, rc);

  // overlays
  DrawCDI();

  canvas.select(MapWindowFont);

  DrawMapScale(canvas, rc, BigZoom);
  DrawMapScale2(canvas, rc);

  DrawCompass(canvas, rc);

  // JMW Experimental only! EXPERIMENTAL
#if 0
  //  #ifdef GNAV
  if (EnableAuxiliaryInfo) {
    DrawHorizon(canvas, rc);
  }
  //  #endif
#endif

  DrawFlightMode(canvas, rc);

  DrawThermalBand(canvas, rc);

  DrawFinalGlide(canvas,rc);

  //  DrawSpeedToFly(canvas, rc);

  DrawGPSStatus(canvas, rc);
}


///////


//////////////////////////////////////////////////

TerrainRenderer *terrain_renderer = NULL;

void DrawTerrain(Canvas &canvas, 
		 MapWindowProjection &map_projection,
		 const double sunazimuth, const double sunelevation,
		 const double lon, const double lat,
		 const bool isBigZoom)
{
  // TODO feature: sun-based rendering option

  if (!terrain.isTerrainLoaded()) {
    return;
  }

  if (!terrain_renderer) {
    terrain_renderer = new TerrainRenderer(map_projection.GetMapRectBig());
  }
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
  if (!RASP.RenderWeatherParameter)
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

