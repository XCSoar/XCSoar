/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "OZRenderer.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/ObservationZones/AnnularSectorZone.hpp"
#include "Task/ObservationZones/Boundary.hpp"
#include "Projection/Projection.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/SimplifiedMath.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/TaskLook.hpp"
#include "Look/AirspaceLook.hpp"

#ifdef USE_GDI
#include "AirspaceRendererSettings.hpp"
#endif

OZRenderer::OZRenderer(const TaskLook &_task_look,
                       const AirspaceLook &_airspace_look,
                       const AirspaceRendererSettings &_settings) noexcept
  :task_look(_task_look), airspace_look(_airspace_look), settings(_settings)
{
}

void
OZRenderer::Prepare(Canvas &canvas, Layer layer, int offset) const noexcept
{
  if (layer == LAYER_SHADE) {
    Color color = airspace_look.classes[AATASK].fill_color;
#ifdef ENABLE_OPENGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    canvas.Select(Brush(color.WithAlpha(64)));
#elif defined(USE_GDI)
    canvas.SetMixMask();

    // this color is used as the black bit
    canvas.SetTextColor(color);
    // get brush, can be solid or a 1bpp bitmap
    canvas.Select(airspace_look.brushes[settings.classes[AATASK].brush]);
#else /* !GDI */
    canvas.Select(Brush(color.WithAlpha(64)));
#endif /* !GDI */

    canvas.SelectNullPen();
    return;
  }

  canvas.SelectHollowBrush();

  if (layer != LAYER_ACTIVE || offset < 0)
    canvas.Select(task_look.oz_inactive_pen);
  else if (offset == 0)
    /* current task point */
    canvas.Select(task_look.oz_current_pen);
  else
    canvas.Select(task_look.oz_active_pen);
}

void
OZRenderer::Finish(Canvas &canvas, Layer layer) const noexcept
{
  if (layer == LAYER_SHADE) {
#ifdef ENABLE_OPENGL
    glDisable(GL_BLEND);
#elif defined(USE_GDI)
    canvas.SetMixCopy();
#endif /* GDI */
  }
}

