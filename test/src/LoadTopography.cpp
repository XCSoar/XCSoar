// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * This program loads the topography from a map file and exits.  Useful
 * for valgrind and profiling.
 */

#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyFile.hpp"
#include "Topography/XShape.hpp"
#include "system/Args.hpp"
#include "io/FileLineReader.hpp"
#include "io/ZipArchive.hpp"
#include "io/ZipLineReader.hpp"
#include "util/PrintException.hxx"

#include <stdio.h>
#include <tchar.h>

#ifdef ENABLE_OPENGL

static const uint16_t *
TriangulateAll(const TopographyFile &file)
{
  const std::lock_guard lock{file.mutex};

  const uint16_t *dummy = nullptr;
  for (const XShape &shape : file)
    if (shape.get_type() == MS_SHAPE_POLYGON)
      for (unsigned i = 0; i < 4; ++i)
        dummy = shape.GetIndices(i, 1).indices;

  return dummy;
}

static void
TriangulateAll(const TopographyStore &store)
{
  for (auto &i : store)
    TriangulateAll(i);
}

#endif

int main(int argc, char **argv)
try {
  Args args(argc, argv, "{FILE.xcm | FILE.tpl PATH}");
  const auto file = args.ExpectNextPath();
  decltype(args.ExpectNextPath()) directory{};
  if (!args.IsEmpty())
    directory = args.ExpectNextPath();
  args.ExpectEnd();

  TopographyStore topography;

  if (directory == nullptr) {
    ZipArchive archive(file);

    ZipLineReaderA reader(archive.get(), "topology.tpl");
    topography.Load(reader, NULL, archive.get());
  } else {
    FileLineReaderA reader{file};
    topography.Load(reader, directory, nullptr);
  }

  topography.LoadAll();

#ifdef ENABLE_OPENGL
  TriangulateAll(topography);
#endif

  return EXIT_SUCCESS;
} catch (const std::runtime_error &e) {
  PrintException(e);
  return EXIT_FAILURE;
}
