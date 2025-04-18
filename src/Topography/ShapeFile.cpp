// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
