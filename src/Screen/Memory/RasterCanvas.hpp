/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_RASTER_CANVAS_HPP
#define XCSOAR_SCREEN_RASTER_CANVAS_HPP

#include "Buffer.hpp"
#include "Bresenham.hpp"
#include "Murphy.hpp"
#include "Screen/Point.hpp"
#include "Util/AllocatedArray.hxx"
#include "Compiler.h"

#include <assert.h>

/*
  line_masks:
   -1               SOLID
   -1-0b100         DASH
   -1-0b1000        LONGDASH
   -1-0b1100        DOTS
   -1-0b10100       DDOT
 */

/**
 * A software renderer for various primitives.
 *
 * Much of this code was copied from SDL_gfx.
 */
template<typename PixelTraits>
class RasterCanvas : private PixelTraits {
  using typename PixelTraits::pointer_type;
  using typename PixelTraits::rpointer_type;
  using typename PixelTraits::const_pointer_type;
  using typename PixelTraits::const_rpointer_type;
  using PixelTraits::ReadPixel;
  using PixelTraits::ForVertical;

public:
  using typename PixelTraits::color_type;

private:
  WritableImageBuffer<PixelTraits> buffer;

  AllocatedArray<int> polygon_buffer;
  AllocatedArray<BresenhamIterator> edge_buffer;

public:
  RasterCanvas(WritableImageBuffer<PixelTraits> _buffer,
               PixelTraits _traits=PixelTraits())
    :PixelTraits(_traits), buffer(_buffer) {}

protected:
  PixelTraits &GetPixelTraits() {
    return *this;
  }

  constexpr bool Check(unsigned x, unsigned y) const {
    return buffer.Check(x, y);
  }

  pointer_type At(unsigned x, unsigned y) {
    assert(x < buffer.width);
    assert(y < buffer.height);

    return buffer.At(x, y);
  }

  static bool ClipAxis(int &position, unsigned &length, unsigned max,
                       unsigned &src_position) {
    if (position < 0) {
      if (length <= unsigned(-position))
        return false;

      length -= -position;
      src_position -= position;
      position = 0;
    }

    if (unsigned(position) >= max)
      return false;

    if (unsigned(position) + length >= max)
      length = max - position;

    return true;
  }

  static bool ClipScaleAxis(int &dest_position, unsigned &dest_length,
                            unsigned dest_max,
                            unsigned &src_position, unsigned &src_length) {
    if (dest_position < 0) {
      if (dest_length <= unsigned(-dest_position))
        return false;

      const unsigned dest_delta = -dest_position;
      const unsigned src_delta = dest_delta * src_length / dest_length;
      src_position += src_delta;
      src_length -= src_delta;
      dest_length -= dest_delta;

      dest_position = 0;
    }

    if (unsigned(dest_position) >= dest_max)
      return false;

    if (unsigned(dest_position) + dest_length >= dest_max) {
      unsigned new_dest_length = dest_max - dest_position;
      unsigned dest_delta = dest_length - new_dest_length;
      src_length -= dest_delta * src_length / dest_length;
      dest_length = new_dest_length;
    }

    return true;
  }

  static constexpr unsigned CLIP_LEFT_EDGE = 0x1;
  static constexpr unsigned CLIP_RIGHT_EDGE = 0x2;
  static constexpr unsigned CLIP_BOTTOM_EDGE = 0x4;
  static constexpr unsigned CLIP_TOP_EDGE = 0x8;

  static constexpr bool CLIP_INSIDE(unsigned a) {
    return !a;
  }

  static constexpr bool CLIP_REJECT(unsigned a, unsigned b) {
    return a & b;
  }

  static constexpr bool CLIP_ACCEPT(unsigned a, unsigned b) {
    return !(a | b);
  }

  gcc_pure
  unsigned ClipEncodeX(int x) const {
    if (unsigned(x)< buffer.width)
      return 0;
    if (x<0)
      return CLIP_LEFT_EDGE;
    return CLIP_RIGHT_EDGE;
  }

