/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "PlaneFileGlue.hpp"
#include "Plane.hpp"
#include "Units/Units.hpp"
#include "IO/KeyValueFileReader.hpp"
#include "IO/KeyValueFileWriter.hpp"
#include "IO/TextWriter.hpp"
#include "IO/FileLineReader.hpp"

#include <cstdio>

static bool
ReadPolar(const TCHAR *string, Plane &plane)
{
  TCHAR *p;

  fixed v1 = Units::ToSysUnit(fixed(_tcstod(string, &p)), unKiloMeterPerHour);
  if (*p != _T(','))
    return false;

  fixed w1 = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  fixed v2 = Units::ToSysUnit(fixed(_tcstod(p + 1, &p)), unKiloMeterPerHour);
  if (*p != _T(','))
    return false;

  fixed w2 = fixed(_tcstod(p + 1, &p));
  if (*p != _T(','))
    return false;

  fixed v3 = Units::ToSysUnit(fixed(_tcstod(p + 1, &p)), unKiloMeterPerHour);
  if (*p != _T(','))
    return false;

  fixed w3 = fixed(_tcstod(p + 1, &p));
  if (*p != '\0')
    return false;

  plane.v1 = v1;
  plane.v2 = v2;
  plane.v3 = v3;
  plane.w1 = w1;
  plane.w2 = w2;
  plane.w3 = w3;
  return true;
}

static bool
ReadFixed(const TCHAR *string, fixed &out)
{
  TCHAR *endptr;
  double tmp = _tcstod(string, &endptr);
  if (endptr == string)
    return false;

  out = fixed(tmp);
  return true;
}

static bool
ReadUnsigned(const TCHAR *string, unsigned &out)
{
  TCHAR *endptr;
  unsigned tmp = _tcstoul(string, &endptr, 0);
  if (endptr == string)
    return false;

  out = tmp;
  return true;
}

bool
PlaneGlue::Read(Plane &plane, KeyValueFileReader &reader)
{
  bool has_registration = false;
  bool has_competition_id = false;
  bool has_type = false;
  bool has_polar_name = false;
  bool has_polar = false;
  bool has_reference_mass = false;
  bool has_dry_mass = false;
  bool has_handicap = false;
  bool has_max_ballast = false;
  bool has_dump_time = false;
  bool has_max_speed = false;
  bool has_wing_area = false;

  KeyValuePair pair;
  while (reader.Read(pair)) {
    if (!has_registration && _tcscmp(pair.key, _T("Registration")) == 0) {
      plane.registration = pair.value;
      has_registration = true;
    } else if (!has_competition_id && _tcscmp(pair.key, _T("CompetitionID")) == 0) {
      plane.competition_id = pair.value;
      has_competition_id = true;
    } else if (!has_type && _tcscmp(pair.key, _T("Type")) == 0) {
      plane.type = pair.value;
      has_type = true;
    } else if (!has_handicap && _tcscmp(pair.key, _T("Handicap")) == 0) {
      has_handicap = ReadUnsigned(pair.value, plane.handicap);
    } else if (!has_polar_name && _tcscmp(pair.key, _T("PolarName")) == 0) {
      plane.polar_name = pair.value;
      has_polar_name = true;
    } else if (!has_polar && _tcscmp(pair.key, _T("PolarInformation")) == 0) {
      has_polar = ReadPolar(pair.value, plane);
    } else if (!has_reference_mass && _tcscmp(pair.key, _T("PolarReferenceMass")) == 0) {
      has_reference_mass = ReadFixed(pair.value, plane.reference_mass);
    } else if (!has_dry_mass && _tcscmp(pair.key, _T("PolarDryMass")) == 0) {
      has_dry_mass = ReadFixed(pair.value, plane.dry_mass);
    } else if (!has_max_ballast && _tcscmp(pair.key, _T("MaxBallast")) == 0) {
      has_max_ballast = ReadFixed(pair.value, plane.max_ballast);
    } else if (!has_dump_time && _tcscmp(pair.key, _T("DumpTime")) == 0) {
      has_dump_time = ReadUnsigned(pair.value, plane.dump_time);
    } else if (!has_max_speed && _tcscmp(pair.key, _T("MaxSpeed")) == 0) {
      has_max_speed = ReadFixed(pair.value, plane.max_speed);
    } else if (!has_wing_area && _tcscmp(pair.key, _T("WingArea")) == 0) {
      has_wing_area = ReadFixed(pair.value, plane.wing_area);
    }
  }

  if (!has_polar || !has_reference_mass)
    return false;

  if (!has_registration)
    plane.registration.clear();
  if (!has_competition_id)
    plane.competition_id.clear();
  if (!has_type)
    plane.type.clear();
  if (!has_polar_name)
    plane.polar_name.clear();
  if (!has_dry_mass)
    plane.dry_mass = plane.reference_mass;
  if (!has_handicap)
    plane.handicap = 100;
  if (!has_max_ballast)
    plane.max_ballast = fixed_zero;
  if (!has_dump_time)
    plane.dump_time = fixed_zero;
  if (!has_max_speed)
    plane.max_speed = fixed(55.555);
  if (!has_wing_area)
    plane.wing_area = fixed_zero;

  return true;
}

bool
PlaneGlue::ReadFile(Plane &plane, const TCHAR *path)
{
  FileLineReader reader(path);
  KeyValueFileReader kvreader(reader);
  return Read(plane, kvreader);
}

void
PlaneGlue::Write(const Plane &plane, KeyValueFileWriter &writer)
{
  TCHAR tmp[255];
  writer.Write(_T("Registration"), plane.registration);
  writer.Write(_T("CompetitionID"), plane.competition_id);
  writer.Write(_T("Type"), plane.type);

  _sntprintf(tmp, 255, _T("%u"), plane.handicap);
  writer.Write(_T("Handicap"), tmp);

  writer.Write(_T("PolarName"), plane.polar_name);

  fixed V1 = Units::ToUserUnit(plane.v1, unKiloMeterPerHour);
  fixed V2 = Units::ToUserUnit(plane.v2, unKiloMeterPerHour);
  fixed V3 = Units::ToUserUnit(plane.v3, unKiloMeterPerHour);
  _sntprintf(tmp, 255, _T("%.3f,%.3f,%.3f,%.3f,%.3f,%.3f"),
             (double)V1, (double)plane.w1,
             (double)V2, (double)plane.w2,
             (double)V3, (double)plane.w3);
  writer.Write(_T("PolarInformation"), tmp);

  _sntprintf(tmp, 255, _T("%f"), (double)plane.reference_mass);
  writer.Write(_T("PolarReferenceMass"), tmp);
  _sntprintf(tmp, 255, _T("%f"), (double)plane.dry_mass);
  writer.Write(_T("PolarDryMass"), tmp);
  _sntprintf(tmp, 255, _T("%f"), (double)plane.max_ballast);
  writer.Write(_T("MaxBallast"), tmp);
  _sntprintf(tmp, 255, _T("%f"), (double)plane.dump_time);
  writer.Write(_T("DumpTime"), tmp);
  _sntprintf(tmp, 255, _T("%f"), (double)plane.max_speed);
  writer.Write(_T("MaxSpeed"), tmp);
  _sntprintf(tmp, 255, _T("%f"), (double)plane.wing_area);
  writer.Write(_T("WingArea"), tmp);
}

void
PlaneGlue::WriteFile(const Plane &plane, const TCHAR *path)
{
  TextWriter writer(path);
  KeyValueFileWriter kvwriter(writer);
  Write(plane, kvwriter);
}
