#include "MapDrawHelper.hpp"
#include "MapWindow.h"

MapDrawHelper::MapDrawHelper(Canvas &_canvas, 
                             Canvas &_buffer, 
                             Canvas &_stencil, 
                             MapWindow &_map,
                             const RECT &_rc):
  m_canvas(_canvas),
  m_buffer(_buffer),
  m_stencil(_stencil),
  m_map(_map),
  m_rc(_rc),
  m_buffer_drawn(false),
  m_use_stencil(false) 
{
}

MapDrawHelper::MapDrawHelper(MapDrawHelper &_that):
  m_canvas(_that.m_canvas),
  m_buffer(_that.m_buffer),
  m_stencil(_that.m_stencil),
  m_map(_that.m_map),
  m_rc(_that.m_rc),
  m_buffer_drawn(_that.m_buffer_drawn),
  m_use_stencil(_that.m_use_stencil) 
{
}

MapDrawHelper::~MapDrawHelper() {
  buffer_render_finish();
}

void 
MapDrawHelper::add(std::vector<POINT>& screen, const GEOPOINT& pt) const
{
  POINT sc;
  m_map.LonLat2Screen(pt, sc);
  screen.push_back(sc);
}

bool 
MapDrawHelper::add_if_visible(std::vector<POINT>& screen, const GEOPOINT& pt) const
{
  if (!m_map.LonLatVisible(pt)) {
    add(screen, pt);
    return true;
  } else {
    return false;
  }
}

void 
MapDrawHelper::draw_great_circle(Canvas& the_canvas, const GEOPOINT &from,
                                 const GEOPOINT &to) 
{
  const fixed distance = from.distance(to);
  if (!positive(distance)) {
    return;
  }
/*
  const fixed sc_distance = min(m_map.GetScreenDistanceMeters()/2, distance/4);
  std::vector<POINT> screen;
  bool visible = add_if_visible(screen, from);
  GEOPOINT p_last = from;
  for (fixed t=sc_distance; t<= distance; t+= sc_distance) {
    const GEOPOINT p =  from.interpolate(to, t/distance); 
//          from.intermediate_point(to, t);
    
    if (m_map.LonLatVisible(p)) {
      if (!visible) {
        add(screen, p_last);
      }
      add(screen, p);
      visible = true;
    } else {
      if (visible) {
        add(screen, p);
      } 
      visible = false;
    }
    p_last = p;
  }
  if (screen.size()>1) {
    the_canvas.polygon(&screen[0], screen.size());
  }
*/
  std::vector<POINT> screen;
  add(screen, from);
  add(screen, to);
  the_canvas.clipped_polyline(&screen[0], screen.size(), m_rc);
}

void 
MapDrawHelper::draw_search_point_vector(Canvas& the_canvas, 
                                        const SearchPointVector& points) 
{
  const size_t size = points.size();
  if (size<3) {
    return;
  }
  std::vector<POINT> screen; 
  screen.reserve(size);
  for (SearchPointVector::const_iterator it = points.begin();
       it!= points.end(); ++it) {
    add(screen, it->get_location());
  }
  the_canvas.polygon(&screen[0], size);
  if (m_use_stencil) {
    m_stencil.polygon(&screen[0], size);
  }
}

void 
MapDrawHelper::draw_circle(Canvas& the_canvas, const POINT& center, unsigned radius) 
{
  the_canvas.circle(center.x, center.y, radius);
  if (m_use_stencil) {
    m_stencil.circle(center.x, center.y, radius);
  }
}

void 
MapDrawHelper::buffer_render_finish() 
{
  if (m_buffer_drawn) {
    // need to do this to prevent drawing of colored outline
    m_buffer.white_pen();
    
    if (m_use_stencil) {
      m_buffer.copy_transparent_black(m_stencil, m_rc);
    }
    m_canvas.copy_transparent_white(m_buffer, m_rc);
    m_buffer.background_opaque();
    m_buffer_drawn = false;
  }
}

void 
MapDrawHelper::buffer_render_start() 
{
  if (!m_buffer_drawn) {
    clear_buffer();
    m_buffer_drawn = true;
  }
}

void 
MapDrawHelper::clear_buffer() {
  static const Color whitecolor(0xff,0xff,0xff);
  m_buffer.background_transparent();
  m_buffer.set_background_color(whitecolor);
  m_buffer.set_text_color(whitecolor);
  m_buffer.white_pen();
  m_buffer.white_brush();
  m_buffer.clear();
  
  m_stencil.background_transparent();
  m_stencil.set_background_color(whitecolor);
  m_stencil.set_text_color(whitecolor);
  m_stencil.white_pen();
  m_stencil.white_brush(); 
  m_stencil.clear();
}
