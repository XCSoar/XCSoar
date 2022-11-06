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

#include "ShapeFile.hpp"

#include <stdexcept>

ShapeFile::ShapeFile(zzip_dir *dir, const char *filename)
{
  if (msShapefileOpen(&obj, "rb", dir, filename, 0) == -1)
    throw std::runtime_error{"Failed to open shapefile"};
}

void
ShapeFile::ReadShape(shapeObj &shape, std::size_t i)
{
  msSHPReadShape(obj.hSHP, i, &shape);
  if (shape.type == MS_SHAPE_NULL)
    throw std::runtime_error{"Failed to read shape"};
}

const char *
ShapeFile::ReadLabel(std::size_t i, unsigned field) noexcept
{
  return msDBFReadStringAttribute(obj.hDBF, i, field);
}
