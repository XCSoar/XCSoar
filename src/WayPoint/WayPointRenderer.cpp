/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "WayPointRenderer.hpp"
#include "MapWindowProjection.hpp"
#include "MapWindowLabels.hpp"
#include "SettingsMap.hpp"
#include "SettingsComputer.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/WaypointVisitor.hpp"
#include "Engine/Navigation/Aircraft.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/Task/Tasks/OrderedTask.hpp"
#include "Engine/Task/Tasks/AbortTask.hpp"
#include "Engine/Task/Tasks/GotoTask.hpp"
#include "Engine/Task/Tasks/BaseTask/UnorderedTaskPoint.hpp"
#include "Engine/Task/Tasks/TaskSolvers/TaskSolution.hpp"
#include "Engine/Task/TaskPoints/AATPoint.hpp"
#include "Engine/Task/TaskPoints/ASTPoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Canvas.hpp"
#include "Appearance.hpp"
#include "Units.hpp"
#include "Screen/Layout.hpp"
#include "Terrain/RasterTerrain.hpp"

#include <stdio.h>

class WaypointVisitorMap: 
  public WaypointVisitor, 
  public TaskPointConstVisitor
{
  const MapWindowProjection &projection;
  const SETTINGS_MAP &settings_map;
  const TaskBehaviour &task_behaviour;

public:
  WaypointVisitorMap(const MapWindowProjection &_projection,
                     const SETTINGS_MAP &_settings_map,
                     const TaskBehaviour &_task_behaviour,
                     const AIRCRAFT_STATE &_aircraft_state, Canvas &_canvas,
                     const GlidePolar &polar,
                     const RoutePolars& _rpolars,
                     RasterTerrain* _terrain):
    projection(_projection),
    settings_map(_settings_map), task_behaviour(_task_behaviour),
    aircraft_state(_aircraft_state),
    p_start (aircraft_state.get_location(), aircraft_state.NavAltitude),
    canvas(_canvas),
    glide_polar(polar),
    task_valid(false),
    rpolars(_rpolars),
    terrain(_terrain),
    labels(projection.GetScreenWidth(), projection.GetScreenHeight())
  {
    // if pan mode, show full names
    pDisplayTextType = settings_map.DisplayTextType;
    if (settings_map.EnablePan)
      pDisplayTextType = DISPLAYNAME;

    _tcscpy(sAltUnit, Units::GetAltitudeName());

    proj.reset(aircraft_state.get_location());
    proj.update_fast();
  }

  void
  FormatTitle(TCHAR* Buffer, const Waypoint &way_point)
  {
    Buffer[0] = _T('\0');

    switch (pDisplayTextType) {
    case DISPLAYNAME:
      _tcscpy(Buffer, way_point.Name.c_str());
      break;

    case DISPLAYFIRSTFIVE:
      _tcsncpy(Buffer, way_point.Name.c_str(), 5);
      Buffer[5] = '\0';
      break;

    case DISPLAYFIRSTTHREE:
      _tcsncpy(Buffer, way_point.Name.c_str(), 3);
      Buffer[3] = '\0';
      break;

    case DISPLAYNONE:
      Buffer[0] = '\0';
      break;

    case DISPLAYUNTILSPACE:
      _tcscpy(Buffer, way_point.Name.c_str());
      TCHAR *tmp;
      tmp = _tcsstr(Buffer, _T(" "));
      if (tmp != NULL)
        tmp[0] = '\0';
      break;

    default:
      assert(0);
      break;
    }
  }

  void
  DrawWaypoint(const Waypoint& way_point, bool in_task = false)
  {
    RasterPoint sc;
    if (!projection.GeoToScreenIfVisible(way_point.Location, sc))
      return;

    if (!projection.WaypointInScaleFilter(way_point) && !in_task)
      return;

    bool reachable_glide = false;
    bool reachable_terrain = (terrain == NULL);

    int AltArrivalAGL = 0;

    if (way_point.is_landable()) {
      const UnorderedTaskPoint t(way_point, task_behaviour);
      const GlideResult r =
        TaskSolution::glide_solution_remaining(t, aircraft_state, glide_polar);

      if (r.glide_reachable()) {

        reachable_glide = true;
        // reachable according to height, now check terrain intersection

        if (terrain) {
          RasterTerrain::ExclusiveLease rlease(*terrain);
          const AGeoPoint p_dest (t.get_location(), r.MinHeight);
          GeoPoint p_intx = p_dest;

          if (!rpolars.intersection(p_start, p_dest, &terrain->map, proj, p_intx))
            reachable_terrain = true;
        }

        AltArrivalAGL = (int)Units::ToUserAltitude(r.AltitudeDifference);
      }

      WayPointRenderer::DrawLandableSymbol(canvas, sc, reachable_glide,
                                           reachable_terrain, way_point,
                                           projection.GetScreenAngle());
    } else {
      // non landable turnpoint
      const MaskedIcon *icon;
      if (projection.GetMapScale() > fixed(4000))
        icon = &Graphics::SmallIcon;
      else {
        switch (way_point.Type) {
        case wtMountainTop:
          icon = &Graphics::MountainTopIcon;
          break;
        case wtBridge:
          icon = &Graphics::BridgeIcon;
          break;
        case wtTunnel:
          icon = &Graphics::TunnelIcon;
          break;
        case wtTower:
          icon = &Graphics::TowerIcon;
          break;
        case wtPowerPlant:
          icon = &Graphics::PowerPlantIcon;
          break;
        default:
          icon = &Graphics::TurnPointIcon;
          break;
        }
      }
      icon->draw(canvas, sc);
    }

    int pWayPointLabelSelection = settings_map.WayPointLabelSelection;
    if (pWayPointLabelSelection == wlsNoWayPoints)
      return;
    if (!in_task && task_valid) {
      if (pWayPointLabelSelection == wlsTaskWayPoints)
        return;
      if (pWayPointLabelSelection == wlsTaskAndLandableWayPoints &&
          !way_point.is_landable())
        return;
    }

    TextInBoxMode_t text_mode;
    if (reachable_glide && way_point.is_landable()) {
      text_mode.Mode = settings_map.LandableRenderMode;
      text_mode.Bold = true;
    } else if (in_task) {
      text_mode.Mode = OutlinedInverted;
      text_mode.Bold = true;
    }

    TCHAR Buffer[32];
    FormatTitle(Buffer, way_point);

    if (AltArrivalAGL != 0) {
      size_t length = _tcslen(Buffer);
      if (length > 0)
        Buffer[length++] = _T(':');

      _stprintf(Buffer + length, _T("%d%s"), AltArrivalAGL, sAltUnit);
    }

    if (reachable_glide && (Appearance.IndLandable == wpLandableWinPilot ||
                            Appearance.UseSWLandablesRendering))
      // make space for the green circle
      sc.x += 5;

    labels.Add(Buffer, sc.x + 5, sc.y, text_mode, AltArrivalAGL,
               in_task, way_point.is_landable(), way_point.is_airport());
  }

  void
  Visit(const Waypoint& way_point)
  {
    DrawWaypoint(way_point, false);
  }

  void
  Visit(const UnorderedTaskPoint& tp)
  {
    DrawWaypoint(tp.get_waypoint(), true);
  }

  void
  Visit(const StartPoint& tp)
  {
    DrawWaypoint(tp.get_waypoint(), true);
  }

  void
  Visit(const FinishPoint& tp)
  {
    DrawWaypoint(tp.get_waypoint(), true);
  }

  void
  Visit(const AATPoint& tp)
  {
    DrawWaypoint(tp.get_waypoint(), true);
  }

  void
  Visit(const ASTPoint& tp)
  {
    DrawWaypoint(tp.get_waypoint(), true);
  }

private:
  const AIRCRAFT_STATE aircraft_state;
  const AGeoPoint p_start;
  Canvas &canvas;
  int pDisplayTextType;
  TCHAR sAltUnit[4];
  const GlidePolar glide_polar;
  bool task_valid;
  TaskProjection proj;
  const RoutePolars& rpolars;
  RasterTerrain* terrain;

public:
  void set_task_valid() {
    task_valid = true;
  }
  WayPointLabelList labels;
};

