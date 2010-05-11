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
#include "AirspaceClientUI.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspaceVisitor.hpp"
#include "AirspaceVisibility.hpp"
#include "Airspace/AirspaceWarningVisitor.hpp"

class AirspaceWarningCopy: 
  public AirspaceWarningVisitor
{
public:
  void Visit(const AirspaceWarning& as) {
    if (as.get_warning_state()== AirspaceWarning::WARNING_INSIDE) {
      ids_inside.push_back(&as.get_airspace());
    } else if (as.get_warning_state()> AirspaceWarning::WARNING_CLEAR) {
      ids_warning.push_back(&as.get_airspace());
      locs.push_back(as.get_solution().location);
    }
    if (!as.get_ack_expired()) {
      ids_acked.push_back(&as.get_airspace());
    }
  }

  std::vector<GEOPOINT> get_locations() const {
    return locs;
  }
  bool is_warning(const AbstractAirspace& as) const {
    return find(as, ids_warning);
  }
  bool is_acked(const AbstractAirspace& as) const {
    return find(as, ids_acked);
  }
  bool is_inside(const AbstractAirspace& as) const {
    return find(as, ids_inside);
  }

private:
  bool find(const AbstractAirspace& as, 
            const std::vector<const AbstractAirspace*>& list) const {
    for (std::vector<const AbstractAirspace*>::const_iterator it = list.begin();
         it != list.end(); ++it) {
      if ((*it) == &as) {
        return true;
      }
    }
    return false;
  }

  std::vector<const AbstractAirspace*> ids_inside;
  std::vector<const AbstractAirspace*> ids_warning;
  std::vector<const AbstractAirspace*> ids_acked;
  std::vector<GEOPOINT> locs;
};


class AirspaceMapVisible: public AirspaceVisible
{
public:
  AirspaceMapVisible(const SETTINGS_COMPUTER& _settings, 
                     const fixed& _altitude, const bool& _border,
                     const AirspaceWarningCopy& warnings):
    m_border(_border),
    AirspaceVisible(_settings, _altitude),
    m_warnings(warnings)
    {
    };

  virtual bool operator()( const AbstractAirspace& airspace ) const { 
    return condition(airspace);
  }

  bool condition( const AbstractAirspace& airspace ) const { 
    return parent_condition(airspace) 
      || m_warnings.is_inside(airspace)
      || m_warnings.is_warning(airspace);
  }
private:
  const bool &m_border;
  const AirspaceWarningCopy& m_warnings;
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
  AirspaceVisitorMap(MapDrawHelper &_helper, const AirspaceWarningCopy& warnings,
                     const SETTINGS_COMPUTER& settings_computer,
                     const NMEA_INFO& state):
    MapDrawHelper(_helper),
    m_warnings(warnings),
    m_border(false),
    pen_thick(Pen::SOLID, IBLSCALE(10), Color(0x00, 0x00, 0x00)),
    pen_medium(Pen::SOLID, IBLSCALE(3), Color(0x00, 0x00, 0x00)),
    visible(settings_computer,
            state.GetAltitudeBaroPreferred(),
            m_border,
            warnings)
    {
      m_predicate = &visible;
    };

  void Visit(const AirspaceCircle& airspace) {
    buffer_render_start();
    set_buffer_pens(airspace);

    POINT center;
    m_proj.LonLat2Screen(airspace.get_center(),center);
    unsigned radius = m_proj.DistanceMetersToScreen(airspace.get_radius());
    draw_circle(m_buffer, center, radius);
  }

  void Visit(const AirspacePolygon& airspace) {
    buffer_render_start();
    set_buffer_pens(airspace);
    draw_search_point_vector(m_buffer, airspace.get_points());
  }

  void set_border(bool set) {
    m_border = set;
    m_use_stencil = !m_border;
  }

  void draw_intercepts() {
    set_border(false);
    buffer_render_finish();
  }

private:

  void set_buffer_pens(const AbstractAirspace &airspace) {
    if (m_border) {
      if (m_settings_map.bAirspaceBlackOutline)
        m_buffer.black_pen();
      else
        m_buffer.select(MapGfx.hAirspacePens[airspace.get_type()]);

      m_buffer.hollow_brush();

    } else {

      /// @todo stop drawing border for acked outside airspaces     

      if (m_warnings.is_acked(airspace)) {

        m_buffer.hollow_brush();

      } else {

        // this color is used as the black bit
        m_buffer.set_text_color(MapGfx.Colours[m_settings_map.
                                               iAirspaceColour[airspace.get_type()]]);
        // get brush, can be solid or a 1bpp bitmap
        m_buffer.select(MapGfx.hAirspaceBrushes[m_settings_map.
                                                iAirspaceBrush[airspace.get_type()]]);
        m_buffer.white_pen();

      }

      if (m_warnings.is_warning(airspace) || m_warnings.is_inside(airspace)) {
        m_stencil.black_brush();
        m_stencil.select(pen_medium);
      } else {
        m_stencil.select(pen_thick);
        m_stencil.hollow_brush();
      }

    }
  }

  const AirspaceWarningCopy& m_warnings;
  Pen pen_thick;
  Pen pen_medium;
  AirspaceMapVisible visible;
  bool m_border;
};


void
MapWindow::DrawAirspaceIntersections(Canvas &canvas)
{
  for (std::vector<GEOPOINT>::const_iterator it = m_airspace_intersections.begin();
       it != m_airspace_intersections.end(); ++it) {
    
    draw_masked_bitmap_if_visible(canvas, MapGfx.hAirspaceInterceptBitmap,
                                  *it, 10, 10);
  }
}

/**
 * Draws the airspace to the given canvas
 * @param canvas The drawing canvas
 * @param buffer The drawing buffer
 */
void
MapWindow::DrawAirspace(Canvas &canvas, Canvas &buffer)
{
  if (m_airspace == NULL)
    return;

  {
    AirspaceWarningCopy awc;
    m_airspace->visit_warnings(awc);

    MapDrawHelper helper (canvas, buffer, stencil_canvas, *this, 
                          GetMapRect(), SettingsMap());
    AirspaceVisitorMap v(helper, awc, SettingsComputer(), Basic());

    // JMW TODO wasteful to draw twice, can't it be drawn once?
    // we are using two draws so borders go on top of everything
    
    v.set_border(false);
    m_airspace->visit_within_range(PanLocation, fixed(GetScreenDistanceMeters()), v);
    v.set_border(true);
    m_airspace->visit_within_range(PanLocation, fixed(GetScreenDistanceMeters()), v);
    v.draw_intercepts();

    m_airspace_intersections = awc.get_locations();
  }
}
