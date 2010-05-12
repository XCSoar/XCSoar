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

#include "ReplayLogger.hpp"
#include <algorithm>

#include "Navigation/GeoPoint.hpp"
#include "StringUtil.hpp"

typedef struct _LOGGER_INTERP_POINT
{
  GEOPOINT loc;
  fixed alt;
  fixed t;
} LOGGER_INTERP_POINT;

/*
  ps = (1 u u^2 u^3)[0  1 0 0] p0
  [-t 0 t 0] p1
  [2t t-3 3-2t -t] p2
  [-t 2-t t-2 t] p3
*/

class CatmullRomInterpolator
{
public:
  CatmullRomInterpolator()
  {
    Reset();
  }

  void
  Reset()
  {
    num = 0;
    for (int i = 0; i < 4; i++) {
      p[i].t = 0;
    }
  }

  LOGGER_INTERP_POINT p[4];

  void
  Update(fixed t, fixed lon, fixed lat, fixed alt)
  {
    if (num && (t<=p[num-1].t)) return;

    if (num < 4)
      num++;

    for (int i = 0; i < 3; i++) {
      p[i].loc = p[i + 1].loc;
      p[i].alt = p[i + 1].alt;
      p[i].t = p[i + 1].t;
    }

    p[3].loc.Latitude = Angle::degrees(fixed(lat));
    p[3].loc.Longitude = Angle::degrees(fixed(lon));
    p[3].alt = alt;
    p[3].t = t;
  }

  bool
  Ready()
  {
    return (num == 4);
  }

  fixed
  GetSpeed(fixed time)
  {
    if (!Ready())
      return fixed_zero;

    fixed u = (time - p[1].t) / (p[2].t - p[1].t);

    fixed s0 = p[0].loc.distance(p[1].loc);
    s0 /= (p[1].t - p[0].t);
    fixed s1 = p[1].loc.distance(p[2].loc);
    s1 /= (p[2].t - p[1].t);

    u = max(fixed_zero, min(fixed_one, u));

    return s1 * u + s0 * (fixed_one - u);
  }
  void 
  Interpolate(fixed time, GEOPOINT &loc, fixed &alt)
  {
    if (!Ready()) {
      loc = p[num].loc;
      alt = p[num].alt;
      return;
    }

    const fixed u((time - p[1].t) / (p[2].t - p[1].t));

    if (!positive(u)) {
      loc = p[1].loc;
      alt = p[1].alt;
      return;
    }

    if (u > fixed_one) {
      loc = p[2].loc;
      alt = p[2].alt;
      return;
    }

    const fixed t(0.98);
    const fixed u2 = u * u;
    const fixed u3 = u2 * u;
    const fixed c[4]= {-t * u3 + 2 * t * u2 - t * u,
                       (fixed_two - t) * u3 + (t - fixed(3)) * u2 + fixed_one,
                       (t - fixed_two) * u3 + (fixed(3) - 2 * t) * u2 + t * u,
                        t * u3 - t * u2};

    loc.Latitude = (p[0].loc.Latitude*c[0] + p[1].loc.Latitude*c[1]
                    + p[2].loc.Latitude*c[2] + p[3].loc.Latitude*c[3]);

    loc.Longitude = (p[0].loc.Longitude*c[0] + p[1].loc.Longitude*c[1]
                     + p[2].loc.Longitude*c[2] + p[3].loc.Longitude*c[3]);

    alt = (p[0].alt*c[0] + p[1].alt*c[1] + p[2].alt*c[2] + p[3].alt*c[3]);

  }

  fixed
  GetMinTime()
  {
    return p[0].t;
  }

  fixed
  GetMaxTime()
  {
    return max(fixed_zero, max(p[0].t, max(p[1].t, max(p[2].t, p[3].t))));
  }

  fixed
  GetAverageTime()
  {
    if (num <= 0)
      return fixed_zero;

    fixed tav = fixed_zero;
    for (int i = 0; i < num; i++) {
      tav += p[i].t / num;
    }

    return tav;
  }

  bool
  NeedData(fixed t_simulation)
  {
    return !Ready() || (p[2].t <= t_simulation + fixed(0.1));
  }

private:
  int num;
  fixed tzero;
};



ReplayLogger::ReplayLogger():
  Enabled(false),
  TimeScale(1.0),
  fp(NULL)
{
  FileName[0] = _T('\0');
}


