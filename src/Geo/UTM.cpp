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

#include "Geo/UTM.hpp"
#include "Geo/GeoPoint.hpp"
#include "WGS84.hpp"
#include "Util/Macros.hpp"

static constexpr double k0 = 0.9996;

static constexpr double e = 0.00669438;
static constexpr double e2 = e * e;
static constexpr double e3 = e * e;
static constexpr double e_p2 = e / (1.0 - e);

gcc_const
static char
CalculateZoneLetter(const Angle latitude)
{
  static constexpr char letters[] = "CDEFGHJKLMNPQRSTUVWXX";
  unsigned index = (unsigned)((latitude.Degrees() + 80) / 8);
  return (index < ARRAY_SIZE(letters)) ? letters[index] : '\0';
}

gcc_const
static unsigned
CalculateZoneNumber(const GeoPoint &p)
{
  if (p.latitude <= Angle::Degrees(64) &&
      p.latitude >= Angle::Degrees(56) &&
      p.longitude <= Angle::Degrees(12) &&
      p.longitude >= Angle::Degrees(3))
    return 32;

  if (p.latitude <= Angle::Degrees(84) &&
      p.latitude >= Angle::Degrees(72) &&
      p.longitude >= Angle::Degrees(0)) {
    if (p.longitude <= Angle::Degrees(9))
      return 31;
    if (p.longitude <= Angle::Degrees(21))
      return 33;
    if (p.longitude <= Angle::Degrees(33))
      return 35;
    if (p.longitude <= Angle::Degrees(42))
      return 37;
  }

  return (int)floor((p.longitude.Degrees() + 180) / 6) + 1;
}

gcc_const
static Angle
GetCentralMeridian(unsigned zone_number)
{
  return Angle::Degrees(int(zone_number - 1) * 6 - 180 + 3);
}

UTM
UTM::FromGeoPoint(GeoPoint p)
{
  double lat = (double)p.latitude.Radians();
  double _sin = (double)p.latitude.sin();
  double _cos = (double)p.latitude.cos();
  double _tan = _sin / _cos;
  double tan2 = _tan * _tan;
  double tan4 = tan2 * tan2;

  UTM utm;
  utm.zone_number = CalculateZoneNumber(p);
  utm.zone_letter = CalculateZoneLetter(p.latitude);

  double n = WGS84::EQUATOR_RADIUS / sqrt(1 - e * _sin * _sin);
  double c = e_p2 * _cos * _cos;

  double a = _cos * (double)(p.longitude.Radians() -
                             GetCentralMeridian(utm.zone_number).Radians());
  double a2 = a * a;
  double a3 = a * a2;
  double a4 = a * a3;
  double a5 = a * a4;
  double a6 = a * a5;

  double m = WGS84::EQUATOR_RADIUS
    * ((1 - e / 4 - 3 * e2 / 64 - 5 * e3 / 256) * lat -
       (3 * e / 8 + 3 * e2 / 32 + 45 * e3 / 1024) * sin(2 * lat) +
       (15 * e2 / 256 + 45 * e3 / 1024) * sin(4 * lat) -
       (35 * e3 / 3072) * sin(6 * lat));

  utm.easting = k0 * n * (a + (1 - tan2 + c) * a3 / 6 +
                          (5 - 18 * tan2 + tan4 + 72 * c - 58 * e_p2) * a5 / 120)
    + 500000;

  utm.northing = k0 * (m + n * _tan *
                       (a2 / 2 +  a4 / 24 * (5 - tan2 + 9 * c + 4 * c * c) +
                        a6 / 720 * (61 - 58 * tan2 + tan4 + 600 * c - 330 * e_p2)));
  if (p.latitude.IsNegative())
    utm.northing += 10000000;

  return utm;
}

GeoPoint
UTM::ToGeoPoint() const
{
  // remove longitude offset
  double x = (double)easting - 500000;
  double y = (double)northing;

  // if southern hemisphere
  if (zone_letter < 'N')
    y -= 10000000.0;

  double m  = y / k0;
  double mu = m / (WGS84::EQUATOR_RADIUS * (1 - e / 4 - 3 * e2 / 64 - 5 * e3 / 256));
  double _e_sqrt = sqrt(1 - e);
  double _e = (1 - _e_sqrt) / (1 + _e_sqrt);
  double _e2 = _e * _e;
  double _e3 = _e * _e2;
  double _e4 = _e * _e3;
  double _e5 = _e * _e4;

  Angle phi1rad = Angle::Radians(mu +
      (3. / 2 * _e - 27. / 32 * _e3 + 269. / 512 * _e5) * sin(2 * mu) +
      (21. / 16 * _e2 - 55. / 32 * _e4) * sin(4 * mu) +
      (151. / 96 * _e3 - 417. / 128 * _e5) * sin(6 * mu) +
      (1097. / 512 * _e4) * sin(8 * mu));

  double _sin = (double)phi1rad.sin();
  double sin2 = _sin * _sin;
  double _cos = (double)phi1rad.cos();
  double _tan = _sin / _cos;
  double tan2 = _tan * _tan;
  double tan4 = tan2 * tan2;

  double _e_sin2_sqrt = sqrt(1 - e * sin2);
  double _e_sin2_sqrt3 = _e_sin2_sqrt * _e_sin2_sqrt * _e_sin2_sqrt;
  double n = WGS84::EQUATOR_RADIUS / _e_sin2_sqrt;
  double c = e * _cos * _cos;
  double c2 = c * c;
  double _r = WGS84::EQUATOR_RADIUS * (1 - e) / _e_sin2_sqrt3;

  double d = x / (n * k0);
  double d2 = d * d;
  double d3 = d * d2;
  double d4 = d * d3;
  double d5 = d * d4;
  double d6 = d * d5;

  double latitude = (double)phi1rad.Radians() -
      (n * _tan / _r) * (d2 / 2 -
       d4 / 24 * (5 + 3 * tan2 + 10 * c - 4 * c2 - 9 * e_p2) +
       d6 / 720 * (61 + 90 * tan2 + 298 * c + 45 * tan4 - 252 * e_p2 - 3 * c2));

  double longitude = (d - d3 / 6 * (1 + 2 * tan2 + c) +
                      d5 / 120 * (5 - 2 * c + 28 * tan2 - 3 * c2 + 8 * e_p2 + 24 * tan4)) / _cos;

  return GeoPoint(Angle::Radians(longitude) + GetCentralMeridian(zone_number),
                  Angle::Radians(latitude));
}
