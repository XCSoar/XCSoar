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

RenderObservationZone::RenderObservationZone(Canvas &_canvas,
                                             const Projection &_projection,
                                             const SETTINGS_MAP &_settings_map)
  :m_buffer(_canvas), m_proj(_projection),
   m_settings_map(_settings_map),
   layer(LAYER_SHADE),
   pen_boundary_current(Pen::SOLID, Layout::SmallScale(2), Graphics::TaskColor),
   pen_boundary_active(Pen::SOLID, Layout::SmallScale(1), Graphics::TaskColor),
   pen_boundary_inactive(Pen::SOLID, Layout::SmallScale(1), Color(127, 127, 127))
{
}

bool 
RenderObservationZone::draw_style(int offset)
{
  if (layer == LAYER_SHADE) {
    if (offset < 0)
      /* past task point */
      return false;

#ifdef ENABLE_OPENGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Color color = Graphics::Colours[m_settings_map.iAirspaceColour[AATASK]];
    m_buffer.select(Brush(color.with_alpha(64)));
#else /* !OPENGL */

#ifndef ENABLE_SDL
    m_buffer.mix_mask();
#endif

    // this color is used as the black bit
    m_buffer.set_text_color(Graphics::Colours[m_settings_map.
                                           iAirspaceColour[AATASK]]);
    // get brush, can be solid or a 1bpp bitmap
    m_buffer.select(Graphics::hAirspaceBrushes[m_settings_map.
                                            iAirspaceBrush[AATASK]]);
#endif /* !OPENGL */

    m_buffer.null_pen();
    
    return true;
  } else {
    m_buffer.hollow_brush();
    if (layer == LAYER_ACTIVE && offset >= 0) {
      if (offset == 0)
        /* current task point */
        m_buffer.select(pen_boundary_current);
      else
        m_buffer.select(pen_boundary_active);
    } else {
      m_buffer.select(pen_boundary_inactive); 
    }
    return true;
  }
}

void
RenderObservationZone::un_draw_style()
{
  if (layer == LAYER_SHADE) {
#ifdef ENABLE_OPENGL
    glDisable(GL_BLEND);
#elif !defined(ENABLE_SDL)
    m_buffer.mix_copy();
#endif /* GDI */
  }
}

void 
RenderObservationZone::draw_two_lines() {
  m_buffer.two_lines(p_start, p_center, p_end);
}

void 
RenderObservationZone::draw_circle() {
  m_buffer.circle(p_center.x, p_center.y, p_radius);
}

void 
RenderObservationZone::draw_segment(const Angle start_radial, 
                                    const Angle end_radial) 
{
  m_buffer.segment(p_center.x, p_center.y, p_radius,
                   start_radial-m_proj.GetScreenAngle(), 
                   end_radial-m_proj.GetScreenAngle());
}

void 
RenderObservationZone::parms_oz(const CylinderZone& oz) 
{
  p_radius = m_proj.GeoToScreenDistance(oz.getRadius());
  p_center = m_proj.GeoToScreen(oz.get_location());
}

void 
RenderObservationZone::parms_sector(const SectorZone& oz) 
{
  parms_oz(oz);
  p_start = m_proj.GeoToScreen(oz.get_SectorStart());
  p_end = m_proj.GeoToScreen(oz.get_SectorEnd());
}

void
RenderObservationZone::Draw(const ObservationZonePoint &_oz)
{
  switch (_oz.shape) {
  case ObservationZonePoint::LINE:
  case ObservationZonePoint::FAI_SECTOR: {
    const SectorZone &oz = (const SectorZone &)_oz;
    parms_sector(oz);

    if (layer != LAYER_ACTIVE)
      draw_segment(oz.getStartRadial(), oz.getEndRadial());
    else
      draw_two_lines();

    break;
  }

  case ObservationZonePoint::CYLINDER: {
    const CylinderZone &oz = (const CylinderZone &)_oz;

    if (layer != LAYER_INACTIVE) {
      parms_oz(oz);
      draw_circle();
    }

    break;
  }

  case ObservationZonePoint::SECTOR: {
    const SectorZone &oz = (const SectorZone &)_oz;

    if (layer != LAYER_INACTIVE) {
      parms_sector(oz);
      draw_segment(oz.getStartRadial(), oz.getEndRadial());
      draw_two_lines();
    }

    break;
  }

  case ObservationZonePoint::KEYHOLE:
  case ObservationZonePoint::BGAFIXEDCOURSE:
  case ObservationZonePoint::BGAENHANCEDOPTION: {
    const SectorZone &oz = (const SectorZone &)_oz;
    parms_sector(oz);

    if (layer != LAYER_ACTIVE) {
      draw_segment(oz.getStartRadial(), oz.getEndRadial());
      p_radius = m_proj.GeoToScreenDistance(fixed(500));
      draw_circle();
    } else
      draw_two_lines();

    break;
  }
  }
}
