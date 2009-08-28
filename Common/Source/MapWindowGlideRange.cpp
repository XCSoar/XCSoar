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
#include "XCSoar.h"
#include "Blackboard.hpp"
#include "SettingsTask.hpp"
#include "SettingsUser.hpp"
#include "Screen/Util.hpp"
#include "Screen/Graphics.hpp"
#include "Compatibility/gdi.h"


void MapWindow::CalculateScreenPositionsGroundline(void) {
  if (FinalGlideTerrain) {
    LatLon2Screen(DerivedDrawInfo.GlideFootPrint,
		  Groundline, NUMTERRAINSWEEPS+1, 1);
  }
}


void MapWindow::DrawTerrainAbove(Canvas &canvas, const RECT rc, Canvas &buffer) {

  if (!DerivedDrawInfo.Flying) return;

  buffer.background_transparent();
  buffer.set_background_color(Color(0xff,0xff,0xff));

  buffer.white_pen();
  buffer.set_text_color(Color(0xf0,0xf0,0xf0));
  buffer.select(MapGfx.hAboveTerrainBrush); // hAirspaceBrushes[3] or 6
  buffer.rectangle(rc.left, rc.top, rc.right, rc.bottom);

  buffer.white_pen();
  buffer.white_brush();
  buffer.polygon(Groundline, NUMTERRAINSWEEPS + 1);

  // need to do this to prevent drawing of colored outline
  buffer.white_pen();

  canvas.copy_transparent_white(buffer, rc);

  // restore original color
  buffer.background_opaque();
}


void MapWindow::DrawGlideThroughTerrain(Canvas &canvas, const RECT rc) {
  canvas.select(MapGfx.hpTerrainLineBg);
  canvas.clipped_polyline(Groundline, NUMTERRAINSWEEPS + 1, rc);
  if ((FinalGlideTerrain==1) ||
      ((!EnableTerrain || !DerivedDrawInfo.Flying) && (FinalGlideTerrain==2))) {
    canvas.select(MapGfx.hpTerrainLine);
    canvas.clipped_polyline(Groundline, NUMTERRAINSWEEPS + 1, rc);
  }

  if (DerivedDrawInfo.Flying && ValidTaskPoint(ActiveWayPoint)) {
    if ((DerivedDrawInfo.TerrainWarningLatitude != 0.0)
        &&(DerivedDrawInfo.TerrainWarningLongitude != 0.0)) {

      POINT sc;
      if (PointVisible(DerivedDrawInfo.TerrainWarningLongitude,
                       DerivedDrawInfo.TerrainWarningLatitude)) {
        LatLon2Screen(DerivedDrawInfo.TerrainWarningLongitude,
                      DerivedDrawInfo.TerrainWarningLatitude, sc);
        DrawBitmapIn(canvas, sc, MapGfx.hTerrainWarning);
      }
    }
  }
}


#include "McReady.h"
#include "InfoBoxLayout.h"
extern Font MapWindowBoldFont;

/*
 * The VisualGlide by Paolo Ventafridda
 * Sort of a Stocker dynamic chart!
 *
 * VisualGlide=1 : Steady sector/circle
 *             2 : Moving sector/circle   optional configurable, not much useful.
 */
