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

#include "FanetStation.hpp"

void
FanetStation::Update(const FanetStation &other)
{
  temperature = other.temperature;
  wind_dir_deg = other.wind_dir_deg;
  wind_speed_kmph = other.wind_speed_kmph;
  wind_gust_kmph = other.wind_gust_kmph;
  rel_humidity = other.rel_humidity;
  preasure_hpa = other.preasure_hpa;
  soc_percent = other.soc_percent;
}