  gcc_pure
  unsigned ClipEncodeY(int y) const {
    if (unsigned(y)< buffer.height)
      return 0;
    if (y<0)
      return CLIP_TOP_EDGE;
    return CLIP_BOTTOM_EDGE;
  }

  gcc_pure
  unsigned ClipEncode(int x, int y) const {
    return ClipEncodeX(x) | ClipEncodeY(y);
  }

  bool ClipIncremental(int &x1, int &y1, int &x2, int &y2,
                       unsigned& code1, unsigned& code2) const {

    bool swapped = false;

    while (true) {
      if (CLIP_ACCEPT(code1, code2)) {
        if (swapped) {
          std::swap(x1, x2);
          std::swap(y1, y2);
          std::swap(code1, code2);
        }
        return true;
      }

      if (CLIP_REJECT(code1, code2))
	return false;

      if (CLIP_INSIDE(code1)) {
        swapped = !swapped;
        std::swap(x1, x2);
        std::swap(y1, y2);
        std::swap(code1, code2);
      }

      if (code1 & CLIP_LEFT_EDGE) {
	if ((y2 != y1) && (x1 != x2)) {
	  const float m = float(y2 - y1) / float(x2 - x1);
	  y1 -= int(x1 * m);
	  code1 = ClipEncodeY(y1);
	} else {
	  code1 &= ~CLIP_LEFT_EDGE;
	}
        x1 = 0;
      } else if (code1 & CLIP_RIGHT_EDGE) {
	if ((y2 != y1) && (x1 != x2)) {
	  const float m = float(y2 - y1) / float(x2 - x1);
	  y1 -= int((x1 - (buffer.width - 1)) * m);
	  code1 = ClipEncodeY(y1);
	} else {
	  code1 &= ~CLIP_RIGHT_EDGE;
	}
        x1 = buffer.width - 1;
      } else if (code1 & CLIP_BOTTOM_EDGE) {
	if ((y2 != y1) && (x1 != x2)) {
	  const float m = float(x2 - x1) / float(y2 - y1);
          x1 -= int((y1 - (buffer.height - 1)) * m);
	  code1 = ClipEncodeX(x1);
	} else {
	  code1 &= ~CLIP_BOTTOM_EDGE;
	}
        y1 = buffer.height - 1;
      } else if (code1 & CLIP_TOP_EDGE) {
	if ((y2 != y1) && (x1 != x2)) {
	  const float m = float(x2 - x1) / float(y2 - y1);
          x1 -= int(y1 * m);
	  code1 = ClipEncodeX(x1);
	} else {
	  code1 &= ~CLIP_TOP_EDGE;
	}
        y1 = 0;
      }
    }
  }

  bool ClipLine(int &x1, int &y1, int &x2, int &y2) const {
    unsigned code1 = ClipEncode(x1, y1);
    unsigned code2 = ClipEncode(x2, y2);
    return ClipIncremental(x1, y1, x2, y2, code1, code2);
  }

public:
  template<typename PixelOperations>
  void DrawPixel(int x, int y, color_type c, PixelOperations operations) {
    if (Check(x, y))
      PixelTraits::WritePixel(At(x, y), c);
  }

  void DrawPixel(int x, int y, color_type c) {
    DrawPixel(x, y, c, GetPixelTraits());
  }

  template<typename PixelOperations>
  void FillRectangle(int x1, int y1, int x2, int y2, color_type c,
                     PixelOperations operations) {
    if (x1 < 0)
      x1 = 0;

    if (y1 < 0)
      y1 = 0;

    if (x2 > int(buffer.width))
      x2 = buffer.width;

    if (y2 > int(buffer.height))
      y2 = buffer.height;

    if (x1 >= x2 || y1 >= y2)
      return;

    const unsigned columns = x2 - x1;

    pointer_type p = At(x1, y1);
    ForVertical(p, buffer.pitch, y2 - y1, [operations, columns, c](pointer_type q){
        operations.FillPixels(q, columns, c);
      });
  }

