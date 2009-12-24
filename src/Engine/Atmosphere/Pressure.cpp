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

#include "Pressure.hpp"

static const fixed fixed_101325(1013.25);
static const fixed isa_sea_level_density(1.225);
static const fixed hpa_to_pa(100);
static const fixed pa_to_hpa(0.01);
static const fixed k1(0.190263);
static const fixed inv_k1(1.0 / 0.190263);
static const fixed k2(8.417286e-5);
static const fixed inv_k2(1.0 / 8.417286e-5);
static const fixed k4(44330.8);
static const fixed k5(1.0 / 4946.54);
static const fixed k6(1.0 / 42266.5);
static const fixed k7(1.0 / 0.234969);


AtmosphericPressure::AtmosphericPressure():
  m_QNH(fixed_101325)
{
}


fixed
AtmosphericPressure::QNHAltitudeToStaticPressure(const fixed alt) const
{
  return hpa_to_pa*pow((pow(m_QNH,k1)-k2*alt),inv_k1);
}

fixed
AtmosphericPressure::StaticPressureToQNHAltitude(const fixed ps) const
{
  return (pow(m_QNH,k1) - pow(ps*pa_to_hpa, k1))*inv_k2;
}

fixed
AtmosphericPressure::AltitudeToQNHAltitude(const fixed alt) const
{
  return StaticPressureToQNHAltitude(fixed(pow((k4 - alt) * k5, inv_k1)));
}

void
AtmosphericPressure::FindQNH(const fixed alt_raw, 
                             const fixed alt_known) 
{
  // step 1, find static pressure from device assuming it's QNH adjusted
  const fixed psraw = QNHAltitudeToStaticPressure(alt_raw);

  // step 2, calculate QNH so that reported alt will be known alt
  m_QNH = pow(pow(psraw*pa_to_hpa, k1) + k2*alt_known, inv_k1);
}

fixed 
AtmosphericPressure::AirDensity(const fixed altitude) const
{
  return fixed(pow((k4 - altitude) * k6, k7));
}

fixed
AtmosphericPressure::AirDensityRatio(const fixed altitude) const
{
  return sqrt(isa_sea_level_density/AirDensity(altitude));
}

