// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * Load an image from a resource or a file and exit.
 *
 */

#define ENABLE_SCREEN
#define ENABLE_CMDLINE
#define USAGE "{PATH}"

#include "Main.hpp"
#include "ui/canvas/Bitmap.hpp"

static AllocatedPath path = nullptr;

static void
ParseCommandLine(Args &args)
{
  path = args.ExpectNextPath();
}

static void
Main([[maybe_unused]] UI::Display &display)
{
  Bitmap bitmap;
  bool success = bitmap.LoadFile(path);
  if (!success)
    fprintf(stderr, "Failed to load image\n");
}
