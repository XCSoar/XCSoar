/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "RasterWeather.h"
#include "LogFile.hpp"
#include "XCSoar.h"
#include "Dialogs.h"
#include "Language.hpp"
#include "Units.h"
#include "Blackboard.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Registry.hpp"
#include "LocalPath.hpp"
#include "LocalTime.hpp"

#include "wcecompat/ts_string.h"

#include "RasterTerrain.h" // JMW TODO decouple


int RasterWeather::IndexToTime(int x) {
  if (x % 2 == 0) {
    return (x/2)*100;
  } else {
    return (x/2)*100+30;
  }
}


void RasterWeather::RASP_filename(char* rasp_filename,
                                  const TCHAR* name) {
  TCHAR fname[MAX_PATH];
  _stprintf(fname,
            TEXT("xcsoar-rasp.dat/%s.curr.%04dlst.d2.jp2"),
            name, IndexToTime(weather_time));
  LocalPathS(rasp_filename, fname);
}

bool RasterWeather::LoadItem(int item, const TCHAR* name) {
  char rasp_filename[MAX_PATH];
  RASP_filename(rasp_filename, name);
  weather_map[item] = new RasterMapJPG2000();
  weather_map[item]->Open(rasp_filename);
  if (!weather_map[item]->isMapLoaded()) {
    weather_map[item]->Close();
    delete weather_map[item];
    weather_map[item]= 0;
    return false;
  }
  return true;
};


void RasterWeather::Scan(double lat, double lon) {
  int i;
  for (i=0; i<MAX_WEATHER_TIMES; i++) {
    weather_time = i;
    weather_available[i] = LoadItem(0,TEXT("wstar"));
    if (!weather_available[i]) {
      weather_available[i] = LoadItem(0,TEXT("wstar_bsratio"));
      if (weather_available[i]) {
        bsratio = true;
      }
    }
    Close();
  }
  weather_time = 0;
}


void RasterWeather::Reload(double lat, double lon) {
  static int last_weather_time = -1;
  bool found = false;
  bool now = false;

  if (RasterTerrain::render_weather == 0) {
    // will be drawing terrain
    return;
  }

  if (weather_time== 0) {
    // "Now" time, so find time in half hours
    int dsecs = (int)TimeLocal((long)GPS_INFO.Time);
    int half_hours = (dsecs/1800) % 48;
    weather_time = max(weather_time, half_hours);
    now = true;
  }

  // limit values, for safety
  weather_time = min(MAX_WEATHER_TIMES-1, max(0, weather_time));

  if (weather_time == last_weather_time) {
    // no change, quick exit.
    if (now) {
      // must return to 0 = Now time on exit
      weather_time = 0;
    }
    return;
  } else {
    last_weather_time = weather_time;
  }

  // scan forward to next valid time
  while ((weather_time<MAX_WEATHER_TIMES) && (!found)) {
    if (!weather_available[weather_time]) {
      weather_time++;
    } else {
      found = true;

      Close();
      if (bsratio) {
        LoadItem(0,TEXT("wstar_bsratio"));
      } else {
        LoadItem(0,TEXT("wstar"));
      }
      LoadItem(1,TEXT("blwindspd"));
      LoadItem(2,TEXT("hbl"));
      LoadItem(3,TEXT("dwcrit"));
      LoadItem(4,TEXT("blcloudpct"));
      LoadItem(5,TEXT("sfctemp"));
      LoadItem(6,TEXT("hwcrit"));
      LoadItem(7,TEXT("wblmaxmin"));
      LoadItem(8,TEXT("blcwbase"));
    }
  }

  // can't find valid time, so reset to zero
  if (!found || now) {
    weather_time = 0;
  }

  SetViewCenter(lat, lon);
  ServiceFullReload(lat, lon);
}


void RasterWeather::Close() {
  // todo: locking!
  int i;
  for (i=0; i<MAX_WEATHER_MAP; i++) {
    if (weather_map[i]) {
      weather_map[i]->Close();
      delete weather_map[i];
      weather_map[i]=0;
    }
  }
}


void RasterWeather::SetViewCenter(double lat, double lon) {
  for (int i=0; i<MAX_WEATHER_MAP; i++) {
    if (weather_map[i]) {
      weather_map[i]->SetViewCenter(lat, lon);
    }
  }
}


void RasterWeather::ServiceFullReload(double lat, double lon) {
  for (int i=0; i<MAX_WEATHER_MAP; i++) {
    if (weather_map[i]) {
      weather_map[i]->ServiceFullReload(lat, lon);
    }
  }
}


void RasterWeather::ItemLabel(int i, TCHAR* Buffer) {
  _stprintf(Buffer, TEXT("\0"));

  switch (i) {
  case 0:
    return;
  case 1: // wstar
    _stprintf(Buffer, gettext(TEXT("W*")));
    return;
  case 2: // blwindspd
    _stprintf(Buffer, gettext(TEXT("BL Wind spd")));
    return;
  case 3: // hbl
    _stprintf(Buffer, gettext(TEXT("H bl")));
    return;
  case 4: // dwcrit
    _stprintf(Buffer, gettext(TEXT("dwcrit")));
    return;
  case 5: // blcloudpct
    _stprintf(Buffer, gettext(TEXT("bl cloud")));
    return;
  case 6: // sfctemp
    _stprintf(Buffer, gettext(TEXT("Sfc temp")));
    return;
  case 7: // hwcrit
    _stprintf(Buffer, gettext(TEXT("hwcrit")));
    return;
  case 8: // wblmaxmin
    _stprintf(Buffer, gettext(TEXT("wblmaxmin")));
    return;
  case 9: // blcwbase
    _stprintf(Buffer, gettext(TEXT("blcwbase")));
    return;
  default:
    // error!
    break;
  }
}


void RasterWeather::ValueToText(TCHAR* Buffer, short val) {
  Buffer[0]=0;
  switch (RasterTerrain::render_weather) {
  case 0:
    return;
  case 1: // wstar
    _stprintf(Buffer, TEXT("%.1f%s"), ((val-200)/100.0)*LIFTMODIFY, Units::GetVerticalSpeedName());
    return;
  case 2: // blwindspd
    _stprintf(Buffer, TEXT("%.0f%s"), (val/100.0)*SPEEDMODIFY, Units::GetHorizontalSpeedName());
    return;
  case 3: // hbl
    _stprintf(Buffer, TEXT("%.0f%s"), val*ALTITUDEMODIFY, Units::GetAltitudeName());
    return;
  case 4: // dwcrit
    _stprintf(Buffer, TEXT("%.0f%s"), val*ALTITUDEMODIFY, Units::GetAltitudeName());
    return;
  case 5: // blcloudpct
    _stprintf(Buffer, TEXT("%d%%"), max(0,min(100,val)));
    return;
  case 6: // sfctemp
    _stprintf(Buffer, TEXT("%d")TEXT(DEG), iround(val*0.5-20.0));
    return;
  case 7: // hwcrit
    _stprintf(Buffer, TEXT("%.0f%s"), val*ALTITUDEMODIFY, Units::GetAltitudeName());
    return;
  case 8: // wblmaxmin
    _stprintf(Buffer, TEXT("%.1f%s"), ((val-200)/100.0)*LIFTMODIFY, Units::GetVerticalSpeedName());
    return;
  case 9: // blcwbase
    _stprintf(Buffer, TEXT("%.0f%s"), val*ALTITUDEMODIFY, Units::GetAltitudeName());
    return;
  default:
    // error!
    break;
  }
}


