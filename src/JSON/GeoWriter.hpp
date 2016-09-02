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

#ifndef XCSOAR_JSON_GEO_WRITER_HPP
#define XCSOAR_JSON_GEO_WRITER_HPP

#include "Writer.hpp"
#include "Geo/GeoPoint.hpp"

namespace JSON {
  /**
   * Writer for a JSON floating point value.
   */
  static inline void WriteDouble(BufferedOutputStream &writer, double value) {
    writer.Format("%f", (double)value);
  }

  static inline void WriteAngle(BufferedOutputStream &writer, Angle value) {
    WriteDouble(writer, value.Degrees());
  }

  static inline void WriteGeoPointAttributes(JSON::ObjectWriter &object,
                                             const ::GeoPoint &value) {
    object.WriteElement("longitude", WriteAngle, value.longitude);
    object.WriteElement("latitude", WriteAngle, value.latitude);
  }

  static inline void WriteGeoPoint(BufferedOutputStream &writer,
                                   const ::GeoPoint &value) {
    ObjectWriter object(writer);
    WriteGeoPointAttributes(object, value);
  }
};

#endif

