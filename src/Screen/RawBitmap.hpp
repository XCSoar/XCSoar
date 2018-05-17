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

#include "PortableColor.hpp"
#include "OS/ByteOrder.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Surface.hpp"
#include "Screen/OpenGL/Features.hpp"
#endif

#ifdef USE_GDI
#include <windef.h>
#include <wingdi.h>
#else
#include <memory>
#endif

#include <stdint.h>

class Canvas;

#ifdef ENABLE_OPENGL
class GLTexture;
#endif

/**
 * The RawColor structure encapsulates color information about one
 * point in a #RawBitmap.
 */
struct RawColor
{
  RawColor() = default;

#ifdef GREYSCALE
  Luminosity8 value;

  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B)
    :value(R, G, B) {}

#elif defined(HAVE_GLES)
  RGB565Color value;

  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B)
    :value(R, G, B) {}

#elif defined(USE_MEMORY_CANVAS) || defined(ENABLE_SDL) || defined(USE_EGL) || defined(USE_GLX)

#if IS_BIG_ENDIAN
  /* big-endian */
  uint8_t dummy;
  RGB8Color value;

  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B)
    :dummy(), value(R, G, B) {}
#else
  /* little-endian */
  BGR8Color value;
  uint8_t dummy;

  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B)
    :value(R, G, B), dummy() {}
#endif

#else /* !SDL */

  BGR8Color value;

  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B)
    :value(R, G, B) {}

#endif
};

/**
 * This class provides fast drawing methods and offscreen buffer.
 */
class RawBitmap final
#ifdef ENABLE_OPENGL
  :private GLSurfaceListener
#endif
{
protected:
  const unsigned width;
  const unsigned height;
  const unsigned corrected_width;

#ifdef USE_GDI
  RawColor *buffer;
#else
  const std::unique_ptr<RawColor[]> buffer;
#endif

#ifdef ENABLE_OPENGL
  GLTexture *texture;

  /**
   * Has the buffer been modified, and needs to be copied into the
   * texture?
   */
  mutable bool dirty = true;
#elif defined(USE_GDI)
  BITMAPINFO bi;

  HBITMAP bitmap;
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

#if defined(ENABLE_OPENGL) || defined(USE_GDI)
  ~RawBitmap();
#endif

  /**
   * Returns the Buffer
   * @return The Buffer as RawColor array
   */
  RawColor *GetBuffer() {
#ifdef USE_GDI
    return buffer;
#else
    return buffer.get();
#endif
  }

  const RawColor *GetBuffer() const {
#ifdef USE_GDI
    return buffer;
#else
    return buffer.get();
#endif
  }

  /**
   * Returns a pointer to the top-most row.
   */
  RawColor *GetTopRow() {
#ifndef USE_GDI
    return GetBuffer();
#else
  /* in WIN32 bitmaps, the bottom-most row comes first */
    return GetBuffer() + (height - 1) * corrected_width;
#endif
  }

  /**
   * Returns a pointer to the row below the current one.
   */
  RawColor *GetNextRow(RawColor *row) {
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

#ifdef ENABLE_OPENGL
  /**
   * Bind the texture and return a reference to it.  If the texture is
   * "dirty", then the RAM buffer will be copied to the texture by
   * this method.
   */
  GLTexture &BindAndGetTexture() const;
#endif

  void StretchTo(unsigned width, unsigned height, Canvas &dest_canvas,
                 unsigned dest_width, unsigned dest_height,
                 bool transparent_white=false) const;

#ifdef ENABLE_OPENGL
private:
  /* from GLSurfaceListener */
  void SurfaceCreated() override;
  void SurfaceDestroyed() override;
#endif
};

#endif // !defined(AFX_STSCREENBUFFER_H__22D62F5D_32E2_4785_B3D9_2341C11F84A3__INCLUDED_)
