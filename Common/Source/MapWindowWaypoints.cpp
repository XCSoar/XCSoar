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
#include "Screen/Graphics.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Waypoint/WaypointVisitor.hpp"

#include <assert.h>

// JMW OLD_TASK TODO: add visitor for task to draw task waypoints
// (can use same class WaypointVisitorMap)

//FIX
void MapWaypointLabelAdd(TCHAR *Name, int X, int Y,
			 TextInBoxMode_t Mode, int AltArivalAGL,
			 bool inTask, bool isLandable,
			 bool isAirport, bool isExcluded,
			 RECT rc);
void MapWaypointLabelClear();


class WaypointVisitorMap: public WaypointVisitor {
public:
  WaypointVisitorMap(MapWindow &_map,
                     Canvas& _canvas):map(_map),
                                     canvas(_canvas) 
    {
      // if pan mode, show full names
      pDisplayTextType = map.SettingsMap().DisplayTextType;
      if (map.SettingsMap().EnablePan) {
        pDisplayTextType = DISPLAYNAME;
      }
      _tcscpy(sAltUnit, Units::GetAltitudeName());
    };

  void DrawWaypoint(const Waypoint& way_point, bool intask=false) {

    POINT sc;
    if (map.LonLat2ScreenIfVisible(way_point.Location, &sc)) {

      TextInBoxMode_t TextDisplayMode;
      bool irange = false;
      bool islandable = false;
      bool dowrite = intask || (map.SettingsMap().DeclutterLabels<2);

      bool reachable = true; // XXXXX TODO, calculate with glide polar!!
      int AltArrivalAGL = 234*ALTITUDEMODIFY;
      
      TextDisplayMode.AsInt = 0;
      
      irange = map.WaypointInScaleFilter(way_point);
      
      Bitmap *wp_bmp = &MapGfx.hSmall;
      
      if (map.GetMapScaleKM() > 20) {
        wp_bmp = &MapGfx.hSmall;
      } else if(way_point.is_landable()) {
        islandable = true; // so we can always draw them
        if (reachable) {
          
          TextDisplayMode.AsFlag.Reachable = 1;
          
          if ((map.SettingsMap().DeclutterLabels<2)||intask) {
            
            if (intask || (map.SettingsMap().DeclutterLabels<1)) {
              TextDisplayMode.AsFlag.Border = 1;
            }
            // show all reachable landing fields unless we want a decluttered
            // screen.
            dowrite = true;
          }
          
          if (way_point.Flags.Airport)
            wp_bmp = &MapGfx.hBmpAirportReachable;
          else
            wp_bmp = &MapGfx.hBmpFieldReachable;
        } else {
          if (way_point.Flags.Airport)
            wp_bmp = &MapGfx.hBmpAirportUnReachable;
          else
            wp_bmp = &MapGfx.hBmpFieldUnReachable;
        }
      } else {
        if (map.GetMapScaleKM()>4) {
          wp_bmp = &MapGfx.hTurnPoint;
        } else {
          wp_bmp = &MapGfx.hSmall;
        }
      }
      
      if (intask) { // VNT
        TextDisplayMode.AsFlag.WhiteBold = 1;
      }
      
      if(irange || intask || islandable || dowrite) {
        map.draw_masked_bitmap(canvas, *wp_bmp,
                               sc.x, sc.y,
                               20, 20);
      }
      
      if (intask || irange || dowrite) {
        
        TCHAR Buffer[32];
        TCHAR Buffer2[32];
        
        bool draw_alt = TextDisplayMode.AsFlag.Reachable
          && ((map.SettingsMap().DeclutterLabels<1) || intask);
        
        switch(pDisplayTextType) {
        case DISPLAYNAMEIFINTASK:
          dowrite = intask;
          if (intask) {
            if (draw_alt)
              _stprintf(Buffer, TEXT("%s:%d%s"),
                        way_point.Name.c_str(),
                        AltArrivalAGL,
                        sAltUnit);
            else
              _stprintf(Buffer, TEXT("%s"),way_point.Name.c_str());
          }
          break;
        case DISPLAYNAME:
          if (draw_alt)
            _stprintf(Buffer, TEXT("%s:%d%s"),
                      way_point.Name.c_str(),
                      AltArrivalAGL,
                      sAltUnit);
          else
            _stprintf(Buffer, TEXT("%s"),way_point.Name.c_str());
          
          break;
        case DISPLAYNUMBER:
          if (draw_alt)
            _stprintf(Buffer, TEXT("%d:%d%s"),
                      way_point.id,
                      AltArrivalAGL,
                      sAltUnit);
          else
            _stprintf(Buffer, TEXT("%d"),way_point.id);
          
          break;
        case DISPLAYFIRSTFIVE:
          _tcsncpy(Buffer2, way_point.Name.c_str(), 5);
          Buffer2[5] = '\0';
          if (draw_alt)
            _stprintf(Buffer, TEXT("%s:%d%s"),
                      Buffer2,
                      AltArrivalAGL,
                      sAltUnit);
          else
            _stprintf(Buffer, TEXT("%s"),Buffer2);
          
          break;
        case DISPLAYFIRSTTHREE:
          _tcsncpy(Buffer2, way_point.Name.c_str(), 3);
          Buffer2[3] = '\0';
          if (draw_alt)
            _stprintf(Buffer, TEXT("%s:%d%s"),
                      Buffer2,
                      AltArrivalAGL,
                      sAltUnit);
          else
            _stprintf(Buffer, TEXT("%s"),Buffer2);
          
          break;
        case DISPLAYNONE:
          if (draw_alt)
            _stprintf(Buffer, TEXT("%d%s"),
                      AltArrivalAGL,
                      sAltUnit);
          else
            Buffer[0]= '\0';
          break;
        default:
          assert(0);
          break;
        }

        if (dowrite) {
          MapWaypointLabelAdd(
            Buffer,
            sc.x + 5, sc.y,
            TextDisplayMode,
            AltArrivalAGL,
            intask,false,false,false,
            map.GetMapRect());
        }
      }
    }
  }

  void Visit(const Waypoint& way_point) {
    DrawWaypoint(way_point);
  }
private:
  MapWindow &map;
  Canvas &canvas;
  int pDisplayTextType;
  TCHAR sAltUnit[4];
};



void MapWindow::DrawWaypoints(Canvas &canvas)
{
  if (way_points == NULL)
    return;

  MapWaypointLabelClear();

  WaypointVisitorMap v(*this, canvas);
  way_points->visit_within_range(PanLocation, GetScreenDistanceMeters(), v);

  // OLD_TASK -> new TODO, also draw waypoints in task 

  MapWaypointLabelSortAndRender(canvas);
}