  void FillRectangle(int x1, int y1, int x2, int y2, color_type c) {
    FillRectangle<PixelTraits>(x1, y1, x2, y2, c, GetPixelTraits());
  }

  template<typename PixelOperations>
  void DrawHLine(int x1, int x2, int y, color_type c,
                 PixelOperations operations) {
    if (y < 0 || unsigned(y) >= buffer.height)
      return;

    if (x1 < 0)
      x1 = 0;

    if (x2 > int(buffer.width))
      x2 = buffer.width;

    if (x1 >= x2)
      return;

    pointer_type p = At(x1, y);
    operations.FillPixels(p, x2 - x1, c);
  }

  void DrawHLine(int x1, int x2, int y, color_type c) {
    DrawHLine(x1, x2, y, c,
              GetPixelTraits());
  }

  template<typename PixelOperations>
  void DrawVLine(int x, int y1, int y2, color_type c,
                 PixelOperations operations) {
    if (x < 0 || unsigned(x) >= buffer.width)
      return;

    if (y1 < 0)
      y1 = 0;

    if (y2 > int(buffer.height))
      y2 = buffer.height;

    if (y1 >= y2)
      return;

    pointer_type p = At(x, y1);
    ForVertical(p, buffer.pitch, y2 - y1, [operations, c](pointer_type q){
        operations.WritePixel(q, c);
      });
  }

  template<typename PixelOperations>
  void DrawRectangle(int x1, int y1, int x2, int y2, color_type c,
                     PixelOperations operations) {
    DrawHLine(x1, x2, y1, c, operations);
    DrawHLine(x1, x2, y2 - 1, c, operations);
    DrawVLine(x1, y1 + 1, y2 - 1, c, operations);
    DrawVLine(x2, y1 + 1, y2 - 1, c, operations);
  }

  void DrawRectangle(int x1, int y1, int x2, int y2, color_type c) {
    DrawRectangle(x1, y1, x2, y2, c, GetPixelTraits());
  }

  void DrawLineDirect(const int x1, const int y1, const int x2, const int y2,
                      color_type c,
                      unsigned line_mask, unsigned &line_mask_position) {
    /* optimised Bresenham algorithm */

    int dx = x2 - x1;
    int dy = y2 - y1;
    const int sx = dx >= 0 ? 1 : -1;
    const int sy = dy >= 0 ? 1 : -1;

    dx = sx * dx + 1;
    dy = sy * dy + 1;

    pointer_type p = At(x1, y1);

    int pixx = PixelTraits::CalcIncrement(sx) * sizeof(*p);
    int pixy = sy * buffer.pitch;

    if (dx < dy) {
      std::swap(dx, dy);
      std::swap(pixx, pixy);
    }

    unsigned lmp = line_mask_position;

    for (int x = 0, y = 0; x < dx; x++, p = PixelTraits::NextByte(p, pixx)) {
      if ((lmp++ | line_mask) == unsigned(-1))
        PixelTraits::WritePixel(p, c);

      y += dy;
      if (y >= dx) {
        y -= dx;
        p = PixelTraits::NextByte(p, pixy);
      }
    }

    line_mask_position = lmp;
  }

  void DrawLine(int x1, int y1, int x2, int y2, color_type c,
                unsigned line_mask=-1) {
    /* optimised Bresenham algorithm */

    if (!ClipLine(x1, y1, x2, y2))
      return;

    unsigned line_mask_position = 0;
    DrawLineDirect(x1, y1, x2, y2, c, line_mask, line_mask_position);
  }

