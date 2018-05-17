/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

struct TaskLook;
struct AirspaceLook;
class Canvas;
class Projection;
struct AirspaceRendererSettings;
class ObservationZonePoint;

/** Utility class to render an ObzervationZonePoint to a canvas */
class OZRenderer {
public:
  enum Layer {
    /** the background shade */
    LAYER_SHADE,

    /** the inactive boundaries */
    LAYER_INACTIVE,

    /** the active boundaries */
    LAYER_ACTIVE,
  };

protected:
  const TaskLook &task_look;
  const AirspaceLook &airspace_look;
  const AirspaceRendererSettings &settings;

public:
  OZRenderer(const TaskLook &task_look, const AirspaceLook &airspace_look,
             const AirspaceRendererSettings &_settings);

  void Draw(Canvas &canvas, Layer _layer, const Projection &projection,
            const ObservationZonePoint &oz, int offset);

private:
  /**
   * Configure brush and pen on the Canvas for the current layer.
   *
   * @param offset the offset of this task point to the current task
   * point; 0 means it is the current task point, a negative value
   * means it is a "past" task point
   * @return false if nothing is to be drawn in this layer
   */
  void Prepare(Canvas &canvas, Layer layer, int offset) const;

  /**
   * Cleans up the settings after drawing has been finished.  This
   * method must be invoked if draw_style() has returned true.
   */
  void Finish(Canvas &canvas, Layer layer) const;
};

#endif
