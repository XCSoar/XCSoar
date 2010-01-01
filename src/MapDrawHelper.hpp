#ifndef MAP_DRAW_HELPER_HPP
#define MAP_DRAW_HELPER_HPP

#include "Navigation/SearchPointVector.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"

class Canvas;
class MapWindow;

class MapDrawHelper 
{
public:
  MapDrawHelper(Canvas &_canvas, 
                Canvas &_buffer, 
                Canvas &_stencil, 
                MapWindow &_map,
                const RECT &_rc);

  MapDrawHelper(MapDrawHelper &_that);

  ~MapDrawHelper();

  Canvas &m_canvas;
  Canvas &m_buffer;
  Canvas &m_stencil;
  MapWindow& m_map;
  const RECT& m_rc;
  bool m_buffer_drawn;
  bool m_use_stencil;

protected:

  void draw_great_circle(Canvas& the_canvas, const GEOPOINT &from,
                         const GEOPOINT &to);

  void draw_search_point_vector(Canvas& the_canvas, const SearchPointVector& points);

  void draw_circle(Canvas& the_canvas, const POINT& center, unsigned radius);

  void buffer_render_finish();

  void buffer_render_start();

  void clear_buffer();

  bool add_if_visible(std::vector<POINT>& screen, const GEOPOINT& pt) const;
  void add(std::vector<POINT>& screen, const GEOPOINT& pt) const;

};


#endif
