// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RotateDisplay.hpp"
#include "DisplayOrientation.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif

#ifdef KOBO
#include "system/FileUtil.hpp"
#include "Kobo/Model.hpp"
#endif

#ifdef ENABLE_OPENGL
#include "ui/opengl/Features.hpp"
#ifdef SOFTWARE_ROTATE_DISPLAY
#include "UIGlobals.hpp"
#include "ui/window/SingleWindow.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#endif
#endif

void
Display::RotateInitialize()
{
}

bool
Display::RotateSupported()
{
#if defined(ANDROID) || defined(KOBO)
  return true;
#elif defined(SOFTWARE_ROTATE_DISPLAY)
  /* rotate supported via glRotatef() (OpenGL projection matrix) */

  /* we need FBO so BufferCanvas can avoid using Canvas::CopyToTexture() */
  return true;
#else
  return false;
#endif
}

bool
Display::Rotate(DisplayOrientation orientation)
{
#if !defined(ANDROID) && !defined(KOBO) && !defined(SOFTWARE_ROTATE_DISPLAY)
  if (orientation == DisplayOrientation::DEFAULT)
    /* leave it as it is */
    return true;
#endif

#if defined(ANDROID)
  if (native_view == nullptr)
    return false;

  NativeView::ScreenOrientation android_orientation;
  switch (orientation) {
  case DisplayOrientation::PORTRAIT:
    android_orientation = NativeView::ScreenOrientation::PORTRAIT;
    break;

  case DisplayOrientation::LANDSCAPE:
    android_orientation = NativeView::ScreenOrientation::LANDSCAPE;
    break;

  case DisplayOrientation::REVERSE_PORTRAIT:
    android_orientation = NativeView::ScreenOrientation::REVERSE_PORTRAIT;
    break;

  case DisplayOrientation::REVERSE_LANDSCAPE:
    android_orientation = NativeView::ScreenOrientation::REVERSE_LANDSCAPE;
    break;

  default:
    android_orientation = NativeView::ScreenOrientation::LOCKED;
  };

  return native_view->SetRequestedOrientation(Java::GetEnv(),
                                              android_orientation);
#elif defined(KOBO)
  const char *rotate = "3";
  KoboModel kobo_model = DetectKoboModel();

  switch (orientation) {
  case DisplayOrientation::DEFAULT:
  case DisplayOrientation::PORTRAIT:
    switch(kobo_model) {
    case KoboModel::LIBRA2:
      rotate = "1";
      break;
    case KoboModel::LIBRA_H2O:
      rotate = "0";
      break;
    default:
      rotate = "3";
      break;
    }
    break;
  case DisplayOrientation::REVERSE_PORTRAIT:
    switch(kobo_model) {
    case KoboModel::LIBRA2:
      rotate = "3";
      break;
    case KoboModel::LIBRA_H2O:
      rotate = "2";
      break;
    default:
      rotate = "1";
      break;
    }
    break;

  case DisplayOrientation::LANDSCAPE:
    switch(kobo_model) {
    case KoboModel::LIBRA2:
      rotate = "2";
      break;
    case KoboModel::LIBRA_H2O:
      rotate = "1";
      break;
    default:
      rotate = "0";
      break;
    }
    break;

  case DisplayOrientation::REVERSE_LANDSCAPE:
    switch(kobo_model) {
    case KoboModel::LIBRA2:
      rotate = "0";
      break;
    case KoboModel::LIBRA_H2O:
      rotate = "3";
      break;
    default:
      rotate = "2";
      break;
    }
    break;
  };

  return File::WriteExisting(Path("/sys/class/graphics/fb0/rotate"), rotate);
#elif defined(SOFTWARE_ROTATE_DISPLAY)
  if (!RotateSupported())
    return false;

  UIGlobals::GetMainWindow().SetDisplayOrientation(orientation);
  return true;
#else
  return false;
#endif
}

bool
Display::RotateRestore()
{
#if defined(ANDROID)
  return native_view->SetRequestedOrientation(Java::GetEnv(),
                                              NativeView::ScreenOrientation::SENSOR);
#elif defined(KOBO)
  return Rotate(DisplayOrientation::DEFAULT);
#else
  return false;
#endif
}
