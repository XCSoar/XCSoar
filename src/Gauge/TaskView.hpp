// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
class Canvas;
class OrderedTask;
class OrderedTaskPoint;
class RasterTerrain;
class Airspaces;
class WindowProjection;
struct GeoPoint;
struct MapSettings;
struct TaskLook;
struct AirspaceLook;
struct OverlayLook;

/**
 * Draw Task with the given projection.
 *
 * @param fai_sectors render FAI triangle sectors?
 * @highlight_index highlight the task point as beeing manually edited
 */
void
PaintTask(Canvas &canvas, const WindowProjection &projection,
          const OrderedTask &task,
          const GeoPoint &location,
          const MapSettings &settings_map,
          const TaskLook &task_look,
          const AirspaceLook &airspace_look,
          const RasterTerrain *terrain, const Airspaces *airspaces,
          bool fai_sectors,
          int highlight_index);

/**
 * Draw the whole Task into a rectangle.
 *
 * @param fai_sectors render FAI triangle sectors?
 * @highlight_index highlight the task point as beeing manually edited
 */
void
PaintTask(Canvas &canvas, const PixelRect &rc, const OrderedTask &task,
          const GeoPoint &location,
          const MapSettings &settings_map,
          const TaskLook &task_look,
          const AirspaceLook &airspace_look,
          const OverlayLook &overlay_look,
          const RasterTerrain *terrain, const Airspaces *airspaces,
          bool fai_sectors=false,
          int highlight_index = -1);

/**
 * Draw a detailed view of a TaskPoint into a rectangle.
 * @highlight_index highlight the task point as beeing manually edited
 */
void
PaintTaskPoint(Canvas &canvas, const PixelRect &rc,
               const OrderedTask &task, const OrderedTaskPoint &point,
               const GeoPoint &location,
               const MapSettings &settings_map,
               const TaskLook &task_look,
               const AirspaceLook &airspace_look,
               const OverlayLook &overlay_look,
               const RasterTerrain *terrain, const Airspaces *airspaces,
               int highlight_index = -1);