  void DrawThickLine(int x1, int y1, int x2, int y2,
                     unsigned thickness, color_type c,
                     unsigned line_mask, unsigned &line_mask_position) {
    if (thickness == 0)
      return;

    /* Special case: thick "point" */
    if (x1 == x2 && y1 == y2) {
      const unsigned radius = thickness / 2u;
      FillRectangle(x1 - radius, y1 - radius, x1 + radius, y1 + radius, c);
      return;
    }

    MurphyIterator<RasterCanvas<PixelTraits>> murphy(*this, c, line_mask,
                                                     line_mask_position);
    murphy.Wideline(x1, y1, x2, y2, thickness, 0);
    murphy.Wideline(x1, y1, x2, y2, thickness, 1);
    line_mask_position = murphy.GetLineMaskPosition();
  }

  void DrawPolyline(const PixelPoint *points, unsigned n, bool loop,
                    color_type color,
                    unsigned thickness,
                    unsigned line_mask=-1) {

    auto p_last = points[loop? n-1 : 0];
    unsigned code2_orig;
    unsigned code2;
    bool last_visible = false;

    /* this variable keeps track of the position on the line mask
       across all line segments, for continuity */
    unsigned line_mask_position = 0;

    for (unsigned i= loop? 0: 1; i<n; ++i) {
      auto p_this = points[i];
      if (!last_visible) {
        // don't have a start point yet
        code2_orig = ClipEncode(p_last.x, p_last.y);
        code2 = code2_orig;
      }
      unsigned code1_orig = ClipEncode(p_this.x, p_this.y);

      if (CLIP_REJECT(code1_orig, code2_orig)) {
        // both not visible, skip
        p_last = p_this;
        last_visible = false;
        continue;
      }

      unsigned code1 = code1_orig;
      if (ClipIncremental(p_this.x, p_this.y, p_last.x, p_last.y, code1, code2)) {
        if (thickness > 1)
          DrawThickLine(p_this.x, p_this.y, p_last.x, p_last.y, thickness, color,
                        line_mask, line_mask_position);
        else
          DrawLineDirect(p_this.x, p_this.y, p_last.x, p_last.y, color,
                         line_mask, line_mask_position);
        if (code1 == code1_orig) {
          last_visible = true;
          p_last = p_this;
          code2 = code1;
          code2_orig = code1_orig;
        } else {
          last_visible = false;
          p_last = points[i];
        }
      } else {
        last_visible = false;
        p_last = points[i];
      }
    }

  }

  template<typename PixelOperations>
  void FillPolygonFast(const PixelPoint *points, unsigned n, color_type color,
                       PixelOperations operations) {

    assert(points != nullptr);

    if (n < 3)
      return;

    edge_buffer.GrowDiscard(n);

    // initialise buffer of edge iterators, and find y range to scan
    int miny = points[0].y;
    int maxy = points[0].y;

    const auto *p_1 = points;
    const auto *p_0 = points + n - 1;
    int n_edges = 0;

    while (p_1 < points+n) {
      if (p_1->y == p_0->y) {
        // don't add horizontal line, just draw it now?
      } else if (p_1->y < p_0->y) {
        edge_buffer[n_edges] = BresenhamIterator(p_1->x, p_1->y, p_0->x, p_0->y);
        n_edges++;
      } else {
        edge_buffer[n_edges] = BresenhamIterator(p_0->x, p_0->y, p_1->x, p_1->y);
        n_edges++;
      }
      miny = std::min(p_0->y, miny);
      maxy = std::max(p_0->y, maxy);

      p_0 = p_1;
      p_1++;
    }

    if (n_edges < 2)
      return;

    auto edge_start = edge_buffer.begin();
    auto edge_end = edge_start+n_edges;

    // sort array by y value (top best), then x value (left best)
    std::sort(edge_start, edge_end, BresenhamIterator::CompareVerticalHorizontal);

    // perform scans

    for (int y = miny; y <= maxy; y++) {

      bool changed = false;

      // advance active items
      int x = -1;
      for (auto it= edge_start; it!= edge_end; ++it) {
        // advance line until it gets to next y value (if possible)
        changed |= it->AdvanceTo(y);
        if (it->y == y) {
          if (it->x < x)
            changed = true; // order changed
          else
            x = it->x;
        }
      }

      if (changed) {
//        printf("re-sort\n");
        std::sort(edge_start, edge_end, BresenhamIterator::CompareHorizontal);

        while ((edge_start != edge_end) && (!edge_start->count)) {
//          printf("skipping forward\n");
          edge_start++;
        }

/*
        for (auto it= edge_start; it!= edge_end; ++it) {
          printf("%d,%d %d\n", it->x, it->y, it->count);
        }
*/
      }

      // draw line and determine status
      bool mode = false;
      int x0 = -1;
      for (auto it= edge_start; it!= edge_end; ++it) {
        // if this item is valid, it's a start point or end point of a line
        if (it->y != y) 
          continue;

        int x1 = it->x;
        if (mode) 
          DrawHLine(x0, x1, y, color, operations);
        mode = !mode;
        x0 = x1;
      }

    }

  }

