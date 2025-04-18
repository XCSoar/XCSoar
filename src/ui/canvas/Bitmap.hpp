// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Point.hpp"
#include "ui/dim/Size.hpp"

#ifdef USE_MEMORY_CANVAS
#include "ui/canvas/memory/Buffer.hpp"
#include "ui/canvas/memory/PixelTraits.hpp"
#endif

#ifdef ANDROID
#include <jni.h>
#endif

#ifdef USE_GDI
#include <windef.h>
#endif

#include <cassert>
#include <span>

class Path;
class ResourceId;
class UncompressedImage;
struct GeoQuadrilateral;

#ifdef ENABLE_OPENGL
class GLTexture;
#elif !defined(USE_GDI)
#ifdef GREYSCALE
using BitmapPixelTraits = GreyscalePixelTraits;
#else
using BitmapPixelTraits = BGRAPixelTraits;
#endif
#endif

/**
 * An image loaded from storage.
 */
class Bitmap final
{
public:
  enum class Type {
    /**
     * A standard bitmap that will be blitted to the screen.  After
     * loading, it will be converted to the screen's pixel format.
     */
    STANDARD,

    /**
     * A monochrome bitmap (1 bit per pixel).
     */
    MONO,
  };

protected:
#ifdef ENABLE_OPENGL
  GLTexture *texture = nullptr;
  PixelSize size;

  /**
   * Flip up/down?  Some image formats (such as BMP and TIFF) store
   * the bottom-most row first.
   */
  bool flipped = false;
#elif defined(USE_MEMORY_CANVAS)
  WritableImageBuffer<BitmapPixelTraits> buffer = WritableImageBuffer<BitmapPixelTraits>::Empty();
#else
  HBITMAP bitmap = nullptr;
#endif

public:
  Bitmap() = default;
  explicit Bitmap(ResourceId id);

#if !defined(USE_GDI) && !defined(ANDROID)
  Bitmap(std::span<const std::byte> buffer);
#endif

  Bitmap(Bitmap &&src) noexcept;

  ~Bitmap() noexcept {
    Reset();
  }

  Bitmap &operator=(Bitmap &&src) noexcept;

  bool IsDefined() const noexcept {
#ifdef ENABLE_OPENGL
    return texture != nullptr;
#elif defined(USE_MEMORY_CANVAS)
    return buffer.data != nullptr;
#else
    return bitmap != nullptr;
#endif
  }

#ifdef USE_MEMORY_CANVAS
  void Create(PixelSize _size) noexcept {
    assert(!IsDefined());

    buffer.Allocate(_size);
  }
#endif

#ifdef ENABLE_OPENGL
  const PixelSize &GetSize() const noexcept {
    return size;
  }

  unsigned GetWidth() const noexcept {
    return size.width;
  }

  unsigned GetHeight() const noexcept {
    return size.height;
  }

  bool IsFlipped() const noexcept {
    return flipped;
  }
#elif defined(USE_MEMORY_CANVAS)
  const PixelSize &GetSize() const noexcept {
    return buffer.size;
  }

  unsigned GetWidth() const noexcept {
    return buffer.size.width;
  }

  unsigned GetHeight() const noexcept {
    return buffer.size.height;
  }
#else
  [[gnu::pure]]
  PixelSize GetSize() const noexcept;

  unsigned GetWidth() const noexcept {
    return GetSize().width;
  }

  unsigned GetHeight() const noexcept {
    return GetSize().height;
  }
#endif

#ifndef USE_GDI
  bool Load(UncompressedImage &&uncompressed, Type type=Type::STANDARD);
#ifndef ANDROID
  bool Load(std::span<const std::byte> buffer, Type type=Type::STANDARD);
#endif
#endif

  bool Load(ResourceId id, Type type=Type::STANDARD);

#ifndef ENABLE_OPENGL
  /**
   * Load a bitmap and stretch it by the specified zoom factor.
   */
  bool LoadStretch(ResourceId id, unsigned zoom);
#endif

  bool LoadFile(Path path);

  /**
   * Load a georeferenced image (e.g. GeoTIFF) and return its bounds.
   * Throws a std::runtime_error on error.
   */
  GeoQuadrilateral LoadGeoFile(Path path);

  void Reset() noexcept;

#ifdef ENABLE_OPENGL
  GLTexture *GetNative() const noexcept {
    return texture;
  }
#elif defined(USE_MEMORY_CANVAS)
  ConstImageBuffer<BitmapPixelTraits> GetNative() const noexcept {
    return buffer;
  }
#else
  HBITMAP GetNative() const noexcept {
    assert(IsDefined());

    return bitmap;
  }
#endif

#ifdef ENABLE_OPENGL
private:
  bool MakeTexture(const UncompressedImage &uncompressed, Type type) noexcept;

#ifdef ANDROID
  bool Set(JNIEnv *env, jobject _bmp, Type _type,
           bool flipped = false) noexcept;
  bool MakeTexture(jobject _bmp, Type _type,
                   bool flipped = false) noexcept;
#endif
#endif
};
