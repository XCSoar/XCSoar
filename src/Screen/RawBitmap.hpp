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

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Surface.hpp"
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/Features.hpp"
#endif

/**
 * BGRColor structure encapsulates color information about one point. Color
 * order is Blue, Green, Red (not RGB).
 */
struct BGRColor
{
  BGRColor() {}

#ifdef HAVE_GLES
  /**
   * RGB color value encoded with 5/6/5 bits per channel.
   */
  unsigned short value;

  BGRColor(unsigned char R, unsigned char G, unsigned char B)
    :value(((R & 0xf8) << 8) |
           ((G & 0xfc) << 3) |
           (B >> 3)) {}

#elif defined(ENABLE_SDL)

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  unsigned char dummy;
  unsigned char r;
  unsigned char g;
  unsigned char b;

  BGRColor(unsigned char R, unsigned char G, unsigned char B)
    :r(R), g(G), b(B) {}
#else /* little endian */
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char dummy;

  BGRColor(unsigned char R, unsigned char G, unsigned char B)
    :b(B), g(G), r(R) {}
#endif /* little endian */

#else /* !SDL */

#ifdef _WIN32_WCE
  /**
   * RGB color value encoded with 5/5/5 bits per channel.  The most
   * significant bit is unused.
   */
  unsigned short value;

  BGRColor(unsigned char R, unsigned char G, unsigned char B)
    :value(((R & 0xf8) << 7) |
           ((G & 0xf8) << 2) |
           (B >> 3)) {}

#else /* !_WIN32_WCE */
  unsigned char b;
  unsigned char g;
  unsigned char r;

  BGRColor(unsigned char R, unsigned char G, unsigned char B)
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
  const unsigned int width;
  const unsigned int height;
  const unsigned int corrected_width;
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
  RawBitmap(unsigned width, unsigned height);

#ifdef ENABLE_OPENGL
  virtual
#endif
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
#ifdef ENABLE_OPENGL
    texture->bind();

    if (dirty) {
#ifdef HAVE_GLES
      /* 16 bit 5/6/5 on Android */
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, corrected_width, this->height,
                      GL_RGB, GL_UNSIGNED_SHORT_5_6_5, buffer);
#else
      /* 32 bit R/G/B/A on full OpenGL */
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, corrected_width, this->height,
                      GL_BGRA, GL_UNSIGNED_BYTE, buffer);
#endif

      dirty = false;
    }

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    GLEnable scope(GL_TEXTURE_2D);
    dest_canvas.stretch(0, 0, dest_width, dest_height,
                        *texture, 0, 0, width, height);
#elif defined(ENABLE_SDL)
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

#ifdef ENABLE_OPENGL
private:
  /* from GLSurfaceListener */
  virtual void surface_created();
  virtual void surface_destroyed();
#endif
};

#endif // !defined(AFX_STSCREENBUFFER_H__22D62F5D_32E2_4785_B3D9_2341C11F84A3__INCLUDED_)