  template<typename PixelOperations>
  void FillPolygon(const PixelPoint *points, unsigned n, color_type color,
                   PixelOperations operations) {
    assert(points != nullptr);

    if (n < 3)
      return;

    // Allocate temp array, only grow array
    polygon_buffer.GrowDiscard(n);
    int *const ints = polygon_buffer.begin();

    // Determine Y maxima
    int miny = points[0].y;
    int maxy = points[0].y;

    for (unsigned i = 1; i < n; i++) {
      if (points[i].y < miny)
        miny = points[i].y;
      else if (points[i].y > maxy)
        maxy = points[i].y;
    }

    // Draw, scanning y
    for (int y = miny; y <= maxy; y++) {
      unsigned n_ints = 0;
      for (unsigned i = 0; i < n; i++) {
        unsigned ind1, ind2;
        if (i == 0) {
          ind1 = n - 1;
          ind2 = 0;
        } else {
          ind1 = i - 1;
          ind2 = i;
        }

        int y1 = points[ind1].y;
        int y2 = points[ind2].y;
        int x1, x2;

        if (y1 < y2) {
          x1 = points[ind1].x;
          x2 = points[ind2].x;
        } else if (y1 > y2) {
          y2 = points[ind1].y;
          y1 = points[ind2].y;
          x2 = points[ind1].x;
          x1 = points[ind2].x;
        } else
          continue;

        if ( ((y >= y1) && (y < y2)) || ((y == maxy) && (y > y1) && (y <= y2)) ) {
          ints[n_ints++] = ((65536 * (y - y1)) / (y2 - y1)) * (x2 - x1) + (65536 * x1);
        }
      }

      std::sort(ints, ints + n_ints);

      for (unsigned i = 0; i < n_ints; i += 2) {
        int xa = ints[i] + 1;
        xa = (xa >> 16) + ((xa & 32768) >> 15);
        int xb = ints[i+1] - 1;
        xb = (xb >> 16) + ((xb & 32768) >> 15);
        DrawHLine(xa, xb, y, color, operations);
      }
    }
  }

  void FillPolygon(const PixelPoint *points, unsigned n, color_type color) {
    FillPolygonFast(points, n, color,
                    GetPixelTraits());
//    FillPolygon(points, n, color,
//                GetPixelTraits());
  }

