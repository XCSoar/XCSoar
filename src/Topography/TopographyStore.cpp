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

#include "Topography/TopographyStore.hpp"
#include "Topography/TopographyFile.hpp"
#include "Index.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "io/LineReader.hpp"
#include "system/ConvertPathName.hpp"
#include "system/Path.hpp"
#include "Operation/Operation.hpp"
#include "Compatibility/path.h"
#include "LogFile.hpp"

#include <cstdint>

#include <windef.h> // for MAX_PATH

double
TopographyStore::GetNextScaleThreshold(double map_scale) const noexcept
{
  double result(-1);
  for (auto *file : files) {
    double threshold = file->GetNextScaleThreshold(map_scale);
    if (threshold > result)
      result = threshold;
  }

  return result;
}

unsigned
TopographyStore::ScanVisibility(const WindowProjection &m_projection,
                                unsigned max_update) noexcept
{
  // check if any needs to have cache updates because wasnt
  // visible previously when bounds moved

  // we will make sure we update at least one cache per call
  // to make sure eventually everything gets refreshed
  unsigned num_updated = 0;
  for (auto *file : files) {
    try {
      if (file->Update(m_projection)) {
        ++num_updated;
        if (num_updated >= max_update)
          break;
      }
    } catch (...) {
      LogError(std::current_exception());
    }
  }

  serial += num_updated;
  return num_updated;
}

void
TopographyStore::LoadAll() noexcept
{
  for (const auto &i : files) {
    TopographyFile &file = *i;
    file.LoadAll();
  }
}

void
TopographyStore::Load(OperationEnvironment &operation, NLineReader &reader,
                      Path directory, struct zzip_dir *zdir) noexcept
{
  Reset();

  // Create buffer for the shape filenames
  // (shape_filename will be modified with the shape_filename_end pointer)
  char shape_filename[MAX_PATH];
  if (directory != nullptr) {
    const NarrowPathName narrow_directory(directory);
    strcpy(shape_filename, narrow_directory);
    strcat(shape_filename, DIR_SEPARATOR_S);
  } else
    shape_filename[0] = 0;

  char *shape_filename_end = shape_filename + strlen(shape_filename);

  // Read file size to have a rough progress estimate for the progress bar
  const long filesize = std::max(reader.GetSize(), 1l);

  // Set progress bar to 100 steps
  operation.SetProgressRange(100);

  // Iterate through shape files in the "topology.tpl" file until
  // end or max. file number reached
  char *line;
  while (!files.full() && (line = reader.ReadLine()) != nullptr) {
    // .tpl Line format: filename,range,icon,field,r,g,b,pen_width,label_range,important_range,alpha

    const auto entry = ParseTopographyIndexLine(line, shape_filename_end);
    if (!entry)
      continue;

    // Create TopographyFile instance from parsed line
    try {
      TopographyFile *file = new TopographyFile(zdir, shape_filename,
                                                entry->shape_range,
                                                entry->label_range,
                                                entry->important_label_range,
                                                entry->color,
                                                entry->shape_field,
                                                entry->icon, entry->big_icon,
                                                entry->pen_width);
      files.append(file);
    } catch (...) {
      LogError(std::current_exception());
    }

    // Update progress bar
    operation.SetProgressPosition((reader.Tell() * 100) / filesize);
  }
}

void
TopographyStore::Reset() noexcept
{
  for (auto *file : files)
    delete file;

  files.clear();
}
