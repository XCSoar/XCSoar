/**
 * @file STScreenBuffer.h
 * @details Class for pixel working with images. One could get access
 * to image bits using this library or create custom image.
 * @author Vassili Philippov (vasja@spbteam.com)
 * @date Created: June 2001
 * @date Last changed: 07 August 2001
 * @version 2.0
 */

#ifndef XCSOAR_SCREEN_RAW_BITMAP_HPP
#define XCSOAR_SCREEN_RAW_BITMAP_HPP

#include "Screen/Canvas.hpp"

/**
 * BGRColor structure encapsulates color information about one point. Color
 * order is Blue, Green, Red (not RGB).
 */
struct BGRColor
{
  BGRColor() {}
  BGRColor(unsigned char R, unsigned char G, unsigned char B)
    :b(B), g(G), r(R) {}

  unsigned char b;
  unsigned char g;
  unsigned char r;

#ifdef ENABLE_SDL
  unsigned char dummy;
#endif
};

/**
 * This class provides fast drawing methods and offscreen buffer.
 */
class RawBitmap {
protected:
  const unsigned int width;
  const unsigned int height;
  const unsigned int corrected_width;
  BGRColor *buffer;

#ifdef ENABLE_SDL
  SDL_Surface *surface;
#else
  BITMAPINFO bi;
#if defined(_WIN32_WCE) && _WIN32_WCE < 0x0400
  HBITMAP bitmap;
#endif
#endif

public:
  /**
   * Creates buffer with the given size and fills it with
   * the given color
   * @param nWidth Width of the buffer
   * @param nHeight Height of the buffer
   * @param clr Fill color of the buffer
   */
  RawBitmap(unsigned width, unsigned height);
  ~RawBitmap();

  /**
   * Returns the Buffer
   * @return The Buffer as BGRColor array
   */
  BGRColor *GetBuffer(void) {
    return buffer;
  }

  /**
   * Returns a pointer to the top-most row.
   */
  BGRColor *GetTopRow() {
#ifdef ENABLE_SDL
    return buffer;
#else
  /* in WIN32 bitmaps, the bottom-most row comes first */
    return buffer + (height - 1) * corrected_width;
#endif
  }

  /**
   * Returns a pointer to the row below the current one.
   */
  BGRColor *GetNextRow(BGRColor *row) {
#ifdef ENABLE_SDL
    return row + corrected_width;
#else
    return row - corrected_width;
#endif
  }

  /**
   * Returns real width of the screen buffer. It could be slightly more then
   * requested width. This parameter is important only when you work with
   * points array directly (using GetPointsArray function).
   * @return Real width of the screen buffer
   */
  unsigned GetCorrectedWidth() const {
    return corrected_width;
  }

  /**
   * Returns the screen buffer width
   * @return The screen buffer width
   */
  unsigned GetWidth() const {
    return width;
  }

  /**
   * Returns screen buffer height
   * @return The screen buffer height
   */
  unsigned GetHeight() const {
    return height;
  }

  void stretch_to(unsigned width, unsigned height, Canvas &dest_canvas,
                  unsigned dest_width, unsigned dest_height) const {
#ifdef ENABLE_SDL
    Canvas src_canvas(surface);
    dest_canvas.stretch(0, 0, dest_width, dest_height,
                        src_canvas, 0, 0, width, height);
#elif defined(_WIN32_WCE) && _WIN32_WCE < 0x0400
    /* StretchDIBits() is bugged on PPC2002, workaround follows */
    HDC source_dc = ::CreateCompatibleDC(dest_canvas);
    ::SelectObject(source_dc, bitmap);
    ::StretchBlt(dest_canvas, 0, 0,
                 dest_width, dest_height,
                 source_dc, 0, 0, width, height,
                 SRCCOPY);
    ::DeleteDC(source_dc);
#else
    ::StretchDIBits(dest_canvas, 0, 0,
                    dest_width, dest_height,
                    0, GetHeight() - height, width, height,
                    buffer, &bi, DIB_RGB_COLORS, SRCCOPY);
#endif
  }
};

#endif // !defined(AFX_STSCREENBUFFER_H__22D62F5D_32E2_4785_B3D9_2341C11F84A3__INCLUDED_)
