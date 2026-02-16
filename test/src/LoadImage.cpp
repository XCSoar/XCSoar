// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * Load an image from a resource or a file and exit.
 *
 */

#define ENABLE_SCREEN
#define ENABLE_CMDLINE
#define USAGE "{PATH|ID}"

#include "Main.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "util/NumberParser.hpp"
#include "util/StringCompare.hxx"
#include "ResourceId.hpp"

static AllocatedPath path = nullptr;

#ifdef USE_GDI
static ResourceId id = ResourceId::Null();
#endif

static void
ParseCommandLine(Args &args)
{
  path = args.ExpectNextPath();

#ifdef USE_GDI
  char *endptr;
  unsigned _id = ParseUnsigned(path.c_str(), &endptr);
  if (StringIsEmpty(endptr))
    id = ResourceId(_id);
#endif
}

static void
Main([[maybe_unused]] UI::Display &display)
{
  Bitmap bitmap;
  bool success =
#ifdef USE_GDI
    id.IsDefined()
    ? bitmap.Load(id)
    :
#endif
    bitmap.LoadFile(path);
  if (!success)
    fprintf(stderr, "Failed to load image\n");
}
