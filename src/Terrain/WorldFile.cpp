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

#include "WorldFile.hpp"
#include "Geo/GeoBounds.hpp"
#include "IO/ZipLineReader.hpp"
#include "Util/NumberParser.hpp"

#include <algorithm>
#include <stdexcept>

static constexpr bool
IsNearZero(double value)
{
  return value > -0.01 && value < 0.01;
}

struct WorldFileData {
  double x_scale, y_rotation, x_rotation, y_scale, x_origin, y_origin;

  constexpr bool IsRotated() const {
    return !IsNearZero(y_rotation) || !IsNearZero(x_rotation);
  }

  GeoBounds ToGeoBounds(unsigned width, unsigned height) const {
    const Angle lon_min(Angle::Degrees(x_origin));
    const Angle lon_max(Angle::Degrees(x_origin + width * x_scale));
    const Angle lat_min(Angle::Degrees(y_origin));
    const Angle lat_max(Angle::Degrees(y_origin + height * y_scale));

    return GeoBounds(GeoPoint(std::min(lon_min, lon_max),
                              std::max(lat_min, lat_max)),
                     GeoPoint(std::max(lon_min, lon_max),
                              std::min(lat_min, lat_max)));
  }
};

static bool
ReadLine(NLineReader &reader, double &value_r)
{
  const char *line = reader.ReadLine(); // x scale
  if (line == nullptr)
    return false;

  char *endptr;
  double value = ParseDouble(line, &endptr);
  if (endptr == line)
    return false;

  value_r = value;
  return true;
}

static bool
ReadWorldFile(NLineReader &reader, WorldFileData &data)
{
  return ReadLine(reader, data.x_scale) &&
    ReadLine(reader, data.y_rotation) &&
    ReadLine(reader, data.x_rotation) &&
    ReadLine(reader, data.y_scale) &&
    ReadLine(reader, data.x_origin) &&
    ReadLine(reader, data.y_origin);
}

static bool
ReadWorldFile(struct zzip_dir *dir, const char *path, WorldFileData &data)
try {
  ZipLineReaderA reader(dir, path);
  return ReadWorldFile(reader, data);
} catch (const std::runtime_error &e) {
  return false;
}

GeoBounds
LoadWorldFile(struct zzip_dir *dir, const char *path,
              unsigned width, unsigned height)
{
  WorldFileData data;
  if (!ReadWorldFile(dir, path, data) ||
      /* we don't support rotation */
      data.IsRotated())
    return GeoBounds::Invalid();

  return data.ToGeoBounds(width, height);
}

