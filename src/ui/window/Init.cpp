// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Init.hpp"
#include "Screen/Debug.hpp"
#include "ui/event/Globals.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Globals.hpp"
#endif

#ifdef USE_FREETYPE
#include "ui/canvas/Font.hpp"
#endif

#ifdef KOBO
#include "Hardware/RotateDisplay.hpp"
#include "DisplayOrientation.hpp"
#endif

ScreenGlobalInit::ScreenGlobalInit([[maybe_unused]] unsigned antialiasing_samples)
#ifdef ANDROID
  :display(EGL_DEFAULT_DISPLAY, antialiasing_samples)
#elif defined(ENABLE_SDL) || defined(USE_GLX) || defined(MESA_KMS) || \
  defined(USE_WAYLAND) || (defined(USE_EGL) && defined(USE_X11))
  /* the stub UI::Display used by the framebuffer targets (Kobo, VFB)
     has no constructor taking a sample count */
  :display(antialiasing_samples)
#endif
{
#ifdef ENABLE_OPENGL
  /* remember what was asked for: the window surface may have fallen
     back to fewer samples (or to none), but a framebuffer object can
     still provide what the profile asks for */
  OpenGL::requested_antialiasing_samples = antialiasing_samples;
#endif

#ifdef USE_FREETYPE
  Font::Initialise();
#endif

  UI::event_queue = &event_queue;

#ifdef KOBO
  Display::Rotate(DisplayOrientation::DEFAULT);
  UI::event_queue->SetDisplayOrientation(DisplayOrientation::DEFAULT);
#endif

  ScreenInitialized();
}

ScreenGlobalInit::~ScreenGlobalInit()
{
  UI::event_queue = nullptr;

#ifdef USE_FREETYPE
  Font::Deinitialise();
#endif

  ScreenDeinitialized();
}
