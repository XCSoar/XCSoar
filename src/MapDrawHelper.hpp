#ifndef MAP_DRAW_HELPER_HPP
#define MAP_DRAW_HELPER_HPP

#include "Navigation/SearchPointVector.hpp"

class MapDrawHelper 
{
public:
  MapDrawHelper(Canvas &_canvas, 
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
    m_use_stencil(false) {};

  MapDrawHelper(MapDrawHelper &_that):
    m_canvas(_that.m_canvas),
    m_buffer(_that.m_buffer),
    m_stencil(_that.m_stencil),
    m_map(_that.m_map),
    m_rc(_that.m_rc),
    m_buffer_drawn(_that.m_buffer_drawn),
    m_use_stencil(_that.m_use_stencil) {};

  ~MapDrawHelper() {
    buffer_render_finish();
  }

  Canvas &m_canvas;
  Canvas &m_buffer;
  Canvas &m_stencil;
  MapWindow& m_map;
  const RECT& m_rc;
  bool m_buffer_drawn;
  bool m_use_stencil;

protected:

  void draw_search_point_vector(Canvas& the_canvas, const SearchPointVector& points) {
    const size_t size = points.size();
    if (size<3) {
      return;
    }
    std::vector<POINT> screen; 
    screen.reserve(size);
    for (SearchPointVector::const_iterator it = points.begin();
         it!= points.end(); ++it) {
      POINT sc;
      m_map.LonLat2Screen(it->get_location(), sc);
      screen.push_back(sc);
    }
    the_canvas.polygon(&screen[0], size);
    if (m_use_stencil) {
      m_stencil.polygon(&screen[0], size);
    }
  }

  void draw_circle(Canvas& the_canvas, const POINT& center, unsigned radius) {
    the_canvas.circle(center.x, center.y, radius);
    if (m_use_stencil) {
      m_stencil.circle(center.x, center.y, radius);
    }
  }

  void buffer_render_finish() {
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

  void buffer_render_start() {
    if (!m_buffer_drawn) {
      clear_buffer();
      m_buffer_drawn = true;
    }
  }

  void clear_buffer() {
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

};


#endif
