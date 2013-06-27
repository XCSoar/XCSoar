/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Util/AllocatedArray.hpp"
#include "Compiler.h"

#include <assert.h>

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

  struct Point {
    int x, y;

    Point() = default;
    constexpr Point(int _x, int _y):x(_x), y(_y) {}
    constexpr Point(unsigned _x, unsigned _y):x(_x), y(_y) {}
  };

private:
  WritableImageBuffer<PixelTraits> buffer;

  AllocatedArray<int> polygon_buffer;

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

  bool ClipLine(int &x1, int &y1, int &x2, int &y2) {
    unsigned code1 = ClipEncode(x1, y1);
    unsigned code2 = ClipEncode(x2, y2);

    while (true) {
      if (CLIP_ACCEPT(code1, code2))
        return true;

      if (CLIP_REJECT(code1, code2))
	return false;

      if (CLIP_INSIDE(code1)) {
        std::swap(x1, x2);
        std::swap(y1, y2);
        std::swap(code1, code2);
      }

      if (code1 & CLIP_LEFT_EDGE) {
	if ((y2 != y1) && (x1 != x2)) {
	  const float m = float(y2 - y1) / float(x2 - x1);
	  y1 -= x1 * m;
	}
        x1 = 0;
      } else if (code1 & CLIP_RIGHT_EDGE) {
	if ((y2 != y1) && (x1 != x2)) {
	  const float m = float(y2 - y1) / float(x2 - x1);
	  y1 -= int((x1 - (buffer.width - 1)) * m);
	}
        x1 = buffer.width - 1;
      } else if (code1 & CLIP_BOTTOM_EDGE) {
	if ((y2 != y1) && (x1 != x2)) {
	  const float m = float(x2 - x1) / float(y2 - y1);
          x1 -= int((y1 - (buffer.height - 1)) * m);
	}
        y1 = buffer.height - 1;
      } else if (code1 & CLIP_TOP_EDGE) {
	if ((y2 != y1) && (x1 != x2)) {
	  const float m = float(x2 - x1) / float(y2 - y1);
          x1 -= y1 * m;
	}
        y1 = 0;
      }
      code1 = ClipEncode(x1, y1);
    }
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

  void FillRectangle(int x1, int y1, int x2, int y2, color_type c) {
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
    ForVertical(p, buffer.pitch, y2 - y1, [this, columns, c](pointer_type q){
        this->FillPixels(q, columns, c);
      });
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

  void DrawLine(int x1, int y1, int x2, int y2, color_type c) {
    /* optimised Bresenham algorithm */

    if (!ClipLine(x1, y1, x2, y2))
      return;

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

    for (int x = 0, y = 0; x < dx; x++, p = PixelTraits::NextByte(p, pixx)) {
      PixelTraits::WritePixel(p, c);

      y += dy;
      if (y >= dx) {
        y -= dx;
        p = PixelTraits::NextByte(p, pixy);
      }
    }
  }

  void DrawThickLine(int x1, int y1, int x2, int y2,
                     unsigned thickness, color_type c) {
    if (thickness == 0)
      return;

    /* Special case: thick "point" */
    if (x1 == x2 && y1 == y2) {
      const unsigned radius = thickness / 2u;
      FillRectangle(x1 - radius, y1 - radius, x1 + radius, y1 + radius, c);
      return;
    }

    MurphyIterator<RasterCanvas<PixelTraits>> murphy(*this, c);
    murphy.Wideline(x1, y1, x2, y2, thickness, 0);
    murphy.Wideline(x1, y1, x2, y2, thickness, 1);
  }

  template<typename PixelOperations>
  void FillPolygon(const Point *points, unsigned n, color_type color,
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

  void FillPolygon(const Point *points, unsigned n, color_type color) {
    FillPolygon(points, n, color,
                GetPixelTraits());
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

    unsigned j = 0;
    rpointer_type dest = At(dest_x, dest_y);
    for (unsigned i = dest_height; i > 0; --i,
           dest = PixelTraits::NextRow(dest, buffer.pitch, 1)) {
      ScalePixels<decltype(operations), SPT>(dest, dest_width, src, src_width,
                                             operations);

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
