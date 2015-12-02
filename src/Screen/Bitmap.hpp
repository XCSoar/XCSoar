/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_SCREEN_BITMAP_HPP
#define XCSOAR_SCREEN_BITMAP_HPP

#include "Screen/Point.hpp"

#ifdef USE_MEMORY_CANVAS
#include "Screen/Memory/Buffer.hpp"
#include "Screen/Memory/PixelTraits.hpp"
#endif

#ifdef ANDROID
#include "Screen/OpenGL/Surface.hpp"
#include <jni.h>
#endif

#ifdef USE_GDI
#include <windows.h>
#endif

#include <assert.h>

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
#ifdef ANDROID
             : private GLSurfaceListener
#endif
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
#ifdef ANDROID
  jobject bmp = nullptr;

  Type type;
#endif

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

#ifdef USE_MEMORY_CANVAS
  Bitmap(Bitmap &&src) = default;
#else
  Bitmap(Bitmap &&src);
#endif

  ~Bitmap() {
    Reset();
  }

  Bitmap(const Bitmap &other) = delete;
  Bitmap &operator=(const Bitmap &other) = delete;
public:
  bool IsDefined() const {
#ifdef ANDROID
    return bmp != nullptr;
#elif defined(ENABLE_OPENGL)
    return texture != nullptr;
#elif defined(USE_MEMORY_CANVAS)
    return buffer.data != nullptr;
#else
    return bitmap != nullptr;
#endif
  }

#ifdef ENABLE_OPENGL
  unsigned GetWidth() const {
    return size.cx;
  }

  unsigned GetHeight() const {
    return size.cy;
  }

  bool IsFlipped() const {
    return flipped;
  }
#elif defined(USE_MEMORY_CANVAS)
  unsigned GetWidth() const {
    return buffer.width;
  }

  unsigned GetHeight() const {
    return buffer.height;
  }
#else
  unsigned GetWidth() const {
    return GetSize().cx;
  }

  unsigned GetHeight() const {
    return GetSize().cy;
  }
#endif

#ifdef ENABLE_OPENGL
  void EnableInterpolation();
#else
  void EnableInterpolation() {}
#endif

#ifndef USE_GDI
  bool Load(const UncompressedImage &uncompressed, Type type=Type::STANDARD);
#ifndef ANDROID
  bool Load(ConstBuffer<void> buffer, Type type=Type::STANDARD);
#endif
#endif

  bool Load(ResourceId id, Type type=Type::STANDARD);

  /**
   * Load a bitmap and stretch it by the specified zoom factor.
   */
  bool LoadStretch(ResourceId id, unsigned zoom);

  bool LoadFile(Path path);

  /**
   * Load a georeferenced image (e.g. GeoTIFF) and return its bounds.
   * Throws a std::runtime_error on error.
   */
  GeoQuadrilateral LoadGeoFile(Path path);

  void Reset();

  gcc_pure
  const PixelSize GetSize() const;

#ifdef ENABLE_OPENGL
  GLTexture *GetNative() const {
    return texture;
  }
#elif defined(USE_MEMORY_CANVAS)
  ConstImageBuffer<BitmapPixelTraits> GetNative() const {
    return buffer;
  }
#else
  HBITMAP GetNative() const {
    assert(IsDefined());

    return bitmap;
  }
#endif

#ifdef ANDROID
private:
  bool Set(JNIEnv *env, jobject _bmp, Type _type);
  bool MakeTexture();

  /* from GLSurfaceListener */
  virtual void SurfaceCreated() override;
  virtual void SurfaceDestroyed() override;
#endif
};

#endif
