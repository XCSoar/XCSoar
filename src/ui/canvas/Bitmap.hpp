/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

class Path;
class ResourceId;
class UncompressedImage;
struct GeoQuadrilateral;
template<typename T> struct ConstBuffer;

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

  bool interpolation = false;

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
  Bitmap(ConstBuffer<void> buffer);
#endif

  Bitmap(Bitmap &&src) noexcept;

  ~Bitmap() noexcept {
    Reset();
  }

  Bitmap &operator=(Bitmap &&src) noexcept;

public:
  bool IsDefined() const noexcept {
#ifdef ENABLE_OPENGL
    return texture != nullptr;
#elif defined(USE_MEMORY_CANVAS)
    return buffer.data != nullptr;
#else
    return bitmap != nullptr;
#endif
  }

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
  PixelSize GetSize() const noexcept {
    return { buffer.width, buffer.height };
  }

  unsigned GetWidth() const noexcept {
    return buffer.width;
  }

  unsigned GetHeight() const noexcept {
    return buffer.height;
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

#ifdef ENABLE_OPENGL
  void EnableInterpolation() noexcept;
#else
  void EnableInterpolation() noexcept {}
#endif

#ifndef USE_GDI
  bool Load(UncompressedImage &&uncompressed, Type type=Type::STANDARD);
#ifndef ANDROID
  bool Load(ConstBuffer<void> buffer, Type type=Type::STANDARD);
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
