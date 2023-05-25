// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/opengl/Texture.hpp"
#include "Android/Bitmap.hpp"
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#include "system/Path.hpp"
#include "ResourceId.hpp"
#include "android_drawable.h"

Bitmap::Bitmap(ResourceId id)
{
  Load(id);
}

static const char *
find_resource_name(unsigned id) noexcept
{
  for (unsigned i = 0; DrawableNames[i].name != nullptr; ++i)
    if (DrawableNames[i].id == id)
      return DrawableNames[i].name;

  return nullptr;
}

static Java::LocalObject
LoadResourceBitmap(ResourceId id)
{
  const char *name = find_resource_name((unsigned)id);
  if (name == nullptr)
    return nullptr;

  return native_view->LoadResourceBitmap(Java::GetEnv(), name);
}

bool
Bitmap::Set(JNIEnv *env, jobject bmp, Type type, bool flipped) noexcept
{
  assert(bmp != nullptr);

  size.width = AndroidBitmap::GetWidth(env, bmp);
  size.height = AndroidBitmap::GetHeight(env, bmp);

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
