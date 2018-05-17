/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "DisplayGlue.hpp"
#include "RotateDisplay.hpp"
#include "Operation/VerboseOperationEnvironment.hpp"
#include "LogFile.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"

#ifdef USE_POLL_EVENT
#include "Event/Globals.hpp"
#include "Event/Queue.hpp"
#endif

void
Display::LoadOrientation(VerboseOperationEnvironment &env)
{
  if (!Display::RotateSupported())
    return;

  Display::RotateInitialize();

  DisplayOrientation orientation =
    CommonInterface::GetUISettings().display.orientation;
#ifdef KOBO
  /* on the Kobo, the display orientation must be loaded explicitly
     (portrait), because the hardware default is landscape */
#else
  if (orientation == DisplayOrientation::DEFAULT)
    return;
#endif

  if (!Display::Rotate(orientation)) {
    LogFormat("Display rotation failed");
    return;
  }

#ifdef USE_POLL_EVENT
  event_queue->SetDisplayOrientation(orientation);
#endif

  LogFormat("Display rotated");

  CommonInterface::main_window->Initialise();

  /* force the progress dialog to update its layout */
  env.UpdateLayout();
}

void
Display::RestoreOrientation()
{
  if (!Display::RotateSupported())
    return;

#ifndef KOBO
  DisplayOrientation orientation =
    CommonInterface::GetUISettings().display.orientation;
  if (orientation == DisplayOrientation::DEFAULT)
    return;
#endif

  Display::RotateRestore();

#ifdef USE_POLL_EVENT
  event_queue->SetDisplayOrientation(DisplayOrientation::DEFAULT);
#endif
}
