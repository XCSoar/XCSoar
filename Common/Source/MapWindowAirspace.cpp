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
#include "Dialogs.h"

#include <assert.h>

#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspaceVisitor.hpp"

#include "AirspaceVisibility.hpp"

class AirspaceMapVisible: public AirspaceVisible
{
public:
  AirspaceMapVisible(const SETTINGS_COMPUTER& _settings, 
                     const fixed& _altitude, const bool& _border):
    m_border(_border),
    AirspaceVisible(_settings, _altitude)
    {};

  virtual bool operator()( const AbstractAirspace& airspace ) const { 
    return condition(airspace);
  }

  bool condition( const AbstractAirspace& airspace ) const { 
    if (!parent_condition(airspace)) {
      return false;
    }
#ifdef OLD_TASK        
    if (airspace._NewWarnAckNoBrush ||
        (m_settings.iAirspaceBrush[airspace.Type] == NUMAIRSPACEBRUSHES-1)) {
      return m_border;
    } else {
      return true;
    }
#else
    return true;
#endif

  }
private:
  const bool &m_border;
};

#include "MapDrawHelper.hpp"


/**
 * Class to render airspaces onto map in two passes,
 * one for border, one for area.
 * This is a bit slow because projections are performed twice.
 * The old way of doing it was possibly faster but required a lot
 * of code overhead.
 */
class AirspaceVisitorMap: 
  public AirspaceVisitor,
  public MapDrawHelper
{
public:
  AirspaceVisitorMap(MapDrawHelper &_helper):
    MapDrawHelper(_helper),
    m_border(false),
    visible(m_map.SettingsComputer(),
            m_map.Basic().GetAnyAltitude(),
            m_border)
    {
      m_predicate = &visible;
    };

  void Visit(const AirspaceCircle& airspace) {
    buffer_render_start();
    set_buffer_pens(airspace);

    POINT center;
    m_map.LonLat2Screen(airspace.get_center(),center);
    unsigned radius = m_map.DistanceMetersToScreen(airspace.get_radius());
    m_buffer.circle(center.x, center.y, radius);
  }

  void Visit(const AirspacePolygon& airspace) {
    buffer_render_start();
    set_buffer_pens(airspace);
    draw_search_point_vector(m_buffer, airspace.get_points());
  }

  void set_border(bool set) {
    m_border = set;
  }

private:

  void set_buffer_pens(const AbstractAirspace &airspace) {
    if (m_border) {
      if (m_map.SettingsMap().bAirspaceBlackOutline)
        m_buffer.black_pen();
      else
        m_buffer.select(MapGfx.hAirspacePens[airspace.get_type()]);

      m_buffer.hollow_brush();

    } else {
      // this color is used as the black bit
      m_buffer.set_text_color(MapGfx.Colours[m_map.SettingsMap().
                                           iAirspaceColour[airspace.get_type()]]);
      // get brush, can be solid or a 1bpp bitmap
      m_buffer.select(MapGfx.hAirspaceBrushes[m_map.SettingsMap().
                                            iAirspaceBrush[airspace.get_type()]]);
      m_buffer.white_pen();
    }
  }

  AirspaceMapVisible visible;
  bool m_border;
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

  MapDrawHelper helper (canvas, buffer, *this, GetMapRect());
  AirspaceVisitorMap v(helper);
  v.set_border(false);
  airspace_database->visit_within_range(PanLocation, GetScreenDistanceMeters(), v);
  v.set_border(true);
  airspace_database->visit_within_range(PanLocation, GetScreenDistanceMeters(), v);
}


class AirspaceDetailsDialogVisitor: 
  public AirspaceVisitor
{
public:
  AirspaceDetailsDialogVisitor(const SETTINGS_COMPUTER& _settings, 
                               const fixed& _altitude):
    AirspaceVisitor(AirspaceMapVisible(_settings, _altitude, false)),
    m_airspace(NULL) {};

  void Visit(const AirspacePolygon& as) {
    visit_general(as);
  };
  void Visit(const AirspaceCircle& as) {
    visit_general(as);
  };
  void visit_general(const AbstractAirspace& as) {
    if (m_predicate->condition(as)) {
      m_airspace = &as;
    }
  };
  void display() {
    if (m_airspace) {
      dlgAirspaceDetails(*m_airspace);
    }
  };
  bool found() const {
    return m_airspace != NULL;
  }
private:
  const AbstractAirspace *m_airspace;
};


bool
MapWindow::AirspaceDetailsAtPoint(const GEOPOINT &location) const
{
  if (airspace_database == NULL)
    return false;

  AirspaceDetailsDialogVisitor airspace_copy_popup(SettingsComputer(),
                                                   Basic().GetAnyAltitude());

  airspace_database->visit_within_range(location, 100.0, airspace_copy_popup);

  airspace_copy_popup.display();

  return airspace_copy_popup.found();
}
