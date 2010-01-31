#include "RenderObservationZone.hpp"
#include "Screen/Graphics.hpp"
#include "Projection.hpp"

RenderObservationZone::RenderObservationZone(MapDrawHelper &_draw)
  :MapDrawHelper(_draw),
   pen_boundary_current(Pen::SOLID, IBLSCALE(2), MapGfx.TaskColor),
   pen_boundary_active(Pen::SOLID, IBLSCALE(1), MapGfx.TaskColor),
   pen_boundary_inactive(Pen::SOLID, IBLSCALE(1), Color(127, 127, 127)),
   m_past(false),
   m_current(false),
   m_background(false)
{
}

bool 
RenderObservationZone::draw_style(bool is_boundary_active) 
{
  if (m_background) {
    // this color is used as the black bit
    m_buffer.set_text_color(MapGfx.Colours[m_settings_map.
                                           iAirspaceColour[AATASK]]);
    // get brush, can be solid or a 1bpp bitmap
    m_buffer.select(MapGfx.hAirspaceBrushes[m_settings_map.
                                            iAirspaceBrush[AATASK]]);
    m_buffer.white_pen();
    
    return !m_past;
  } else {
    m_buffer.hollow_brush();
    if (is_boundary_active) {
      if (m_current) {
        m_buffer.select(pen_boundary_current);
      } else {
        m_buffer.select(pen_boundary_active);
      }
    } else {
      m_buffer.select(pen_boundary_inactive); 
    }
    return true;
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
RenderObservationZone::draw_segment(const fixed start_radial, 
                                    const fixed end_radial) 
{
  m_buffer.segment(p_center.x, p_center.y, p_radius, m_rc, 
                   start_radial-m_proj.GetDisplayAngle(), 
                   end_radial-m_proj.GetDisplayAngle());
}

void 
RenderObservationZone::parms_oz(const CylinderZone& oz) 
{
  buffer_render_start();
  p_radius = m_proj.DistanceMetersToScreen(oz.getRadius());
  m_proj.LonLat2Screen(oz.get_location(), p_center);
}

void 
RenderObservationZone::parms_sector(const SectorZone& oz) 
{
  parms_oz(oz);
  m_proj.LonLat2Screen(oz.get_SectorStart(), p_start);
  m_proj.LonLat2Screen(oz.get_SectorEnd(), p_end);
}

void 
RenderObservationZone::Visit(const FAISectorZone& oz) 
{
  parms_sector(oz);
  if (draw_style(false)) {
    draw_segment(oz.getStartRadial(), oz.getEndRadial());
  }
  if (draw_style(!m_past)) {
    draw_two_lines();
  }
}

void 
RenderObservationZone::Visit(const SectorZone& oz) 
{
  parms_sector(oz);
  if (draw_style(!m_past)) {
    draw_segment(oz.getStartRadial(), oz.getEndRadial());
    draw_two_lines();
  }
}

void 
RenderObservationZone::Visit(const LineSectorZone& oz) 
{
  parms_sector(oz);
  if (draw_style(false)) {
    draw_segment(oz.getStartRadial(), oz.getEndRadial());
  }
  if (draw_style(!m_past)) {
    draw_two_lines();
  }
}

void 
RenderObservationZone::Visit(const CylinderZone& oz) 
{
  parms_oz(oz);
  if (draw_style(!m_past)) {
    draw_circle();
  }
}
