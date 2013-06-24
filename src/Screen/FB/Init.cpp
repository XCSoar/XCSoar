/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Screen/Init.hpp"
#include "Event/Console/Globals.hpp"
#include "Event/Console/Queue.hpp"
#include "Screen/Debug.hpp"
#include "Screen/Font.hpp"
#include "Asset.hpp"

#ifdef KOBO
#include "OS/FileUtil.hpp"
#endif

#ifdef USE_FREETYPE
#include "Screen/FreeType/Init.hpp"
#endif

ScreenGlobalInit::ScreenGlobalInit()
{
#ifdef KOBO
  /* switch to portrait mode */
  File::WriteExisting("/sys/class/graphics/fb0/rotate", "3");
#endif

#ifdef USE_FREETYPE
  FreeType::Initialise();
#endif

  Font::Initialise();

  event_queue = new EventQueue();

  ScreenInitialized();
}

ScreenGlobalInit::~ScreenGlobalInit()
{
  delete event_queue;
  event_queue = nullptr;

#ifdef USE_FREETYPE
  FreeType::Deinitialise();
#endif

  ScreenDeinitialized();
}
