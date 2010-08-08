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

#include "Terrain/RasterWeather.hpp"
#include "Terrain/RasterMap.hpp"
#include "Language.hpp"
#include "Units.hpp"
#include "LocalPath.hpp"
#include "LocalTime.hpp"
#include "OS/PathName.hpp"
#include "OS/FileUtil.hpp"
#include "ProgressGlue.hpp"

#include <assert.h>
#include <tchar.h>
#include <stdio.h>
#include <windef.h> // for MAX_PATH

RasterWeather::RasterWeather() :
    _parameter(0),
    _weather_time(0),
    bsratio(false)
{
  for (unsigned i = 0; i < MAX_WEATHER_MAP; i++)
    weather_map[i] = NULL;

  for (unsigned i = 0; i < MAX_WEATHER_TIMES; i++)
    weather_available[i] = false;
}

RasterWeather::~RasterWeather() 
{
  Close();
}

int
RasterWeather::IndexToTime(int index)
{
  if (index % 2 == 0) {
    return (index / 2) * 100;
  } else {
    return (index / 2) * 100 + 30;
  }
}

void
RasterWeather::SetParameter(unsigned i)
{
  Poco::ScopedRWLock protect(lock, true);
  _parameter = i;
}

void
RasterWeather::SetTime(unsigned i)
{
  Poco::ScopedRWLock protect(lock, true);
  _weather_time = i;
}

RasterMap*
RasterWeather::GetMap()
{
  // JMW this is not safe in TerrainRenderer's use
  Poco::ScopedRWLock protect(lock, false);
  if (_parameter) {
    assert(_parameter <= MAX_WEATHER_MAP);
    return weather_map[min((unsigned)MAX_WEATHER_MAP, _parameter) - 1];
  }

  assert(1);
  return NULL;
}

unsigned
RasterWeather::GetParameter()
{
  Poco::ScopedRWLock protect(lock, false);
  return _parameter;
}

unsigned
RasterWeather::GetTime()
{
  Poco::ScopedRWLock protect(lock, false);
  return _weather_time;
}

bool
RasterWeather::isWeatherAvailable(unsigned t)
{
  Poco::ScopedRWLock protect(lock, false);
  assert(t < MAX_WEATHER_TIMES);
  return weather_available[min((unsigned)MAX_WEATHER_TIMES, t - 1)];
}

void
RasterWeather::GetFilename(TCHAR *rasp_filename, const TCHAR* name,
                           unsigned time_index)
{
  TCHAR fname[MAX_PATH];
  _stprintf(fname, _T("xcsoar-rasp.dat/%s.curr.%04dlst.d2.jp2"), name,
            IndexToTime(time_index));
  LocalPath(rasp_filename, fname);
}

bool
RasterWeather::LoadItem(int item, const TCHAR* name, unsigned time_index)
{
  TCHAR rasp_filename[MAX_PATH];
  GetFilename(rasp_filename, name, time_index);
  weather_map[item] = new RasterMap(NarrowPathName(rasp_filename));
  if (!weather_map[item]->isMapLoaded()) {
    delete weather_map[item];
    return false;
  }

  return true;
}

void
RasterWeather::ScanAll(const GEOPOINT &location)
{
  /* not holding the lock here, because this method is only called
     during startup, when the other threads aren't running yet */

  TCHAR fname[MAX_PATH];
  LocalPath(fname, _T("xcsoar-rasp.dat"));
  if (!File::Exists(fname))
    return;

  ProgressGlue::SetRange(MAX_WEATHER_TIMES);
  for (unsigned i = 0; i < MAX_WEATHER_TIMES; i++) {
    ProgressGlue::SetValue(i);
    weather_available[i] = LoadItem(0, _T("wstar"), i);
    if (!weather_available[i]) {
      weather_available[i] = LoadItem(0, _T("wstar_bsratio"), i);
      if (weather_available[i])
        bsratio = true;
    }
    _Close();
  }
}