  template<typename PixelOperations>
  void DrawCircle(int x, int y, unsigned rad, color_type color,
                  PixelOperations operations) {
    // Special case for rad=0 - draw a point
    if (rad == 0) {
      DrawPixel(x, y, color, operations);
      return;
    }

    /* Get circle and clipping boundary and test if bounding box of
       circle is visible */
    const int x2 = x + rad;
    if (x2 < 0)
      return;

    const int x1 = x - rad;
    if (x1 >= int(buffer.width))
      return;

    const int y2 = y + rad;
    if (y2 < 0)
      return;

    const int y1 = y - rad;
    if (y1 >= int(buffer.height))
      return;

    // draw
    int cx = 0, cy = rad;
    int df = 1 - rad;
    int d_e = 3;
    int d_se = -2 * rad + 5;

    do {
      const int ypcy = y + cy, ymcy = y - cy;

      if (cx > 0) {
        const int xpcx = x + cx, xmcx = x - cx;

        DrawPixel(xmcx, ypcy, color, operations);
        DrawPixel(xpcx, ypcy, color, operations);
        DrawPixel(xmcx, ymcy, color, operations);
        DrawPixel(xpcx, ymcy, color, operations);
      } else {
        DrawPixel(x, ymcy, color, operations);
        DrawPixel(x, ypcy, color, operations);
      }

      const int xpcy = x + cy;
      const int xmcy = x - cy;
      if (cx > 0 && cx != cy) {
        const int ypcx = y + cx, ymcx = y - cx;

        DrawPixel(xmcy, ypcx, color, operations);
        DrawPixel(xpcy, ypcx, color, operations);
        DrawPixel(xmcy, ymcx, color, operations);
        DrawPixel(xpcy, ymcx, color, operations);
      } else if (cx == 0) {
        DrawPixel(xmcy, y, color, operations);
        DrawPixel(xpcy, y, color, operations);
      }

      // Update
      if (df < 0) {
        df += d_e;
        d_e += 2;
        d_se += 2;
      } else {
        df += d_se;
        d_e += 2;
        d_se += 4;
        cy--;
      }
      cx++;
    } while (cx <= cy);
  }

  void DrawCircle(int x, int y, unsigned rad, color_type color) {
    DrawCircle(x, y, rad, color,
               GetPixelTraits());
  }

  template<typename PixelOperations>
  void FillCircle(int x, int y, unsigned rad, color_type color,
                  PixelOperations operations) {
    // Special case for rad=0 - draw a point
    if (rad == 0) {
      DrawPixel(x, y, color, operations);
      return;
    }

    /* Get circle and clipping boundary and test if bounding box of
       circle is visible */
    const int x2 = x + rad;
    if (x2 < 0)
      return;

    const int x1 = x - rad;
    if (x1 >= int(buffer.width))
      return;

    const int y2 = y + rad;
    if (y2 < 0)
      return;

    const int y1 = y - rad;
    if (y1 >= int(buffer.height))
      return;

    // draw
    int cx = 0, cy = rad;
    int ocx = 0xffff, ocy = 0xffff;
    int df = 1 - rad;
    int d_e = 3;
    int d_se = -2 * rad + 5;

    do {
      const int xpcx = x + cx, xpcy = x + cy;
      const int xmcx = x - cx, xmcy = x - cy;

      if (ocy != cy) {
        if (cy > 0) {
          const int ypcy = y + cy;
          const int ymcy = y - cy;
          DrawHLine(xmcx, xpcx, ypcy, color, operations);
          DrawHLine(xmcx, xpcx, ymcy, color, operations);
        } else {
          DrawHLine(xmcx, xpcx, y, color, operations);
        }
        ocy = cy;
      }
      if (ocx != cx) {
        if (cx != cy) {
          if (cx > 0) {
            const int ypcx = y + cx;
            const int ymcx = y - cx;
            DrawHLine(xmcy, xpcy, ymcx, color, operations);
            DrawHLine(xmcy, xpcy, ypcx, color, operations);
          } else {
            DrawHLine(xmcy, xpcy, y, color, operations);
          }
        }
        ocx = cx;
      }
      /*
       * Update
       */
      if (df < 0) {
        df += d_e;
        d_e += 2;
        d_se += 2;
      } else {
        df += d_se;
        d_e += 2;
        d_se += 4;
        cy--;
      }
      cx++;
    } while (cx <= cy);
  }

  void FillCircle(int x, int y, unsigned rad, color_type color) {
    FillCircle(x, y, rad, color,
               GetPixelTraits());
  }

