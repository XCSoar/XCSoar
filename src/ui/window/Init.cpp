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

#ifdef USE_GDI
#include "ui/canvas/gdi/GdiPlusBitmap.hpp"
#endif

#ifdef USE_WINUSER
#include "PaintWindow.hpp"
#include "SingleWindow.hpp"

#include <libloaderapi.h>
#endif

ScreenGlobalInit::ScreenGlobalInit()
#ifdef ANDROID
  :display(EGL_DEFAULT_DISPLAY)
#endif
{
#ifdef USE_FREETYPE
  Font::Initialise();
#endif

#ifdef USE_GDI
  GdiStartup();
#endif

  UI::event_queue = &event_queue;

#ifdef KOBO
  Display::Rotate(DisplayOrientation::DEFAULT);
  UI::event_queue->SetDisplayOrientation(DisplayOrientation::DEFAULT);
#endif

#ifdef USE_WINUSER
  HINSTANCE hInstance = ::GetModuleHandle(nullptr);
  PaintWindow::register_class(hInstance);
  UI::SingleWindow::RegisterClass(hInstance);
#endif

  ScreenInitialized();
}

ScreenGlobalInit::~ScreenGlobalInit()
{
  UI::event_queue = nullptr;

#ifdef USE_GDI
  GdiShutdown();
#endif

#ifdef USE_FREETYPE
  Font::Deinitialise();
#endif

  ScreenDeinitialized();
}
