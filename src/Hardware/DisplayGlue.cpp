// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplayGlue.hpp"
#include "RotateDisplay.hpp"
#include "Operation/VerboseOperationEnvironment.hpp"
#include "LogFile.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"

#ifdef USE_POLL_EVENT
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
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
    LogString("Display rotation failed");
    return;
  }

#ifdef USE_POLL_EVENT
  UI::event_queue->SetDisplayOrientation(orientation);
#endif

  LogString("Display rotated");

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
  UI::event_queue->SetDisplayOrientation(DisplayOrientation::DEFAULT);
#endif
}

DisplayOrientation
Display::DetectInitialOrientation()
{
  auto orientation = DisplayOrientation::DEFAULT;

#ifdef MESA_KMS
  // When running in DRM/KMS mode, infer the display orientation from the linux
  // console rotation.
  char buf[3];
  auto rotatepath = Path("/sys/class/graphics/fbcon/rotate");
  if (File::ReadString(rotatepath, buf, sizeof(buf))) {
    switch (*buf) {
    case '0': orientation = DisplayOrientation::LANDSCAPE; break;
    case '1': orientation = DisplayOrientation::REVERSE_PORTRAIT; break;
    case '2': orientation = DisplayOrientation::REVERSE_LANDSCAPE; break;
    case '3': orientation = DisplayOrientation::PORTRAIT; break;
    }
  }
#endif
  return orientation;
}
