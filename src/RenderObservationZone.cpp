#include "RenderObservationZone.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/FAISectorZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/ObservationZones/BGAFixedCourseZone.hpp"
#include "Task/ObservationZones/BGAEnhancedOptionZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Screen/Graphics.hpp"
#include "Projection.hpp"
#include "SettingsMap.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"

RenderObservationZone::RenderObservationZone(Canvas &_canvas,
                                             const Projection &_projection,
                                             const SETTINGS_MAP &_settings_map)
  :m_buffer(_canvas), m_proj(_projection),
   m_settings_map(_settings_map),
   pen_boundary_current(Pen::SOLID, IBLSCALE(2), Graphics::TaskColor),
   pen_boundary_active(Pen::SOLID, IBLSCALE(1), Graphics::TaskColor),
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
    m_buffer.mix_mask();

    // this color is used as the black bit
    m_buffer.set_text_color(Graphics::Colours[m_settings_map.
                                           iAirspaceColour[AATASK]]);
    // get brush, can be solid or a 1bpp bitmap
    m_buffer.select(Graphics::hAirspaceBrushes[m_settings_map.
                                            iAirspaceBrush[AATASK]]);
    m_buffer.white_pen();
    
    return !m_past;
  } else {
    m_buffer.mix_copy();

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
RenderObservationZone::draw_segment(const Angle start_radial, 
                                    const Angle end_radial) 
{
  m_buffer.segment(p_center.x, p_center.y, p_radius,
                   start_radial-m_proj.GetDisplayAngle(), 
                   end_radial-m_proj.GetDisplayAngle());
}

void 
RenderObservationZone::parms_oz(const CylinderZone& oz) 
{
  p_radius = m_proj.DistanceMetersToScreen(oz.getRadius());
  p_center = m_proj.LonLat2Screen(oz.get_location());
}

void 
RenderObservationZone::parms_sector(const SectorZone& oz) 
{
  parms_oz(oz);
  p_start = m_proj.LonLat2Screen(oz.get_SectorStart());
  p_end = m_proj.LonLat2Screen(oz.get_SectorEnd());
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

  m_buffer.mix_copy();
}

void 
RenderObservationZone::Visit(const KeyholeZone& oz) 
{
  parms_sector(oz);
  if (draw_style(false)) {
    draw_segment(oz.getStartRadial(), oz.getEndRadial());
    p_radius = m_proj.DistanceMetersToScreen(fixed(500));
    draw_circle();
  }
  if (draw_style(!m_past)) {
    draw_two_lines();
  }

  m_buffer.mix_copy();
}

void 
RenderObservationZone::Visit(const BGAFixedCourseZone& oz) 
{
  parms_sector(oz);
  if (draw_style(false)) {
    draw_segment(oz.getStartRadial(), oz.getEndRadial());
    p_radius = m_proj.DistanceMetersToScreen(fixed(500));
    draw_circle();
  }
  if (draw_style(!m_past)) {
    draw_two_lines();
  }

  m_buffer.mix_copy();
}

void 
RenderObservationZone::Visit(const BGAEnhancedOptionZone& oz) 
{
  parms_sector(oz);
  if (draw_style(false)) {
    draw_segment(oz.getStartRadial(), oz.getEndRadial());
    p_radius = m_proj.DistanceMetersToScreen(fixed(500));
    draw_circle();
  }
  if (draw_style(!m_past)) {
    draw_two_lines();
  }

  m_buffer.mix_copy();
}

void 
RenderObservationZone::Visit(const SectorZone& oz) 
{
  parms_sector(oz);
  if (draw_style(!m_past)) {
    draw_segment(oz.getStartRadial(), oz.getEndRadial());
    draw_two_lines();
  }

  m_buffer.mix_copy();
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

  m_buffer.mix_copy();
}

void 
RenderObservationZone::Visit(const CylinderZone& oz) 
{
  parms_oz(oz);
  if (draw_style(!m_past)) {
    draw_circle();
  }

  m_buffer.mix_copy();
}
