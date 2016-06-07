#
#  XCSoar Glide Computer - http://www.xcsoar.org/
#  Copyright (C) 2000-2012 The XCSoar Project
#  A detailed list of copyright holders can be found in the file "AUTHORS".
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

#
# This is a gdb module that aids debugging XCSoar.  To load it, launch
# gdb and type:
#
#  source gdb.py
#

import gdb

double_type = gdb.lookup_type('double')
string_type = gdb.lookup_type('char').pointer()

def fixed_value(f):
    """Extracts the floating point value of a 'fixed' instance."""

    if f.type.unqualified().strip_typedefs().code == double_type.code:
        return float(f.cast(double_type))
    else:
        return long(f['m_nVal']) / (1. * (1 << 28))

def angle_value(a):
    return fixed_value(a['value']) * 57.2957795131

class FixedPrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        return str(fixed_value(self.value))

class AnglePrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        return str(angle_value(self.value))

class AngleRangePrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        start = AnglePrinter(self.value['start']).to_string()
        end = AnglePrinter(self.value['end']).to_string()
        return 'AngleRange(%s..%s)' % (start, end)

class GeoPointPrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        if angle_value(self.value['latitude']) >= 180:
            return 'GeoPoint::INVALID'

        longitude = AnglePrinter(self.value['longitude']).to_string()
        latitude = AnglePrinter(self.value['latitude']).to_string()
        return 'GeoPoint(%s %s)' % (longitude, latitude)

class GeoBoundsPrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        if angle_value(self.value['latitude']['end']) >= 180:
            return 'GeoBounds::INVALID'

        west = AnglePrinter(self.value['longitude']['start']).to_string()
        east = AnglePrinter(self.value['longitude']['end']).to_string()
        south = AnglePrinter(self.value['latitude']['start']).to_string()
        north = AnglePrinter(self.value['latitude']['end']).to_string()
        return 'GeoBounds([%s .. %s] [%s .. %s])' % (west, east, south, north)

class GeoVectorPrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        bearing = AnglePrinter(self.value['bearing']).to_string()
        distance = fixed_value(self.value['distance'])
        if distance < 0:
            return 'GeoVector::INVALID'

        return 'GeoVector(%s %s)' % (bearing, distance)

class SpeedVectorPrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        bearing = AnglePrinter(self.value['bearing']).to_string()
        norm = fixed_value(self.value['norm'])
        if norm < 0:
            return 'GeoVector::INVALID'
        if norm == 0:
            return 'GeoVector::ZERO'

        return 'SpeedVector(%s %s)' % (bearing, norm)

class ValidityPrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        return 'Validity(%u)' % self.value['last']

class StaticStringPrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        return self.value['the_data'].cast(string_type)

class BrokenDatePrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        return 'Date(%04u/%02u/%02u)' % \
               (int(self.value['year']),
                int(self.value['month']),
                int(self.value['day']))

class BrokenTimePrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        return 'Time(%02u:%02u:%02u)' % \
               (int(self.value['hour']),
                int(self.value['minute']),
                int(self.value['second']))

class BrokenDateTimePrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        return 'DateTime(%04u/%02u/%02u %02u:%02u:%02u)' % \
               (int(self.value['year']),
                int(self.value['month']),
                int(self.value['day']),
                int(self.value['hour']),
                int(self.value['minute']),
                int(self.value['second']))

class RoughTimePrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        value = int(self.value['value'])
        if value == 0xffff:
            return 'RoughTime::INVALID'

        return 'RoughTime(%02u:%02u)' % (value / 60, value % 60)

class RoughTimeSpanPrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        start = int(self.value['start']['value'])
        end = int(self.value['start']['value'])

        if start == 0xffff and end == 0xffff:
            return 'RoughTimeSpan::INVALID'

        if start == 0xffff:
            start = ''
        else:
            start = '%02u:%02u' % (start / 60, start % 60)

        if end == 0xffff:
            end = ''
        else:
            end = '%02u:%02u' % (end / 60, end % 60)

        return 'RoughTimeSpan(%s..%s)' % (start, end)

def lookup_function(value):
    type = value.type

    if type.code == gdb.TYPE_CODE_REF:
        type = type.target ()

    type = type.unqualified().strip_typedefs()
    typename = type.tag
    if typename == None:
        return None

    if typename == 'fixed':
        return FixedPrinter(value)
    elif typename == 'Angle':
        return AnglePrinter(value)
    elif typename == 'AngleRange':
        return AngleRangePrinter(value)
    elif typename == 'GeoPoint':
        return GeoPointPrinter(value)
    elif typename == 'GeoBounds':
        return GeoBoundsPrinter(value)
    elif typename == 'GeoVector':
        return GeoVectorPrinter(value)
    elif typename == 'SpeedVector':
        return SpeedVectorPrinter(value)
    elif typename == 'Validity':
        return ValidityPrinter(value)
    elif typename == 'BrokenDate':
        return BrokenDatePrinter(value)
    elif typename == 'BrokenTime':
        return BrokenTimePrinter(value)
    elif typename == 'BrokenDateTime':
        return BrokenDateTimePrinter(value)
    elif typename == 'RoughTime':
        return RoughTimePrinter(value)
    elif typename == 'RoughTimeSpan':
        return RoughTimeSpanPrinter(value)
    elif typename[:12] == 'StaticString' or typename[:12] == 'NarrowString':
        return StaticStringPrinter(value)

    return None

gdb.pretty_printers.append(lookup_function)