static void
MapWaypointLabelRender(Canvas &canvas, unsigned width, unsigned height,
                       LabelBlock &label_block,
                       WayPointLabelList &labels)
{
  labels.Sort();

  for (unsigned i = 0; i < labels.size(); i++) {
    const WayPointLabelList::Label *E = &labels[i];
    TextInBox(canvas, E->Name, E->Pos.x, E->Pos.y, E->Mode,
              width, height, &label_block);
  }
}

void
WayPointRenderer::render(Canvas &canvas, LabelBlock &label_block,
                         const MapWindowProjection &projection,
                         const SETTINGS_MAP &settings_map,
                         const TaskBehaviour &task_behaviour,
                         const GlidePolar &glide_polar,
                         const AIRCRAFT_STATE &aircraft_state,
                         const ProtectedTaskManager *task,
                         RasterTerrain* terrain)
{
  if ((way_points == NULL) || way_points->empty())
    return;

  if (task) {

    canvas.set_text_color(Color::BLACK);

    ProtectedTaskManager::Lease task_manager(*task);
    
    WaypointVisitorMap v(projection, settings_map, task_behaviour,
                         aircraft_state,
                         canvas, glide_polar,
                         task_manager->get_route_polars_safety(), 
                         terrain);
    
    // task items come first, this is the only way we know that an item is in task,
    // and we won't add it if it is already there
    if (task_manager->stats_valid()) {
      v.set_task_valid();
    }

    const AbstractTask *atask = task_manager->get_active_task();
    if (atask != NULL)
      atask->tp_CAccept(v);

    way_points->visit_within_range(projection.GetGeoScreenCenter(),
                                   projection.GetScreenDistanceMeters(), v);

    MapWaypointLabelRender(canvas,
                           projection.GetScreenWidth(),
                           projection.GetScreenHeight(),
                           label_block, v.labels);
  }
}

