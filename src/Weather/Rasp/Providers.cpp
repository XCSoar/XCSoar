/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Providers.hpp"

const RaspProvider rasp_providers[] = {
  { _T("Poland"),
    "https://www.fcst.pl/xcsoar-rasp.dat" },

  { _T("Norddeutschland 2.2km (by Dragos Constantinescu)"),
    "http://46.243.114.203/drjack/RASP/NORD_DE/FCST/xcsoar-rasp.dat" },
  { _T("Romania 2.2km (by Dragos Constantinescu)"),
    "http://46.243.114.203/drjack/RASP/ROMANIA_3DOM/FCST/xcsoar-rasp.dat" },

  { _T("Avenal, California, USA 0.9 km (by Alex Caldwell)"),
    "http://canv.raspmaps.com/RASP/SIERRA/FCST/xcsoar-rasp.dat" },
  { _T("Santa Ynez, California, USA 0.9 km (by Alex Caldwell)"),
    "http://canv.raspmaps.com/RASP/SANTA_YNEZ/FCST/xcsoar-rasp.dat" },

  { _T("Russia Orel (by Linar Yusupov)"),
    "http://soaringweather.no-ip.info/RASP/xcsoar-OREL-rasp.dat" },
  { _T("Russia Moscow (by Linar Yusupov)"),
    "http://soaringweather.no-ip.info/RASP/xcsoar-MOSCOW-rasp.dat" },
  { _T("Russia Kazan (by Linar Yusupov)"),
    "http://soaringweather.no-ip.info/RASP/xcsoar-KAZAN-rasp.dat" },
  { _T("Russia Novo (by Linar Yusupov)"),
    "http://soaringweather.no-ip.info/RASP/xcsoar-NOVO-rasp.dat" },
  { _T("Russia SPb (by Linar Yusupov)"),
    "http://soaringweather.no-ip.info/RASP/xcsoar-SPETER-rasp.dat" },
  { _T("Russia Vladik (by Linar Yusupov)"),
    "http://soaringweather.no-ip.info/RASP/xcsoar-VLADIK-rasp.dat" },

  { _T("Japan Kanto 2.6km (by Haruhiko Okamura)"),
    "http://blipmap.glider.jp/BLIPMAP/xcsoar-rasp.dat" },

  { nullptr, nullptr }
};