void
RasterWeather::Reload(const GEOPOINT &location, int day_time)
{
  static unsigned last_weather_time;
  bool found = false;
  bool now = false;

  if (_parameter == 0)
    // will be drawing terrain
    return;

  Poco::ScopedRWLock protect(lock, true);
  if (_weather_time == 0) {
    // "Now" time, so find time in half hours
    unsigned half_hours = (TimeLocal(day_time) / 1800) % 48;
    _weather_time = max(_weather_time, half_hours);
    now = true;
  }

  // limit values, for safety
  _weather_time = min(MAX_WEATHER_TIMES - 1, _weather_time);

  if (_weather_time == last_weather_time) {
    // no change, quick exit.
    if (now)
      // must return to 0 = Now time on exit
      _weather_time = 0;

    return;
  }

  last_weather_time = _weather_time;

  // scan forward to next valid time
  while ((_weather_time < MAX_WEATHER_TIMES) && (!found)) {
    if (!weather_available[_weather_time]) {
      _weather_time++;
    } else {
      found = true;

      Close();
      if (bsratio)
        LoadItem(0, _T("wstar_bsratio"), _weather_time);
      else
        LoadItem(0, _T("wstar"), _weather_time);

      LoadItem(1,_T("blwindspd"), _weather_time);
      LoadItem(2,_T("hbl"), _weather_time);
      LoadItem(3,_T("dwcrit"), _weather_time);
      LoadItem(4,_T("blcloudpct"), _weather_time);
      LoadItem(5,_T("sfctemp"), _weather_time);
      LoadItem(6,_T("hwcrit"), _weather_time);
      LoadItem(7,_T("wblmaxmin"), _weather_time);
      LoadItem(8,_T("blcwbase"), _weather_time);
    }
  }

  // can't find valid time, so reset to zero
  if (!found || now)
    _weather_time = 0;

  SetViewCenter(location);
}

void
RasterWeather::Close()
{
  Poco::ScopedRWLock protect(lock, true);
  _Close();
}

void
RasterWeather::_Close()
{
  for (unsigned i = 0; i < MAX_WEATHER_MAP; i++) {
    delete weather_map[i];
    weather_map[i] = NULL;
  }
}

void
RasterWeather::SetViewCenter(const GEOPOINT &location)
{
  Poco::ScopedRWLock protect(lock, true);

  for (unsigned i = 0; i < MAX_WEATHER_MAP; i++)
    if (weather_map[i])
      weather_map[i]->SetViewCenter(location);
}

const TCHAR*
RasterWeather::ItemLabel(int i)
{
  switch (i) {
  case 1: // wstar
    return _("W*");
  case 2: // blwindspd
    return _("BL Wind spd");
  case 3: // hbl
    return _("H bl");
  case 4: // dwcrit
    return _("dwcrit");
  case 5: // blcloudpct
    return _("bl cloud");
  case 6: // sfctemp
    return _("Sfc temp");
  case 7: // hwcrit
    return _("hwcrit");
  case 8: // wblmaxmin
    return _("wblmaxmin");
  case 9: // blcwbase
    return _("blcwbase");
  }

  return _T("\0");
}

void
RasterWeather::ValueToText(TCHAR* Buffer, short val)
{
  *Buffer = _T('\0');

  switch (_parameter) {
  case 0:
    return;
  case 1: // wstar
    _stprintf(Buffer, _T("%.1f%s"), (double)
              Units::ToUserUnit(fixed(val - 200) / 100, Units::VerticalSpeedUnit),
              Units::GetVerticalSpeedName());
    return;
  case 2: // blwindspd
    _stprintf(Buffer, _T("%.0f%s"), (double)
              Units::ToUserUnit(fixed(val) / 100, Units::SpeedUnit),
              Units::GetSpeedName());
    return;
  case 3: // hbl
    _stprintf(Buffer, _T("%.0f%s"), (double)
              Units::ToUserUnit(fixed(val), Units::AltitudeUnit),
              Units::GetAltitudeName());
    return;
  case 4: // dwcrit
    _stprintf(Buffer, _T("%.0f%s"), (double)
              Units::ToUserUnit(fixed(val), Units::AltitudeUnit),
              Units::GetAltitudeName());
    return;
  case 5: // blcloudpct
    _stprintf(Buffer, _T("%d%%"), (int)max(0, min(100, (int)val)));
    return;
  case 6: // sfctemp
    _stprintf(Buffer, _T("%d")_T(DEG), val / 2 - 20);
    return;
  case 7: // hwcrit
    _stprintf(Buffer, _T("%.0f%s"), (double)
              Units::ToUserUnit(fixed(val), Units::AltitudeUnit),
              Units::GetAltitudeName());
    return;
  case 8: // wblmaxmin
    _stprintf(Buffer, _T("%.1f%s"), (double)
              Units::ToUserUnit(fixed(val - 200) / 100, Units::VerticalSpeedUnit),
              Units::GetVerticalSpeedName());
    return;
  case 9: // blcwbase
    _stprintf(Buffer, _T("%.0f%s"), (double)
              Units::ToUserUnit(fixed(val), Units::AltitudeUnit),
              Units::GetAltitudeName());
    return;
  default:
    // error!
    break;
  }
}
