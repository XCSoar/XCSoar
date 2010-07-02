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
#if !defined(XCSOAR_RENDER_OBSERVATION_ZONE_HPP)
#define XCSOAR_RENDER_OBSERVATION_ZONE_HPP

#include "Task/Visitors/ObservationZoneVisitor.hpp"
#include "Screen/Pen.hpp"

#include <windef.h>

class Canvas;
class Projection;
struct SETTINGS_MAP;

/**
 * Utility class to render an ObzervationZonePoint to a canvas
 */
class RenderObservationZone: 
  public ObservationZoneConstVisitor
{
protected:
  Canvas &m_buffer;
  const Projection &m_proj;
  const SETTINGS_MAP &m_settings_map;

public:
  RenderObservationZone(Canvas &_canvas, const Projection &_projection,
                        const SETTINGS_MAP &_settings_map);

  void Visit(const FAISectorZone& oz);

  void Visit(const KeyholeZone& oz);

  void Visit(const SectorZone& oz);

  void Visit(const LineSectorZone& oz);

  void Visit(const CylinderZone& oz);

  void set_past(bool set) {
    m_past = set;
  }
  void set_current(bool set) {
    m_current = set;
  }
  void set_background(bool set) {
    m_background = set;
  }
protected:
  bool draw_style(bool is_boundary_active);

  void draw_two_lines();

  void draw_circle();

  void draw_segment(const Angle start_radial, const Angle end_radial);

  void parms_oz(const CylinderZone& oz);

  void parms_sector(const SectorZone& oz);

  const Pen pen_boundary_current;
  const Pen pen_boundary_active;
  const Pen pen_boundary_inactive;
  POINT p_center, p_start, p_end;
  unsigned p_radius;
  bool m_past;
  bool m_current;
  bool m_background;
};


#endif