void
OZRenderer::Draw(Canvas &canvas, Layer layer, const Projection &projection,
                 const ObservationZonePoint &_oz, int offset) noexcept
{
  if (layer == LAYER_SHADE && offset < 0)
    return;

  Prepare(canvas, layer, offset);

  switch (_oz.GetShape()) {
  case ObservationZone::Shape::LINE:
  case ObservationZone::Shape::FAI_SECTOR: {
    const SectorZone &oz = (const SectorZone &)_oz;

    auto p_center = projection.GeoToScreen(oz.GetReference());
    if (layer != LAYER_ACTIVE)
      canvas.DrawSegment(p_center,
                         projection.GeoToScreenDistance(oz.GetRadius()),
                         oz.GetStartRadial() - projection.GetScreenAngle(),
                         oz.GetEndRadial() - projection.GetScreenAngle());
    else {
      auto p_start = projection.GeoToScreen(oz.GetSectorStart());
      auto p_end = projection.GeoToScreen(oz.GetSectorEnd());

      canvas.DrawTwoLines(p_start, p_center, p_end);
    }

    break;
  }

  case ObservationZone::Shape::MAT_CYLINDER:
  case ObservationZone::Shape::CYLINDER: {
    const CylinderZone &oz = (const CylinderZone &)_oz;

    if (layer != LAYER_INACTIVE) {
      auto p_center = projection.GeoToScreen(oz.GetReference());
      canvas.DrawCircle(p_center,
                    projection.GeoToScreenDistance(oz.GetRadius()));
    }

    break;
  }

  case ObservationZone::Shape::BGA_START:
  case ObservationZone::Shape::SYMMETRIC_QUADRANT:
  case ObservationZone::Shape::SECTOR: {
    const SectorZone &oz = (const SectorZone &)_oz;

    if (layer != LAYER_INACTIVE) {
      auto p_center = projection.GeoToScreen(oz.GetReference());

      canvas.DrawSegment(p_center,
                         projection.GeoToScreenDistance(oz.GetRadius()),
                         oz.GetStartRadial() - projection.GetScreenAngle(),
                         oz.GetEndRadial() - projection.GetScreenAngle());

      auto p_start = projection.GeoToScreen(oz.GetSectorStart());
      auto p_end = projection.GeoToScreen(oz.GetSectorEnd());
      canvas.DrawTwoLines(p_start, p_center, p_end);
    }

    break;
  }

  case ObservationZone::Shape::CUSTOM_KEYHOLE:
  case ObservationZone::Shape::DAEC_KEYHOLE:
  case ObservationZone::Shape::BGAFIXEDCOURSE:
  case ObservationZone::Shape::BGAENHANCEDOPTION: {
    const KeyholeZone &oz = (const KeyholeZone &)_oz;
    auto p_center = projection.GeoToScreen(oz.GetReference());
    canvas.DrawKeyhole(p_center,
                       projection.GeoToScreenDistance(oz.GetInnerRadius()),
                       projection.GeoToScreenDistance(oz.GetRadius()),
                       oz.GetStartRadial() - projection.GetScreenAngle(),
                       oz.GetEndRadial() - projection.GetScreenAngle());

    break;
  }

  case ObservationZone::Shape::ANNULAR_SECTOR: {
    const AnnularSectorZone &oz = (const AnnularSectorZone &)_oz;
    auto p_center = projection.GeoToScreen(oz.GetReference());
    canvas.DrawAnnulus(p_center,
                       projection.GeoToScreenDistance(oz.GetInnerRadius()),
                       projection.GeoToScreenDistance(oz.GetRadius()),
                       oz.GetStartRadial() - projection.GetScreenAngle(),
                       oz.GetEndRadial() - projection.GetScreenAngle());
  }

  }

  Finish(canvas, layer);
}

static void
AddSegment(GeoBounds &bounds, GeoPoint center, double radius,
           AngleRange range) noexcept
{
  /* for what we're doing, only the for extreme points of the segment
     are relevant; if those angles are inside the segment, we may need
     to add them to avoid clipping */
  for (Angle a : {Angle::Zero(), Angle::QuarterCircle(), Angle::HalfCircle(), Angle::QuarterCircle() + Angle::HalfCircle()})
    if (range.IsInside(a))
      bounds.Extend(FindLatitudeLongitudeS(center, a, radius));
}

GeoBounds
OZRenderer::GetGeoBounds(const ObservationZonePoint &oz) noexcept
{
  auto bounds = GeoBounds::Invalid();
  for (const auto &i : oz.GetBoundary())
    bounds.Extend(i);

  switch (oz.GetShape()) {
  case ObservationZone::Shape::LINE:
  case ObservationZone::Shape::FAI_SECTOR:
    {
      /* for these, the segment is not part of the boundary, but it is
         rendered */
      const SectorZone &sector = (const SectorZone &)oz;
      AddSegment(bounds, sector.GetReference(), sector.GetRadius(),
                 {sector.GetStartRadial(), sector.GetEndRadial()});
    }
    break;

  case ObservationZone::Shape::MAT_CYLINDER:
  case ObservationZone::Shape::CYLINDER:
  case ObservationZone::Shape::BGA_START:
  case ObservationZone::Shape::SYMMETRIC_QUADRANT:
  case ObservationZone::Shape::SECTOR:
  case ObservationZone::Shape::CUSTOM_KEYHOLE:
  case ObservationZone::Shape::DAEC_KEYHOLE:
  case ObservationZone::Shape::BGAFIXEDCOURSE:
  case ObservationZone::Shape::BGAENHANCEDOPTION:
  case ObservationZone::Shape::ANNULAR_SECTOR:
    break;
  }

  return bounds;
}
