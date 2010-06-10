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
  unsigned int width;
  unsigned int height;
  unsigned int corrected_width;
  BGRColor *buffer;
  BGRColor *second_buffer;

public:
  /**
   * Creates buffer with the given size and fills it with
   * the given color
   * @param nWidth Width of the buffer
   * @param nHeight Height of the buffer
   * @param clr Fill color of the buffer
   */
  RawBitmap(unsigned width, unsigned height, const Color color);

  virtual ~RawBitmap();

  /**
   * Returns the Buffer
   * @return The Buffer as BGRColor array
   */
  BGRColor *GetBuffer(void) {
    return buffer;
  }

  void HorizontalBlur(unsigned int boxw);
  void VerticalBlur(unsigned int boxh);
  void Zoom(unsigned int step);

  /**
   * Returns real width of the screen buffer. It could be slightly more then
   * requested width. This parameter is important only when you work with
   * points array directly (using GetPointsArray function).
   * @return Real width of the screen buffer
   */
  int GetCorrectedWidth() {
    return corrected_width;
  }

  /**
   * Returns the screen buffer width
   * @return The screen buffer width
   */
  int GetWidth() {
    return width;
  }

  /**
   * Returns screen buffer height
   * @return The screen buffer height
   */
  int GetHeight() {
    return height;
  }

public:
  /**
   * Returns minimum width that is greater then the given width and
   * that is acceptable as image width (not all numbers are acceptable)
   */
  static int CorrectedWidth(int nWidth);
};

#endif // !defined(AFX_STSCREENBUFFER_H__22D62F5D_32E2_4785_B3D9_2341C11F84A3__INCLUDED_)
