/**
 * @file STScreenBuffer.h
 * @details Class for pixel working with images. One could get access
 * to image bits using this library or create custom image.
 * @author Vassili Philippov (vasja@spbteam.com)
 * @date Created: June 2001
 * @date Last changed: 07 August 2001
 * @version 2.0
 */

#pragma once

#include "PortableColor.hpp"
#include "ui/dim/Size.hpp"

#ifdef USE_GDI
#include <windef.h>
#include <wingdi.h>
#else
#include <memory>
#endif

#include <cstdint>

struct PixelSize;
class Canvas;

#ifdef ENABLE_OPENGL
class GLTexture;
#endif

#if defined(ENABLE_OPENGL) && defined(__arm__) && !defined(ANDROID)
// use 16-bit RGB565 on non-Android ARM devices
#define USE_RGB565
#endif

/**
 * Greyscale, RGB565 and GDI RawColor types store no alpha, so fall back to an
 * opaque colormap there.
 */
constexpr bool
HaveBitmapSourceAlpha() noexcept
{
#if defined(GREYSCALE) || defined(USE_RGB565) || defined(USE_GDI)
  return false;
#else
  return true;
#endif
}

/**
 * The RawColor structure encapsulates color information about one
 * point in a #RawBitmap.
 */
struct RawColor
{
  RawColor() noexcept = default;

#ifdef GREYSCALE
  Luminosity8 value;

  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B) noexcept
    :value(R, G, B) {}

  /** Constructor with alpha (alpha is ignored on greyscale) */
  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B,
                     [[maybe_unused]] uint8_t A) noexcept
    :value(R, G, B) {}

#elif defined(USE_RGB565)

  RGB565Color value;

  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B) noexcept
    :value(R, G, B) {}

  /** Constructor with alpha (alpha is ignored on RGB565) */
  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B,
                     [[maybe_unused]] uint8_t A) noexcept
    :value(R, G, B) {}

#elif defined(ENABLE_OPENGL)

  RGB8Color value;
  uint8_t alpha;

  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B) noexcept
    :value(R, G, B), alpha(0xff) {}

  /** Constructor with alpha channel support */
  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B, uint8_t A) noexcept
    :value(R, G, B), alpha(A) {}

#elif defined(USE_MEMORY_CANVAS)

  BGR8Color value;
  uint8_t alpha;

  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B) noexcept
    :value(R, G, B), alpha(0xff) {}

  /** Constructor with alpha channel support */
  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B, uint8_t A) noexcept
    :value(R, G, B), alpha(A) {}

#elif defined(USE_GDI)

  BGR8Color value;

  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B) noexcept
    :value(R, G, B) {}

  /** Constructor with alpha (alpha is ignored on GDI) */
  constexpr RawColor(uint8_t R, uint8_t G, uint8_t B,
                     [[maybe_unused]] uint8_t A) noexcept
    :value(R, G, B) {}

#else
#error No implementation
#endif
};

/**
 * This class provides fast drawing methods and offscreen buffer.
 */
class RawBitmap final
{
  const PixelSize size;

#ifdef USE_GDI
  const unsigned corrected_width;
  RawColor *buffer;
#else
  const std::unique_ptr<RawColor[]> buffer;
#endif

#ifdef ENABLE_OPENGL
  const std::unique_ptr<GLTexture> texture;

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
   * Creates buffer with the given size.
   */
  explicit RawBitmap(PixelSize size) noexcept;

#if defined(ENABLE_OPENGL) || defined(USE_GDI)
  ~RawBitmap() noexcept;
#endif

  /**
   * Returns the Buffer
   * @return The Buffer as RawColor array
   */
  RawColor *GetBuffer() noexcept {
#ifdef USE_GDI
    return buffer;
#else
    return buffer.get();
#endif
  }

  const RawColor *GetBuffer() const noexcept {
#ifdef USE_GDI
    return buffer;
#else
    return buffer.get();
#endif
  }

  /**
   * Returns a pointer to the top-most row.
   */
  RawColor *GetTopRow() noexcept {
#ifndef USE_GDI
    return GetBuffer();
#else
  /* in WIN32 bitmaps, the bottom-most row comes first */
    return GetBuffer() + (size.height - 1) * corrected_width;
#endif
  }

  /**
   * Returns a pointer to the row below the current one.
   */
  RawColor *GetNextRow(RawColor *row) noexcept {
#ifndef USE_GDI
    return row + size.width;
#else
    return row - corrected_width;
#endif
  }

  void SetDirty() noexcept {
#ifdef ENABLE_OPENGL
    dirty = true;
#endif
  }

  PixelSize GetSize() const noexcept {
    return size;
  }

#ifdef ENABLE_OPENGL
  /**
   * Bind the texture and return a reference to it.  If the texture is
   * "dirty", then the RAM buffer will be copied to the texture by
   * this method.
   */
  GLTexture &BindAndGetTexture() const noexcept;
#endif

  /**
   * Stretch and position this bitmap to the destination canvas.
   *
   * @param use_source_alpha if true, use per-pixel alpha from source for
   *                         blending; takes precedence over transparent_white
   * @param transparent_white if true, white pixels are treated as transparent
   *                         (only effective when not use_source_alpha)
   * @param alpha overall layer opacity (0.0=transparent, 1.0=opaque);
   *              on the memory canvas, ignored when use_source_alpha
   *              is set (combining both is not supported)
   */
  void StretchTo(PixelSize src_size,
                 Canvas &dest_canvas, PixelSize dest_size,
                 bool transparent_white=false,
                 bool use_source_alpha=false,
                 float alpha=1.0f) const noexcept;
};