void MapWindow::DrawGlideCircle(Canvas &canvas, POINT Orig, RECT rc)
{
  double tmp=0;
  TCHAR gtext[100];
  char text[20]; // TODO size it

  double cruise= CALCULATED_INFO.AverageLD;
  static double maxcruise=(GlidePolar::bestld);
  static double mincruise=(GlidePolar::bestld/4);
  int i;
  double gunit;
  static int spread=0;
  //static short rcx=rc.left+rc.right/2-30;
  //static short rcy=rc.top+rc.bottom-35;
  short rcx=rc.left+rc.right/2-IBLSCALE(20);
  short rcy=rc.bottom-IBLSCALE(15); // 35

  if ( cruise < 0 ) cruise = GlidePolar::bestld;
  if ( cruise < mincruise ) return;
  if ( cruise >maxcruise ) cruise=maxcruise;

  // Spread from
  static short turn=1;
  static short count=0;
  spread += (10 * turn);
  if ( spread <-25 || spread >25 ) turn*=-1;
  if ( ++count >6) count=-1;

  canvas.hollow_brush();
  // SetBkMode(hdc,TRANSPARENT);

  canvas.select(MapWindowBoldFont);

  // 100m or 300ft scale
  if ( Units::GetUserAltitudeUnit() == unMeter ) gunit=100; else gunit = 91.44;

  for (i=1; i<9; i++) {

    canvas.select(MapGfx.hpVisualGlideHeavyBlack);

    /*
     * TRACKUP, NORTHUP, NORTHCIRCLE, TRACKCIRCLE, NORTHTRACK
     */
    if ( ( DisplayOrientation == TRACKUP) || (DisplayOrientation == NORTHCIRCLE)
	 || (DisplayOrientation == TRACKCIRCLE)
	 && (DisplayMode != dmCircling) )
      {
	if ( VisualGlide == 1 ) {
	  tmp = i*gunit*cruise*ResMapScaleOverDistanceModify;
          canvas.arc(Orig.x, Orig.y, (int)tmp, rc, 315, 45);
	} else
	  {
	    tmp = i*gunit*cruise*ResMapScaleOverDistanceModify;
            canvas.arc(Orig.x, Orig.y, (int)tmp, rc,
                       330 + spread, 30 + spread);
	  }
      } else
      {
	tmp = i*gunit*cruise*ResMapScaleOverDistanceModify;
        canvas.circle(Orig.x, Orig.y, (int)tmp, rc, true, false);
      }


    canvas.set_text_color(turn > 0 || true
                          ? Color(0x0,0x0,0x0)
                          : Color(0xff,0x00,0x00)); // red
    if ( i==2 || i==4 || i==6 || i==8 ) {
      if (Units::GetUserAltitudeUnit() == unMeter)
        _stprintf(gtext, _T("-%dm"), i * 100);
      else
        _stprintf(gtext, _T("-%dft"), i * 300);
      if (count<5)
        canvas.text(Orig.x + 35, Orig.y - 5 - (int)tmp, gtext);
    }

    canvas.set_text_color(turn > 0 || true
                          ? Color(0x0,0x0,0x0) // dark grey
                          : Color(0xff,0x00,0x00)); // red
    if ( i==2 || i==4 || i==6 || i==8 ) {
      if ( Units::GetUserDistanceUnit() == unKiloMeter )
	{
	  //sprintf(text,"%3.1f Km",i*100*cruise /1000);
	  sprintf(text,"%3.0fkm",i*100*cruise /1000);
	} else  if ( Units::GetUserDistanceUnit() == unNauticalMiles )
	{
	  sprintf(text,"%3.0fnm", i*100*cruise / 1852);
	} else  if ( Units::GetUserDistanceUnit() == unStatuteMiles )
	{
	  sprintf(text,"%3.0fm", i*100*cruise / 1609);
	}

      _stprintf(gtext, _T("%S"), text);
      if (count<5)
        canvas.text(Orig.x - 100, Orig.y - 5 - (int)tmp, gtext);
    }
  }

/*
  if (NewMap&&OutlinedTp)
    oldcolor=SetTextColor(hdc, RGB(0x0,0x0,0x0)); // dark grey 0x50
  else {
    if (turn>0||true)
      oldcolor=SetTextColor(hdc, RGB(0x0,0x0,0x0)); // dark grey 0x50
    else
      oldcolor=SetTextColor(hdc, RGB(0xff,0x00,0x00)); // red
  }
  _stprintf(gtext,_T("L/D:%d"),(int)cruise);

  //ExtTextOut( hdc, Orig.x+30, Orig.y +20 , 0, NULL, gtext , _tcslen(gtext), NULL );
  //ExtTextOut( hdc, Orig.x-30, Orig_Aircraft.y +50 , 0, NULL, gtext , _tcslen(gtext), NULL );
  //ExtTextOut( hdc, (rc.left+rc.right)/2, rc.top+rc.bottom-20 , 0, NULL, gtext , _tcslen(gtext), NULL );

  if (NewMap&&OutlinedTp) {
    ExtTextOut( hdc, rcx+2, rcy , 0, NULL, gtext , _tcslen(gtext), NULL );
    ExtTextOut( hdc, rcx+1, rcy , 0, NULL, gtext , _tcslen(gtext), NULL );
    ExtTextOut( hdc, rcx-1, rcy , 0, NULL, gtext , _tcslen(gtext), NULL );
    ExtTextOut( hdc, rcx-2, rcy , 0, NULL, gtext , _tcslen(gtext), NULL );
    ExtTextOut( hdc, rcx, rcy+1 , 0, NULL, gtext , _tcslen(gtext), NULL );
    ExtTextOut( hdc, rcx, rcy-1 , 0, NULL, gtext , _tcslen(gtext), NULL );

#ifdef PNA
    if (GlobalModelType == MODELTYPE_PNA_HP31X ) {
	    ExtTextOut( hdc, rcx+3, rcy , 0, NULL, gtext , _tcslen(gtext), NULL );
	    ExtTextOut( hdc, rcx-3, rcy , 0, NULL, gtext , _tcslen(gtext), NULL );
	    ExtTextOut( hdc, rcx, rcy+2 , 0, NULL, gtext , _tcslen(gtext), NULL );
	    ExtTextOut( hdc, rcx, rcy-2 , 0, NULL, gtext , _tcslen(gtext), NULL );
	    ExtTextOut( hdc, rcx, rcy+3 , 0, NULL, gtext , _tcslen(gtext), NULL );
	    ExtTextOut( hdc, rcx, rcy-3 , 0, NULL, gtext , _tcslen(gtext), NULL );
    }
#endif

    SetTextColor(hdc,RGB(0xff,0xff,0xff));
  }
  ExtTextOut( hdc, rcx, rcy , 0, NULL, gtext , _tcslen(gtext), NULL );
*/
}
