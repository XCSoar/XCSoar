/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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

#include "Terrain/RasterWeather.hpp"
#include "Terrain/RasterMap.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"
#include "LocalPath.hpp"
#include "OS/FileUtil.hpp"
#include "Time/BrokenTime.hpp"
#include "Util/ConvertString.hpp"
#include "Util/Clamp.hpp"
#include "Util/Macros.hpp"
#include "Operation/Operation.hpp"
#include "zzip/zzip.h"

#include <assert.h>
#include <tchar.h>
#include <stdio.h>
#include <windef.h> // for MAX_PATH

struct WeatherDescriptor {
  const TCHAR *name;
  const TCHAR *label;
  const TCHAR *help;
};

static constexpr WeatherDescriptor WeatherDescriptors[RasterWeather::MAX_WEATHER_MAP] = {
  {
    nullptr,
    N_("Terrain"),
    N_("Display terrain on map, no weather data displayed."),
  },
  {
    _T("wstar"),
    N_("W*"),
    N_("Average dry thermal updraft strength near mid-BL height.  Subtract glider descent rate to get average vario reading for cloudless thermals.  Updraft strengths will be stronger than this forecast if convective clouds are present, since cloud condensation adds buoyancy aloft (i.e. this neglects \"cloudsuck\").  W* depends upon both the surface heating and the BL depth."),
  },
  {
    _T("blwindspd"),
    N_("BL Wind spd"),
    N_("The speed and direction of the vector-averaged wind in the BL.  This prediction can be misleading if there is a large change in wind direction through the BL."),
  },
  {
    _T("hbl"),
    N_("H bl"),
    N_("Height of the top of the mixing layer, which for thermal convection is the average top of a dry thermal.  Over flat terrain, maximum thermalling heights will be lower due to the glider descent rate and other factors.  In the presence of clouds (which release additional buoyancy aloft, creating \"cloudsuck\") the updraft top will be above this forecast, but the maximum thermalling height will then be limited by the cloud base.  Further, when the mixing results from shear turbulence rather than thermal mixing this parameter is not useful for glider flying. "),
  },
  {
    _T("dwcrit"),
    N_("dwcrit"),
    N_("This parameter estimates the height above ground at which the average dry updraft strength drops below 225 fpm and is expected to give better quantitative numbers for the maximum cloudless thermalling height than the BL Top height, especially when mixing results from vertical wind shear rather than thermals.  (Note: the present assumptions tend to underpredict the max. thermalling height for dry consitions.) In the presence of clouds the maximum thermalling height may instead be limited by the cloud base.  Being for \"dry\" thermals, this parameter omits the effect of \"cloudsuck\"."),
  },
  {
    _T("blcloudpct"),
    N_("bl cloud"),
    N_("This parameter provides an additional means of evaluating the formation of clouds within the BL and might be used either in conjunction with or instead of the other cloud prediction parameters.  It assumes a very simple relationship between cloud cover percentage and the maximum relative humidity within the BL.  The cloud base height is not predicted, but is expected to be below the BL Top height."),
  },
  {
    _T("sfctemp"),
    N_("Sfc temp"),
    N_("The temperature at a height of 2m above ground level.  This can be compared to observed surface temperatures as an indication of model simulation accuracy; e.g. if observed surface temperatures are significantly below those forecast, then soaring conditions will be poorer than forecast."),
  },
  {
    _T("hwcrit"),
    N_("hwcrit"),
    N_("This parameter estimates the height at which the average dry updraft strength drops below 225 fpm and is expected to give better quantitative numbers for the maximum cloudless thermalling height than the BL Top height, especially when mixing results from vertical wind shear rather than thermals.  (Note: the present assumptions tend to underpredict the max. thermalling height for dry consitions.) In the presence of clouds the maximum thermalling height may instead be limited by the cloud base.  Being for \"dry\" thermals, this parameter omits the effect of \"cloudsuck\"."),
  },
  {
    _T("wblmaxmin"),
    N_("wblmaxmin"),
    N_("Maximum grid-area-averaged extensive upward or downward motion within the BL as created by horizontal wind convergence. Positive convergence is associated with local small-scale convergence lines.  Negative convergence (divergence) produces subsiding vertical motion, creating low-level inversions which limit thermalling heights."),
  },
  {
    _T("blcwbase"),
    N_("blcwbase"),
    nullptr,
  },
};

RasterWeather::RasterWeather()
  :center(GeoPoint::Invalid()),
   parameter(0),
   weather_time(0), last_weather_time(0),
   reload(true),
   weather_map(nullptr)
{
  std::fill_n(weather_available, ARRAY_SIZE(weather_available), false);
}

BrokenTime
RasterWeather::IndexToTime(unsigned index)
{
  return BrokenTime(index / 2, index % 2 == 0 ? 0 : 30);
}

void
RasterWeather::SetParameter(unsigned i)
{
  Poco::ScopedRWLock protect(lock, true);
  parameter = i;
  reload = true;
}

void
RasterWeather::SetTime(unsigned i)
{
  assert(i < MAX_WEATHER_TIMES);

  Poco::ScopedRWLock protect(lock, true);
  weather_time = i;
}

const RasterMap *
RasterWeather::GetMap() const
{
  // JMW this is not safe in TerrainRenderer's use
  Poco::ScopedRWLock protect(lock, false);
  return weather_map;
}

unsigned
RasterWeather::GetParameter() const
{
  Poco::ScopedRWLock protect(lock, false);
  return parameter;
}

unsigned
RasterWeather::GetTime() const
{
  Poco::ScopedRWLock protect(lock, false);
  return weather_time;
}