static void
DrawLandableBase(Canvas &canvas, const RasterPoint& pt,
                             bool airport, const fixed radius)
{
  int iradius = iround(radius);
  if (airport)
    canvas.circle(pt.x, pt.y, iradius);
  else {
    RasterPoint diamond[4];
    diamond[0].x = pt.x + 0;
    diamond[0].y = pt.y - iradius;
    diamond[1].x = pt.x + iradius;
    diamond[1].y = pt.y + 0;
    diamond[2].x = pt.x + 0;
    diamond[2].y = pt.y + iradius;
    diamond[3].x = pt.x - iradius;
    diamond[3].y = pt.y - 0;
    canvas.polygon(diamond, sizeof(diamond)/sizeof(diamond[0]));
  }
}

static void
DrawLandableRunway(Canvas &canvas, const RasterPoint &pt,
                   const Angle &a, fixed radius, fixed width)
{
  if (radius <= fixed_zero)
    return;

  fixed fwx, fwy;
  (a + Angle::degrees(fixed_int_constant(90))).sin_cos(fwx, fwy);
  int wx = iround(fwx * width);
  int wy = iround(fwy * width);

  fixed x, y;
  a.sin_cos(x, y);
  int lx = iround(x * radius * fixed_two) & ~0x1;  // make it a even number
  int ly = iround(y * radius * fixed_two) & ~0x1;

  RasterPoint runway[4];
  runway[0].x = pt.x        - (lx / 2) + (wx / 2);
  runway[0].y = pt.y        + (ly / 2) - (wy / 2);
  runway[1].x = runway[0].x            - wx;
  runway[1].y = runway[0].y            + wy;
  runway[2].x = runway[1].x + lx;
  runway[2].y = runway[1].y - ly;
  runway[3].x = runway[2].x            + wx;
  runway[3].y = runway[2].y            - wy;
  canvas.polygon(runway, sizeof(runway)/sizeof(runway[0]));
}

void
WayPointRenderer::DrawLandableSymbol(Canvas &canvas, const RasterPoint &pt,
                                     bool reachable_glide,
                                     bool reachable_terrain,
                                     const Waypoint &way_point,
                                     const Angle &screenRotation)
{
  static const Color NOT_REACHABLE_TERRAIN(224, 64, 64);

  if (!Appearance.UseSWLandablesRendering) {
    const MaskedIcon *icon;

    if (reachable_glide && reachable_terrain)
      icon = way_point.is_airport() ? &Graphics::AirportReachableIcon :
                                      &Graphics::FieldReachableIcon;
    else
      icon = way_point.is_airport() ? &Graphics::AirportUnreachableIcon :
                                      &Graphics::FieldUnreachableIcon;

    icon->draw(canvas, pt);
    return;
  }

  // SW rendering of landables
  fixed scale = fixed(Layout::SmallScale(Appearance.LandableRenderingScale)) /
                fixed_int_constant(150);
  fixed radius = fixed_int_constant(10) * scale;
  Brush fill;

  canvas.black_pen();
  if (Appearance.IndLandable == wpLandableWinPilot) {
    // Render landable with reachable state
    if (reachable_glide) {
      fill.set(reachable_terrain ? Color::GREEN : NOT_REACHABLE_TERRAIN);
      canvas.select(fill);
      DrawLandableBase(canvas, pt, way_point.is_airport(),
                       radius + radius / fixed_two);
    }
    fill.set(Color::MAGENTA);
  } else if (Appearance.IndLandable == wpLandableAltB) {
    if (reachable_glide)
      fill.set(reachable_terrain ? Color::GREEN : NOT_REACHABLE_TERRAIN);
    else
      fill.set(Color::ORANGE);
  } else {
    if (reachable_glide)
      fill.set(reachable_terrain ? Color::GREEN : NOT_REACHABLE_TERRAIN);
    else if (way_point.is_airport())
      fill.set(Color::WHITE);
    else
      fill.set(Color::LIGHT_GRAY);
  }
  canvas.select(fill);
  DrawLandableBase(canvas, pt, way_point.is_airport(), radius);

  // Render runway indication
  if (way_point.RunwayDirection.value_degrees() >= fixed_zero) {
    fixed len;
    if (Appearance.ScaleRunwayLength && way_point.RunwayLength > 0)
      len = (radius / fixed_two) +
            ((way_point.RunwayLength - 500) / 500) * (radius / fixed_four);
    else
      len = radius;
    len += fixed_two * scale;
    Angle runwayDrawingAngle = way_point.RunwayDirection - screenRotation;
    canvas.white_brush();
    DrawLandableRunway(canvas, pt, runwayDrawingAngle, len,
                       fixed_int_constant(5) * scale);
  }
}

