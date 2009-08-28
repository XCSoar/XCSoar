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
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Marks.h"
#include "TerrainRenderer.h"
#include "RasterTerrain.h"
#include "Task.h"
#include "TopologyStore.h"
#include "Blackboard.hpp"

void MapWindow::RenderMapWindowBg(HDC hdc, const RECT rc,
				  const POINT &Orig,
				  const POINT &Orig_Aircraft)
{
  HFONT hfOld;

  // do slow calculations before clearing the screen
  // to reduce flicker
  CalculateWaypointReachable();
  CalculateScreenPositionsAirspace();
  CalculateScreenPositionsThermalSources();
  CalculateScreenPositionsGroundline();

  if (!EnableTerrain || !DerivedDrawInfo.TerrainValid
      || !RasterTerrain::isTerrainLoaded() ) {

    // JMW this isn't needed any more unless we're not drawing terrain

    // display border and fill background..
    SelectObject(hdc, MapGfx.hBackgroundBrush);
    SelectObject(hdc, GetStockObject(WHITE_PEN));
    Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
  }

  SelectObject(hdc, GetStockObject(BLACK_BRUSH));
  SelectObject(hdc, GetStockObject(BLACK_PEN));
  hfOld = (HFONT)SelectObject(hdc, MapWindowFont);

  // ground first...

  if (BigZoom) {
    BigZoom = false;
  }

  if ((EnableTerrain && (DerivedDrawInfo.TerrainValid)
       && RasterTerrain::isTerrainLoaded())
      || RasterTerrain::render_weather) {
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
    mutexTerrainDataGraphics.Lock();
    DrawTerrain(hdc, rc, sunazimuth, sunelevation,
		DrawInfo.Longitude, DrawInfo.Latitude,
	        BigZoom);
    if ((FinalGlideTerrain==2) && DerivedDrawInfo.TerrainValid) {
      SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);
      DrawTerrainAbove(hdc, rc, hDCTemp);
    }
    mutexTerrainDataGraphics.Unlock();

    if (BigZoom) {
      BigZoom = false;
    }
  }

  if (EnableTopology) {
    DrawTopology(hdc, rc);
  }

  // reset label over-write preventer
  LabelBlockReset();

  if (!TaskIsTemporary()) {
    SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);
    DrawTaskAAT(hdc, rc, hDCTemp);
  }

  // then airspace..
  if (OnAirSpace > 0) {
    // VENTA3 default is true, always true at startup no regsave
    SelectObject(hDCTemp, (HBITMAP)hDrawBitMapTmp);
    DrawAirSpace(hdc, rc, hDCTemp);
  }

  if(TrailActive) {
    // TODO enhancement: For some reason, the shadow drawing of the
    // trail doesn't work in portrait mode.  No idea why.
    if (1) {
      double TrailFirstTime =
	DrawTrail(hdc, Orig_Aircraft, rc);
      DrawTrailFromTask(hdc, rc, TrailFirstTime);
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

      BitBlt(hdcDrawWindowBg, rc.left, rc.top, rc.right, rc.bottom,
      hDCMask, 1, 1, SRCAND);
      BitBlt(hdcDrawWindowBg, rc.left, rc.top, rc.right, rc.bottom,
      hDCTemp, rc.left, rc.top, SRCPAINT);
      BitBlt(hdcDrawWindowBg, rc.left, rc.top, rc.right, rc.bottom,
      hDCTemp, rc.left, rc.top, SRCAND);
      */
    }
  }

  DrawThermalEstimate(hdc, rc);

  if (TaskAborted) {
    DrawAbortedTask(hdc, rc, Orig_Aircraft);
  } else {
    DrawTask(hdc, rc, Orig_Aircraft);
  }

  // draw red cross on glide through terrain marker
  if (FinalGlideTerrain && DerivedDrawInfo.TerrainValid) {
    DrawGlideThroughTerrain(hdc, rc);
  }

  DrawWaypoints(hdc,rc);

  DrawTeammate(hdc, rc);

  if ((EnableTerrain && (DerivedDrawInfo.TerrainValid))
      || RasterTerrain::render_weather) {
    DrawSpotHeights(hdc);
  }

  if (extGPSCONNECT) {
    // TODO enhancement: don't draw offtrack indicator if showing spot heights
    DrawProjectedTrack(hdc, rc, Orig_Aircraft);
    DrawOffTrackIndicator(hdc, rc);
    DrawBestCruiseTrack(hdc, Orig_Aircraft);
  }
  DrawBearing(hdc, rc, extGPSCONNECT);


  // draw wind vector at aircraft
  if (!EnablePan) {
    DrawWindAtAircraft2(hdc, Orig_Aircraft, rc);
  } else if (TargetPan) {
    DrawWindAtAircraft2(hdc, Orig, rc);
  }

  // Draw traffic
  DrawFLARMTraffic(hdc, rc, Orig_Aircraft);

  // finally, draw you!

  if (EnablePan && !TargetPan) {
    DrawCrossHairs(hdc, Orig, rc);
  }

  if (extGPSCONNECT) {
    DrawAircraft(hdc, Orig_Aircraft);
  }

  if ( (!TargetPan) && (!EnablePan) && (VisualGlide>0) ) {
    DrawGlideCircle(hdc, Orig, rc);
  }

  // marks on top...
  DrawMarks(hdc, rc);
  SelectObject(hdcDrawWindow, hfOld);

}


void MapWindow::RenderMapWindow(HDC hdc, const RECT rc)
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

  POINT Orig, Orig_Aircraft;

  CalculateOrigin(rc, &Orig);

  CalculateScreenPositions(Orig, rc, &Orig_Aircraft);

  RenderMapWindowBg(hdc, rc, Orig, Orig_Aircraft);

  // overlays
  DrawCDI();

  hfOld = (HFONT)SelectObject(hdc, MapWindowFont);

  DrawMapScale(hdc,rc, BigZoom);

  DrawMapScale2(hdc,rc, Orig_Aircraft);

  DrawCompass(hdc, rc);

  // JMW Experimental only! EXPERIMENTAL
#if 0
  //  #ifdef GNAV
  if (EnableAuxiliaryInfo) {
    DrawHorizon(hdc, rc);
  }
  //  #endif
#endif

  DrawFlightMode(hdc, rc);

  DrawThermalBand(hdc, rc);

  DrawFinalGlide(hdcDrawWindow,rc);

  //  DrawSpeedToFly(hdc, rc);

  DrawGPSStatus(hdc, rc);

  SelectObject(hdc, hfOld);

}
