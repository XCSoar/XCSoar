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
#include "Airspace/Airspaces.hpp"

#include <assert.h>

#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspaceVisitor.hpp"

/**
 * Class to render airspaces onto map in two passes,
 * one for border, one for area.
 * This is a bit slow because projections are performed twice.
 * The old way of doing it was possibly faster but required a lot
 * of code overhead.
 */

class AirspaceVisitorMap: public AirspaceVisitor 
{
public:
  AirspaceVisitorMap(MapWindow &_map,
                     Canvas& _buffer):map(_map),
                                      buffer(_buffer),
                                      m_found(false),
                                      m_border(true)
    {

    };

  void Visit(const AirspaceCircle& airspace) {
    if (!check_visible(airspace)) 
      return;

    start_render(airspace);
    POINT center;
    map.LonLat2Screen(airspace.get_center(),center);
    unsigned radius = map.DistanceMetersToScreen(airspace.get_radius());
    buffer.circle(center.x, center.y, radius);
  }

  void Visit(const AirspacePolygon& airspace) {
    if (!check_visible(airspace)) 
      return;
    start_render(airspace);

    const SearchPointVector& points = airspace.get_points();
    const size_t size = points.size();
    std::vector<POINT> screen; 
    screen.reserve(size);
    for (SearchPointVector::const_iterator it = points.begin();
         it!= points.end(); ++it) {
      POINT sc;
      map.LonLat2Screen(it->get_location(), sc);
      screen.push_back(sc);
    }
    buffer.polyline(&screen[0], size);
  }

  void set_fill() {
    m_border = false;
    if (m_found) {
      buffer.hollow_brush();
      buffer.white_pen();
    }
  }

  void finish(Canvas& canvas) {
    if (m_found) {
      // need to do this to prevent drawing of colored outline
      buffer.white_pen();
      canvas.copy_transparent_white(buffer, map.GetMapRect());

      // restore original color
      //    SetTextColor(hDCTemp, origcolor);
      buffer.background_opaque();
    }
  }

private:
  bool check_visible(const AbstractAirspace& airspace) {
    if (!airspace.type_visible(map.SettingsComputer()) ||
        !airspace.altitude_visible(map.Basic().GetAnyAltitude(),
                                   map.SettingsComputer())) {
      return false;
    }
#ifdef OLD_TASK        
    if (airspace._NewWarnAckNoBrush ||
        (map.SettingsMap().iAirspaceBrush[airspace.Type] == NUMAIRSPACEBRUSHES-1)) {
      return m_border;
    } else {
      return true;
    }
#else
    return true;
#endif
  }

  void start_render(const AbstractAirspace &airspace) {
    if (!m_found) {
      m_found = true;
      clear();
      if (!m_border) {
        buffer.hollow_brush();
        buffer.white_pen();
      }
    }
    if (m_border) {
      // this color is used as the black bit
      buffer.set_text_color(MapGfx.Colours[map.SettingsMap().
                                           iAirspaceColour[airspace.get_type()]]);
      // get brush, can be solid or a 1bpp bitmap
      buffer.select(MapGfx.hAirspaceBrushes[map.SettingsMap().
                                            iAirspaceBrush[airspace.get_type()]]);
    } else {
      if (map.SettingsMap().bAirspaceBlackOutline)
        buffer.black_pen();
      else
        buffer.select(MapGfx.hAirspacePens[airspace.get_type()]);
    }
  }

  void clear() {
    static const Color whitecolor(0xff,0xff,0xff);
    buffer.background_transparent();
    buffer.set_background_color(whitecolor);
    buffer.set_text_color(whitecolor);
    buffer.white_pen();
    buffer.white_brush();
    const RECT &MapRect = map.GetMapRect();
    buffer.rectangle(MapRect.left, MapRect.top, MapRect.right, MapRect.bottom);
  }

  bool m_found;
  bool m_border;
  Canvas &buffer;
  MapWindow &map;
};


/**
 * Draws the airspace to the given canvas
 * @param canvas The drawing canvas
 * @param rc The area to draw in
 * @param buffer The drawing buffer
 */
void
MapWindow::DrawAirspace(Canvas &canvas, Canvas &buffer)
{
  if (airspace_database == NULL)
    return;

  AirspaceVisitorMap v(*this, buffer);
  airspace_database->visit_within_range(PanLocation, GetScreenDistanceMeters(), v);
  v.set_fill();
  airspace_database->visit_within_range(PanLocation, GetScreenDistanceMeters(), v);
  v.finish(canvas);
}
