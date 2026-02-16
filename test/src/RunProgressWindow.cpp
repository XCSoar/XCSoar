// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_SCREEN
#define ENABLE_MAIN_WINDOW

#include "Main.hpp"
#include "ProgressWindow.hpp"

static void
Main(TestMainWindow &main_window)
{
  ProgressWindow progress(main_window);
  progress.SetMessage("Testing...");
  progress.SetRange(0, 1024);
  progress.SetValue(768);

  main_window.SetFullWindow(progress);

  main_window.RunEventLoop();
}
