/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Replay/ReplayLogger.hpp"
#include <algorithm>

#include "Navigation/GeoPoint.hpp"
#include "StringUtil.hpp"

ReplayLogger::ReplayLogger() :
  TimeScale(1.0),
  Enabled(false),
  initialised(false),
  finished(false),
  fp(NULL)
{
  FileName[0] = _T('\0');
}

bool
ReplayLogger::ReadLine(TCHAR *buffer)
{
  if (!buffer || !OpenFile())
    return false;

  return _fgetts(buffer, 200, fp);
}

bool
ReplayLogger::ScanBuffer(const TCHAR *buffer, fixed *Time,
                         fixed *Latitude, fixed *Longitude, fixed *Altitude)
{
  int DegLat, DegLon;
  int MinLat, MinLon;
  TCHAR NoS, EoW;
  int iAltitude;
  int Hour = 0;
  int Minute = 0;
  int Second = 0;
  int lfound =
      _stscanf(buffer, _T("B%02d%02d%02d%02d%05d%c%03d%05d%cA%05d%*05d"),
      &Hour, &Minute, &Second, &DegLat, &MinLat, &NoS, &DegLon,
      &MinLon, &EoW, &iAltitude);

  if (lfound == EOF)
    return false;

  if (lfound != 10)
    return false;

  *Latitude = DegLat + MinLat / 60000.0;
  if (NoS == _T('S'))
    *Latitude *= -1;

  *Longitude = DegLon + MinLon / 60000.0;
  if (EoW == _T('W'))
    *Longitude *= -1;

  *Altitude = iAltitude;
  *Time = Hour * 3600 + Minute * 60 + Second;
  return true;
}

bool
ReplayLogger::ReadPoint(fixed *Time, fixed *Latitude, fixed *Longitude,
                        fixed *Altitude)
{
  TCHAR buffer[200];

  while (ReadLine(buffer)) {
    if (ScanBuffer(buffer, Time, Latitude, Longitude, Altitude))
      return true;
  }

  return false;
}

void
ReplayLogger::get_time(const bool reset, const fixed mintime)
{
  if (reset)
    t_simulation = fixed_zero;
  else
    t_simulation += fixed_one;

  t_simulation = std::max(mintime, t_simulation);
}

void
ReplayLogger::on_reset()
{
  // nothing
}

void
ReplayLogger::on_stop()
{
  // nothing
}

void
ReplayLogger::on_bad_file()
{
  // nothing
}

bool
ReplayLogger::UpdateInternal()
{
  if (!Enabled) {
    initialised = false;
    CloseFile();
    finished = false;
    cli.Reset();
    t_simulation = 0;
    Enabled = true;
    on_reset();
  }

  const int t_simulation_last = t_simulation;
  get_time(!initialised, cli.GetMinTime());
  initialised = true;
  if ((int)t_simulation <= t_simulation_last)
    return true;

  // if need a new point
  while (cli.NeedData(t_simulation) && (!finished)) {
    fixed t1 = fixed_zero;
    fixed Lat1, Lon1, Alt1;
    finished = !ReadPoint(&t1, &Lat1, &Lon1, &Alt1);

    if (!finished && positive(t1))
      cli.Update(t1, Lon1, Lat1, Alt1);
  }

  if (t_simulation == fixed_zero)
    t_simulation = cli.GetMaxTime();

  if (finished) {
    Stop();
  } else {
    fixed Alt0;
    fixed Alt1;
    GEOPOINT P0, P1;

    cli.Interpolate(t_simulation, P0, Alt0);
    cli.Interpolate(t_simulation + fixed(0.1), P1, Alt1);

    const fixed Speed = cli.GetSpeed(t_simulation);
    const Angle Bearing = P0.bearing(P1);

    on_advance(P0, Speed, Bearing, Alt0, Alt0, t_simulation);
  }

  return !finished;
}

void
ReplayLogger::Stop()
{
  CloseFile();

  if (Enabled)
    on_stop();

  Enabled = false;
}

void
ReplayLogger::Start()
{
  if (Enabled)
    Stop();

  if (!UpdateInternal())
    on_bad_file();
}

const TCHAR*
ReplayLogger::GetFilename()
{
  return FileName;
}

void
ReplayLogger::SetFilename(const TCHAR *name)
{
  if (!name || string_is_empty(name))
    return;

  if (_tcscmp(FileName, name) != 0)
    _tcscpy(FileName, name);
}

bool
ReplayLogger::Update()
{
  if (!Enabled)
    return false;

  Enabled = UpdateInternal();
  return Enabled;
}

bool
ReplayLogger::OpenFile()
{
  if (fp)
    return true;

  if (string_is_empty(FileName))
    return false;

  fp = _tfopen(FileName, _T("rt"));
  if (fp)
    return true;

  return false;
}

void
ReplayLogger::CloseFile()
{
  if (!fp)
    return;

  fclose(fp);
  fp = NULL;
}