  template<typename PixelOperations, typename SPT=PixelTraits>
#ifndef __clang__
  __attribute__((flatten))
#endif
  void CopyRectangle(int x, int y, unsigned w, unsigned h,
                     typename SPT::const_rpointer_type src, unsigned src_pitch,
                     PixelOperations operations) {
    unsigned src_x = 0, src_y = 0;
    if (!ClipAxis(x, w, buffer.width, src_x) ||
        !ClipAxis(y, h, buffer.height, src_y))
      return;

    src = SPT::At(src, src_pitch, src_x, src_y);

    pointer_type p = At(x, y);
    for (; h > 0; --h, p = PixelTraits::NextRow(p, buffer.pitch, 1),
           src = SPT::NextRow(src, src_pitch, 1))
      operations.CopyPixels(p, src, w);
  }

  void CopyRectangle(int x, int y, unsigned w, unsigned h,
                     const_pointer_type src, unsigned src_pitch) {
    CopyRectangle(x, y, w, h, src, src_pitch,
                  GetPixelTraits());
  }

  template<typename PixelOperations, typename SPT=PixelTraits>
  void ScalePixels(rpointer_type dest, unsigned dest_size,
                   typename SPT::const_rpointer_type src,
                   unsigned src_size,
                   PixelOperations operations) const {
#if defined(__ARM_NEON__) && defined(GREYSCALE)
    if (dest_size == src_size * 2) {
      /* NEON-optimised special case */
      NEONBytesTwice neon;
      neon.CopyPixels(dest, src, src_size);

      /* use the portable version for the remainder */
      src += src_size & ~0xf;
      dest += (src_size & ~0xf) * 2;
      src_size &= 0xf;
      dest_size = src_size * 2;
    }
#endif

    unsigned j = 0;
    for (unsigned i = 0; i < dest_size; ++i) {
      operations.WritePixel(PixelTraits::Next(dest, i),
                            SPT::ReadPixel(src));

      j += src_size;
      while (j >= dest_size) {
        j -= dest_size;
        ++src;
      }
    }
  }

  template<typename PixelOperations, typename SPT=PixelTraits>
  void ScaleRectangle(int dest_x, int dest_y,
                      unsigned dest_width, unsigned dest_height,
                      typename SPT::const_rpointer_type src, unsigned src_pitch,
                      unsigned src_width, unsigned src_height,
                      PixelOperations operations) {
    unsigned src_x = 0, src_y = 0;
    if (!ClipScaleAxis(dest_x, dest_width, buffer.width, src_x, src_width) ||
        !ClipScaleAxis(dest_y, dest_height, buffer.height, src_y, src_height))
      return;

    src = SPT::At(src, src_pitch, src_x, src_y);

    typename SPT::const_rpointer_type old_src = nullptr;

    unsigned j = 0;
    rpointer_type dest = At(dest_x, dest_y);
    for (unsigned i = dest_height; i > 0; --i,
           dest = PixelTraits::NextRow(dest, buffer.pitch, 1)) {
      if (src == old_src) {
        /* the previous iteration has already scaled this row: copy
           the previous destination row to the current destination
           row, to avoid redundant ScalePixels() calls */
        PixelTraits::CopyPixels(dest,
                                PixelTraits::NextRow(dest, buffer.pitch, -1),
                                dest_width);
      } else {
        ScalePixels<decltype(operations), SPT>(dest, dest_width, src, src_width,
                                               operations);
        old_src = src;
      }

      j += src_height;
      while (j >= dest_height) {
        j -= dest_height;
        src = SPT::NextRow(src, src_pitch, 1);
      }
    }
  }

  void ScaleRectangle(int dest_x, int dest_y,
                      unsigned dest_width, unsigned dest_height,
                      const_rpointer_type src, unsigned src_pitch,
                      unsigned src_width, unsigned src_height) {
    ScaleRectangle(dest_x, dest_y, dest_width, dest_height,
                   src, src_pitch, src_width, src_height,
                   GetPixelTraits());
  }
};

#endif
