/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#ifndef XCSOAR_RENDER_OBSERVATION_ZONE_HPP
#define XCSOAR_RENDER_OBSERVATION_ZONE_HPP

#include "Task/Visitors/ObservationZoneVisitor.hpp"
#include "Math/Angle.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Point.hpp"

class Canvas;
class Projection;
struct SETTINGS_MAP;

/**
 * Utility class to render an ObzervationZonePoint to a canvas
 */
class RenderObservationZone: 
  public ObservationZoneConstVisitor
{
public:
  enum layer {
    /** the background shade */
    LAYER_SHADE,

    /** the inactive boundaries */
    LAYER_INACTIVE,

    /** the active boundaries */
    LAYER_ACTIVE,
  };

protected:
  Canvas &m_buffer;
  const Projection &m_proj;
  const SETTINGS_MAP &m_settings_map;

  enum layer layer;

public:
  RenderObservationZone(Canvas &_canvas, const Projection &_projection,
                        const SETTINGS_MAP &_settings_map);

  void Visit(const FAISectorZone& oz);

  void Visit(const KeyholeZone& oz);

  void Visit(const BGAFixedCourseZone& oz);

  void Visit(const BGAEnhancedOptionZone& oz);

  void Visit(const SectorZone& oz);

  void Visit(const LineSectorZone& oz);

  void Visit(const CylinderZone& oz);

  void set_layer(enum layer _layer) {
    layer = _layer;
  }

  /**
   * Configure brush and pen on the Canvas for the current layer.
   *
   * @param offset the offset of this task point to the current task
   * point; 0 means it is the current task point, a negative value
   * means it is a "past" task point
   * @return false if nothing is to be drawn in this layer
   */
  bool draw_style(int offset);

  /**
   * Cleans up the settings after drawing has been finished.  This
   * method must be invoked if draw_style() has returned true.
   */
  void un_draw_style();

protected:
  void draw_two_lines();

  void draw_circle();

  void draw_segment(const Angle start_radial, const Angle end_radial);

  void parms_oz(const CylinderZone& oz);

  void parms_sector(const SectorZone& oz);

  const Pen pen_boundary_current;
  const Pen pen_boundary_active;
  const Pen pen_boundary_inactive;
  RasterPoint p_center, p_start, p_end;
  unsigned p_radius;
};

#endif
