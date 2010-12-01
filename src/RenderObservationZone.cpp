#include "RenderObservationZone.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/FAISectorZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/ObservationZones/BGAFixedCourseZone.hpp"
#include "Task/ObservationZones/BGAEnhancedOptionZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Screen/Graphics.hpp"
#include "WindowProjection.hpp"
#include "SettingsMap.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"

RenderObservationZone::RenderObservationZone()
  :layer(LAYER_SHADE),
   pen_boundary_current(Pen::SOLID, Layout::SmallScale(2), Graphics::TaskColor),
   pen_boundary_active(Pen::SOLID, Layout::SmallScale(1), Graphics::TaskColor),
   pen_boundary_inactive(Pen::SOLID, Layout::SmallScale(1), Color(127, 127, 127))
{
}

bool 
RenderObservationZone::draw_style(Canvas &canvas,
                                  const SETTINGS_MAP &settings_map,
                                  int offset) const
{
  if (layer == LAYER_SHADE) {
    if (offset < 0)
      /* past task point */
      return false;

#ifdef ENABLE_OPENGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Color color = Graphics::Colours[settings_map.iAirspaceColour[AATASK]];
    canvas.select(Brush(color.with_alpha(64)));
#else /* !OPENGL */

#ifndef ENABLE_SDL
    canvas.mix_mask();
#endif

    // this color is used as the black bit
    canvas.set_text_color(Graphics::Colours[settings_map.
                                           iAirspaceColour[AATASK]]);
    // get brush, can be solid or a 1bpp bitmap
    canvas.select(Graphics::hAirspaceBrushes[settings_map.
                                            iAirspaceBrush[AATASK]]);
#endif /* !OPENGL */

    canvas.null_pen();
    
    return true;
  } else {
    canvas.hollow_brush();
    if (layer == LAYER_ACTIVE && offset >= 0) {
      if (offset == 0)
        /* current task point */
        canvas.select(pen_boundary_current);
      else
        canvas.select(pen_boundary_active);
    } else {
      canvas.select(pen_boundary_inactive);
    }
    return true;
  }
}

void
RenderObservationZone::un_draw_style(Canvas &canvas) const
{
  if (layer == LAYER_SHADE) {
#ifdef ENABLE_OPENGL
    glDisable(GL_BLEND);
#elif !defined(ENABLE_SDL)
    canvas.mix_copy();
#endif /* GDI */
  }
}

void
RenderObservationZone::Draw(Canvas &canvas, const Projection &projection,
                            const ObservationZonePoint &_oz) const
{
  switch (_oz.shape) {
  case ObservationZonePoint::LINE:
  case ObservationZonePoint::FAI_SECTOR: {
    const SectorZone &oz = (const SectorZone &)_oz;

    RasterPoint p_center = projection.GeoToScreen(oz.get_location());
    if (layer != LAYER_ACTIVE)
      canvas.segment(p_center.x, p_center.y,
                     projection.GeoToScreenDistance(oz.getRadius()),
                     oz.getStartRadial() - projection.GetScreenAngle(),
                     oz.getEndRadial() - projection.GetScreenAngle());
    else {
      RasterPoint p_start = projection.GeoToScreen(oz.get_SectorStart());
      RasterPoint p_end = projection.GeoToScreen(oz.get_SectorEnd());

      canvas.two_lines(p_start, p_center, p_end);
    }

    break;
  }

  case ObservationZonePoint::CYLINDER: {
    const CylinderZone &oz = (const CylinderZone &)_oz;

    if (layer != LAYER_INACTIVE) {
      RasterPoint p_center = projection.GeoToScreen(oz.get_location());
      canvas.circle(p_center.x, p_center.y,
                    projection.GeoToScreenDistance(oz.getRadius()));
    }

    break;
  }

  case ObservationZonePoint::SECTOR: {
    const SectorZone &oz = (const SectorZone &)_oz;

    if (layer != LAYER_INACTIVE) {
      RasterPoint p_center = projection.GeoToScreen(oz.get_location());

      canvas.segment(p_center.x, p_center.y,
                     projection.GeoToScreenDistance(oz.getRadius()),
                     oz.getStartRadial() - projection.GetScreenAngle(),
                     oz.getEndRadial() - projection.GetScreenAngle());

      RasterPoint p_start = projection.GeoToScreen(oz.get_SectorStart());
      RasterPoint p_end = projection.GeoToScreen(oz.get_SectorEnd());
      canvas.two_lines(p_start, p_center, p_end);
    }

    break;
  }

  case ObservationZonePoint::KEYHOLE:
  case ObservationZonePoint::BGAFIXEDCOURSE:
  case ObservationZonePoint::BGAENHANCEDOPTION: {
    const SectorZone &oz = (const SectorZone &)_oz;

    RasterPoint p_center = projection.GeoToScreen(oz.get_location());
    if (layer != LAYER_ACTIVE) {
      canvas.segment(p_center.x, p_center.y,
                     projection.GeoToScreenDistance(oz.getRadius()),
                     oz.getStartRadial() - projection.GetScreenAngle(),
                     oz.getEndRadial() - projection.GetScreenAngle());
      canvas.circle(p_center.x, p_center.y,
                    projection.GeoToScreenDistance(fixed(500)));
    } else {
      RasterPoint p_start = projection.GeoToScreen(oz.get_SectorStart());
      RasterPoint p_end = projection.GeoToScreen(oz.get_SectorEnd());
      canvas.two_lines(p_start, p_center, p_end);
    }

    break;
  }
  }
}
