// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/opengl/Texture.hpp"
#include "Android/Bitmap.hpp"
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#include "system/Path.hpp"
#include "ResourceId.hpp"

#include <android/bitmap.h>
#include <cstdint>

Bitmap::Bitmap(ResourceId id)
{
  Load(id);
}

static Java::LocalObject
LoadResourceBitmap(ResourceId id)
{
  return native_view->LoadResourceBitmap(Java::GetEnv(),
                                         static_cast<const char *>(id));
}

/**
 * Lock the Java Bitmap pixel buffer and scan for non-grayscale
 * pixels.  Returns false when the bitmap cannot be locked or
 * contains only grayscale data.
 */
static bool
ScanBitmapForColors(JNIEnv *env, jobject bmp) noexcept
{
  AndroidBitmapInfo info;
  if (AndroidBitmap_getInfo(env, bmp, &info) != ANDROID_BITMAP_RESULT_SUCCESS)
    return false;

  void *pixels;
  if (AndroidBitmap_lockPixels(env, bmp, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS)
    return false;

  bool found = false;
  const auto *data = static_cast<const uint8_t *>(pixels);

  for (unsigned y = 0; y < info.height && !found; y++) {
    const auto *row = data + y * info.stride;
    for (unsigned x = 0; x < info.width && !found; x++) {
      uint8_t r, g, b;

      switch (info.format) {
      case ANDROID_BITMAP_FORMAT_RGBA_8888:
        r = row[x * 4];
        g = row[x * 4 + 1];
        b = row[x * 4 + 2];
        break;
      case ANDROID_BITMAP_FORMAT_RGB_565: {
        uint16_t pixel;
        memcpy(&pixel, &row[x * 2], sizeof(pixel));
        r = (pixel >> 11) & 0x1F;
        g = (pixel >> 5) & 0x3F;
        b = pixel & 0x1F;
        /* scale green to 5 bits so all channels are comparable */
        g >>= 1;
        break;
      }
      default:
        /* ALPHA_8 or unknown – treat as grayscale */
        goto unlock;
      }

      if (r != g || r != b)
        found = true;
    }
  }

unlock:
  AndroidBitmap_unlockPixels(env, bmp);
  return found;
}

bool
Bitmap::Set(JNIEnv *env, jobject bmp, Type type, bool flipped) noexcept
{
  assert(bmp != nullptr);

  size.width = AndroidBitmap::GetWidth(env, bmp);
  size.height = AndroidBitmap::GetHeight(env, bmp);

  has_colors = ScanBitmapForColors(env, bmp);

  if (!MakeTexture(bmp, type, flipped)) {
    Reset();
    return false;
  }

  return true;
}

bool
Bitmap::MakeTexture(jobject _bmp, Type _type, bool flipped) noexcept
{
  assert(_bmp != nullptr);

  jint result[5];
  if (!native_view->BitmapToTexture(Java::GetEnv(), _bmp,
                                    _type == Bitmap::Type::MONO, result))
    return false;

  texture = new GLTexture(result[0], PixelSize(result[1], result[2]),
                          PixelSize(result[3], result[4]), flipped);
  return true;
}

bool
Bitmap::Load(ResourceId id, Type _type)
{
  assert(id.IsDefined());

  Reset();

  auto new_bmp = LoadResourceBitmap(id);
  if (new_bmp == nullptr)
    return false;

  return Set(new_bmp.GetEnv(), new_bmp, _type);
}

bool
Bitmap::LoadFile(Path path)
{
  assert(path != nullptr && !path.empty());

  Reset();

  Java::LocalObject new_bmp;
  bool flipped = false;
  if (path.EndsWithIgnoreCase(".tif") || path.EndsWithIgnoreCase(".tiff")) {
    new_bmp = native_view->LoadFileTiff(Java::GetEnv(), path);
    flipped = true;
  } else {
    new_bmp = native_view->LoadFileBitmap(Java::GetEnv(), path);
  }
  if (new_bmp == nullptr)
    return false;

  return Set(Java::GetEnv(), new_bmp, Type::STANDARD, flipped);
}
