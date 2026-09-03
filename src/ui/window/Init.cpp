// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Init.hpp"
#include "Screen/Debug.hpp"
#include "ui/event/Globals.hpp"

#ifdef USE_FREETYPE
#include "ui/canvas/Font.hpp"
#endif

#ifdef KOBO
#include "Hardware/RotateDisplay.hpp"
#include "DisplayOrientation.hpp"
#endif

ScreenGlobalInit::ScreenGlobalInit()
#ifdef ANDROID
  :display(EGL_DEFAULT_DISPLAY)
#endif
{
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
