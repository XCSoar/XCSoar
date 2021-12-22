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

/*
 * This program loads the topography from a map file and exits.  Useful
 * for valgrind and profiling.
 */

#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyFile.hpp"
#include "Topography/XShape.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
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
  const std::lock_guard<Mutex> lock(file.mutex);

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

    {
      ConsoleOperationEnvironment operation;
      topography.Load(operation, reader, NULL, archive.get());
    }
  } else {
    FileLineReaderA reader{file};

    {
      ConsoleOperationEnvironment operation;
      topography.Load(operation, reader, directory, nullptr);
    }
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