bool
RasterWeather::isWeatherAvailable(unsigned t) const
{
  Poco::ScopedRWLock protect(lock, false);
  assert(t < MAX_WEATHER_TIMES);
  return weather_available[std::min((unsigned)MAX_WEATHER_TIMES, t)];
}

void
RasterWeather::NarrowWeatherFilename(char *filename, const TCHAR *name,
                                     unsigned time_index)
{
  const WideToACPConverter narrow_name(name);
  const BrokenTime t = IndexToTime(time_index);
  sprintf(filename, "%s.curr.%02u%02ulst.d2.jp2",
          (const char *)narrow_name, t.hour, t.minute);
}

void
RasterWeather::GetFilename(TCHAR *rasp_filename, const TCHAR *name,
                           unsigned time_index)
{
  TCHAR fname[MAX_PATH];
  const BrokenTime t = IndexToTime(time_index);
  _stprintf(fname, _T("xcsoar-rasp.dat/%s.curr.%02u%02ulst.d2.jp2"),
            name, t.hour, t.minute);
  LocalPath(rasp_filename, fname);
}

RasterMap *
RasterWeather::LoadItem(const TCHAR *name, unsigned time_index,
                        OperationEnvironment &operation)
{
  TCHAR rasp_filename[MAX_PATH];
  GetFilename(rasp_filename, name, time_index);
  RasterMap *map = new RasterMap(rasp_filename, nullptr, nullptr, operation);
  if (!map->IsDefined()) {
    delete map;
    return nullptr;
  }

  return map;
}

bool
RasterWeather::ExistsItem(struct zzip_dir *dir, const TCHAR *name,
                          unsigned time_index) const
{
  char filename[MAX_PATH];
  NarrowWeatherFilename(filename, name, time_index);

  ZZIP_STAT st;
  return zzip_dir_stat(dir, filename, &st, 0) == 0;
}

void
RasterWeather::ScanAll(const GeoPoint &location,
                       OperationEnvironment &operation)
{
  /* not holding the lock here, because this method is only called
     during startup, when the other threads aren't running yet */

  operation.SetText(_("Scanning weather forecast"));

  TCHAR fname[MAX_PATH];
  LocalPath(fname, _T("xcsoar-rasp.dat"));

  const WideToACPConverter narrow_path(fname);
  ZZIP_DIR *dir = zzip_dir_open(narrow_path, nullptr);
  if (dir == nullptr)
    return;

  operation.SetProgressRange(MAX_WEATHER_TIMES);
  for (unsigned i = 0; i < MAX_WEATHER_TIMES; i++) {
    operation.SetProgressPosition(i);
    weather_available[i] = ExistsItem(dir, _T("wstar"), i) ||
      ExistsItem(dir, _T("wstar_bsratio"), i);
  }

  zzip_dir_close(dir);
}

void
RasterWeather::Reload(unsigned day_time_local, OperationEnvironment &operation)
{
  if (parameter == 0)
    // will be drawing terrain
    return;

  Poco::ScopedRWLock protect(lock, true);
  unsigned effective_weather_time = weather_time;
  if (effective_weather_time == 0) {
    // "Now" time, so find time in half hours
    unsigned half_hours = (day_time_local / 1800) % 48;
    effective_weather_time = half_hours;
  }

  if (effective_weather_time != last_weather_time)
    reload = true;

  if (!reload) {
    // no change, quick exit.
    return;
  }

  reload = false;

  last_weather_time = effective_weather_time;

  // scan forward to next valid time
  while (!weather_available[effective_weather_time]) {
    ++effective_weather_time;

    if (effective_weather_time >= MAX_WEATHER_TIMES) {
      // can't find valid time, so reset to zero
      weather_time = 0;
      return;
    }
  }

  CloseLocked();

  weather_map = LoadItem(WeatherDescriptors[parameter].name,
                         effective_weather_time, operation);
  if (weather_map == nullptr && parameter == 1)
    weather_map = LoadItem(_T("wstar_bsratio"),
                           effective_weather_time, operation);
}

void
RasterWeather::Close()
{
  Poco::ScopedRWLock protect(lock, true);
  CloseLocked();
}

void
RasterWeather::CloseLocked()
{
  delete weather_map;
  weather_map = nullptr;
  center = GeoPoint::Invalid();
}

void
RasterWeather::SetViewCenter(const GeoPoint &location, fixed radius)
{
  if (parameter == 0)
    // will be drawing terrain
    return;

  Poco::ScopedRWLock protect(lock, true);
  if (weather_map == nullptr)
    return;

  /* only update the RasterMap if the center was moved far enough */
  if (center.IsValid() && center.Distance(location) < fixed(1000))
    return;

  weather_map->SetViewCenter(location, radius);
  if (!weather_map->IsDirty())
    center = location;
}

bool
RasterWeather::IsDirty() const
{
  if (parameter == 0)
    // will be drawing terrain
    return false;

  Poco::ScopedRWLock protect(lock, false);
  return weather_map != nullptr && weather_map->IsDirty();
}

const TCHAR *
RasterWeather::ItemLabel(unsigned i)
{
  assert(i < MAX_WEATHER_MAP);

  const TCHAR *label = WeatherDescriptors[i].label;
  if (gcc_unlikely(label == nullptr))
    return nullptr;

  return gettext(label);
}

const TCHAR *
RasterWeather::ItemHelp(unsigned i)
{
  assert(i < MAX_WEATHER_MAP);

  const TCHAR *help = WeatherDescriptors[i].help;
  if (gcc_unlikely(help == nullptr))
    return nullptr;

  return gettext(help);
}
