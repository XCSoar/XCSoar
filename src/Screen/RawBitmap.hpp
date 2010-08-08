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

#include <windows.h>

#include "Screen/Bitmap.hpp"
#include "Screen/BitmapCanvas.hpp"

/**
 * BGRColor structure encapsulates color information about one point. Color
 * order is Blue, Green, Red (not RGB).
 */
struct BGRColor
{
  BGRColor() {}
  BGRColor(unsigned char R, unsigned char G, unsigned char B)
    : m_B(B), m_G(G), m_R(R) {}
  unsigned char m_B;
  unsigned char m_G;
  unsigned char m_R;
};

/**
 * This class provides fast drawing methods and offscreen buffer.
 */
class RawBitmap : public Bitmap {
protected:
  const unsigned int width;
  const unsigned int height;
  const unsigned int corrected_width;
  BGRColor *buffer;

public:
  /**
   * Creates buffer with the given size and fills it with
   * the given color
   * @param nWidth Width of the buffer
   * @param nHeight Height of the buffer
   * @param clr Fill color of the buffer
   */
  RawBitmap(unsigned width, unsigned height, const Color color);

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
};

#endif // !defined(AFX_STSCREENBUFFER_H__22D62F5D_32E2_4785_B3D9_2341C11F84A3__INCLUDED_)
