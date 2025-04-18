// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct TaskLook;
struct AirspaceLook;
class Canvas;
class Projection;
struct AirspaceRendererSettings;
class ObservationZonePoint;
class GeoBounds;

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
             const AirspaceRendererSettings &_settings) noexcept;

  void Draw(Canvas &canvas, Layer _layer, const Projection &projection,
            const ObservationZonePoint &oz, int offset) noexcept;

  [[gnu::pure]]
  static GeoBounds GetGeoBounds(const ObservationZonePoint &oz) noexcept;

private:
  /**
   * Configure brush and pen on the Canvas for the current layer.
   *
   * @param offset the offset of this task point to the current task
   * point; 0 means it is the current task point, a negative value
   * means it is a "past" task point
   * @return false if nothing is to be drawn in this layer
   */
  void Prepare(Canvas &canvas, Layer layer, int offset) const noexcept;

  /**
   * Cleans up the settings after drawing has been finished.  This
   * method must be invoked if draw_style() has returned true.
   */
  void Finish(Canvas &canvas, Layer layer) const noexcept;
};
