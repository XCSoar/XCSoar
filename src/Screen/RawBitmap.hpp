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

#include "Screen/Point.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Surface.hpp"
#include "Screen/OpenGL/Features.hpp"
#endif

#ifdef ENABLE_SDL
#include <SDL_endian.h>
#endif

#include <stdint.h>

class Canvas;

#ifdef ENABLE_OPENGL
class GLTexture;
#elif defined(ENABLE_SDL)
struct SDL_Surface;
#endif

/**
 * BGRColor structure encapsulates color information about one point. Color
 * order is Blue, Green, Red (not RGB).
 */
struct BGRColor
{
  BGRColor() = default;

#ifdef HAVE_GLES
  /**
   * RGB color value encoded with 5/6/5 bits per channel.
   */
  uint16_t value;

  constexpr BGRColor(uint8_t R, uint8_t G, uint8_t B)
    :value(((R & 0xf8) << 8) |
           ((G & 0xfc) << 3) |
           (B >> 3)) {}

#elif defined(ENABLE_SDL)

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  uint8_t dummy;
  uint8_t r;
  uint8_t g;
  uint8_t b;

  constexpr BGRColor(uint8_t R, uint8_t G, uint8_t B)
    :dummy(), r(R), g(G), b(B) {}
#else /* little endian */
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t dummy;

  constexpr BGRColor(uint8_t R, uint8_t G, uint8_t B)
    :b(B), g(G), r(R), dummy() {}
#endif /* little endian */

#else /* !SDL */

#ifdef _WIN32_WCE
  /**
   * RGB color value encoded with 5/5/5 bits per channel.  The most
   * significant bit is unused.
   */
  uint16_t value;

  constexpr BGRColor(uint8_t R, uint8_t G, uint8_t B)
    :value(((R & 0xf8) << 7) |
           ((G & 0xf8) << 2) |
           (B >> 3)) {}

#else /* !_WIN32_WCE */
  uint8_t b;
  uint8_t g;
  uint8_t r;

  constexpr BGRColor(uint8_t R, uint8_t G, uint8_t B)
    :b(B), g(G), r(R) {}
#endif /* !_WIN32_WCE */

#endif /* !SDL */
};

/**
 * This class provides fast drawing methods and offscreen buffer.
 */
class RawBitmap
#ifdef ENABLE_OPENGL
  :private GLSurfaceListener
#endif
{
protected:
  const UPixelScalar width;
  const UPixelScalar height;
  const UPixelScalar corrected_width;
  BGRColor *buffer;

#ifdef ENABLE_OPENGL
  GLTexture *texture;

  /**
   * Has the buffer been modified, and needs to be copied into the
   * texture?
   */
  mutable bool dirty;
#elif defined(ENABLE_SDL)
  SDL_Surface *surface;
#else
  BITMAPINFO bi;
#ifdef _WIN32_WCE
  /**
   * Need RGB masks for BI_BITFIELDS (16 bit 5-5-5).  This attribute
   * is not used explicitly, it is only here to reserve enough room
   * after the BITMAPINFO attribute.
   */
  DWORD mask_buffer[3];
#endif
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
  RawBitmap(UPixelScalar width, UPixelScalar height);

#ifdef ENABLE_OPENGL
  virtual
#endif
  ~RawBitmap();

  /**
   * Returns the Buffer
   * @return The Buffer as BGRColor array
   */
  BGRColor *GetBuffer() {
    return buffer;
  }

  /**
   * Returns a pointer to the top-most row.
   */
  BGRColor *GetTopRow() {
#ifndef USE_GDI
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
#ifndef USE_GDI
    return row + corrected_width;
#else
    return row - corrected_width;
#endif
  }

  void SetDirty() {
#ifdef ENABLE_OPENGL
    dirty = true;
#endif
  }

  /**
   * Returns real width of the screen buffer. It could be slightly more then
   * requested width. This parameter is important only when you work with
   * points array directly (using GetPointsArray function).
   * @return Real width of the screen buffer
   */
  UPixelScalar GetCorrectedWidth() const {
    return corrected_width;
  }

  /**
   * Returns the screen buffer width
   * @return The screen buffer width
   */
  UPixelScalar GetWidth() const {
    return width;
  }

  /**
   * Returns screen buffer height
   * @return The screen buffer height
   */
  UPixelScalar GetHeight() const {
    return height;
  }

#ifdef ENABLE_OPENGL
  /**
   * Bind the texture and return a reference to it.  If the texture is
   * "dirty", then the RAM buffer will be copied to the texture by
   * this method.
   */
  GLTexture &BindAndGetTexture() const;
#endif

  void StretchTo(UPixelScalar width, UPixelScalar height, Canvas &dest_canvas,
                 UPixelScalar dest_width, UPixelScalar dest_height) const;

#ifdef ENABLE_OPENGL
private:
  /* from GLSurfaceListener */
  virtual void SurfaceCreated() override;
  virtual void SurfaceDestroyed() override;
#endif
};

#endif // !defined(AFX_STSCREENBUFFER_H__22D62F5D_32E2_4785_B3D9_2341C11F84A3__INCLUDED_)