bool
ReplayLogger::ReadLine(TCHAR *buffer)
{
  if (!buffer) {
    if (fp) {
      fclose(fp);
      fp = NULL;
    }
    return false;
  }

  if (fp == NULL && !string_is_empty(FileName))
    fp = _tfopen(FileName, _T("rt"));

  if (fp == NULL) {
    return false;
  }

  if (_fgetts(buffer, 200, fp) == NULL) {
    _tcscat(buffer, TEXT("\0"));
    return false;
  }

  return true;
}


bool
ReplayLogger::ScanBuffer(const TCHAR *buffer, fixed *Time,
                         fixed *Latitude, fixed *Longitude, fixed *Altitude)
{
  int DegLat, DegLon;
  int MinLat, MinLon;
  TCHAR NoS, EoW;
  int iAltitude;
  int bAltitude;
  int Hour=0;
  int Minute=0;
  int Second=0;
  int lfound=0;

  if ((lfound = _stscanf(buffer,
      TEXT("B%02d%02d%02d%02d%05d%c%03d%05d%cA%05d%05d"), &Hour, &Minute,
      &Second, &DegLat, &MinLat, &NoS, &DegLon, &MinLon, &EoW, &iAltitude,
      &bAltitude)) != EOF) {

    if (lfound == 11) {
      *Latitude = DegLat + MinLat / 60000.0;
      if (NoS == _T('S')) {
        *Latitude *= -1;
      }

      *Longitude = DegLon + MinLon / 60000.0;
      if (EoW == _T('W')) {
        *Longitude *= -1;
      }
      *Altitude = iAltitude;
      *Time = Hour * 3600 + Minute * 60 + Second;
      return (lfound > 0);
    }
  }
  return lfound != EOF;
}

bool
ReplayLogger::ReadPoint(fixed *Time, fixed *Latitude, fixed *Longitude,
                        fixed *Altitude)
{
  TCHAR buffer[200];
  bool found = false;

  while (ReadLine(buffer) && !found) {
    if (ScanBuffer(buffer, Time, Latitude, Longitude, Altitude)) {
      found = true;
    }
  }

  return found;
}


fixed
ReplayLogger::get_time(const bool reset, const fixed mintime)
{
  static fixed t_simulation = fixed_zero;
  
  if (reset) {
    t_simulation = fixed_zero;
  } else {
    t_simulation += fixed_one;
  }
  t_simulation = std::max(mintime, t_simulation);
  return t_simulation;
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
  static bool initialised = false;
  static bool finished = false;
  static CatmullRomInterpolator cli;
  static fixed t_simulation;

  if (!Enabled) {
    initialised = false;
    ReadLine(NULL); // close file
    finished = false;
    cli.Reset();
    t_simulation = 0;
    Enabled = true;
    on_reset();
  }

  const int t_simulation_last = t_simulation;
  t_simulation = get_time(!initialised, cli.GetMinTime());
  initialised = true;
  if ((int)t_simulation<= t_simulation_last) {
    return true;
  }

  // if need a new point
  while (cli.NeedData(t_simulation) && (!finished)) {
    fixed t1 = fixed_zero;
    fixed Lat1, Lon1, Alt1;
    finished = !ReadPoint(&t1, &Lat1, &Lon1, &Alt1);

    if (!finished && positive(t1)) {
      cli.Update(t1, Lon1, Lat1, Alt1);
    }
  }

  if (t_simulation == fixed_zero) {
    t_simulation = cli.GetMaxTime();
  }

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
ReplayLogger::Stop(void)
{
  ReadLine(NULL); // close the file

  if (Enabled) {
    on_stop();
  }

  Enabled = false;
}

void
ReplayLogger::Start(void)
{
  if (Enabled)
    Stop();

  if (!UpdateInternal()) {
    on_bad_file();
  }
}

const TCHAR*
ReplayLogger::GetFilename(void)
{
  return FileName;
}

void
ReplayLogger::SetFilename(const TCHAR *name)
{
  if (!name)
    return;

  if (_tcscmp(FileName, name) != 0)
    _tcscpy(FileName, name);
}

bool
ReplayLogger::Update(void)
{
  if (!Enabled)
    return false;

  Enabled = UpdateInternal();
  return Enabled;
}
