/* Copyright_License {

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
#include "AirspaceAltitude.hpp"
#include "AbstractAirspace.hpp"
#include "Units.hpp"

const tstring 
AIRSPACE_ALT::get_as_text_units(const bool concise) const
{
  tstringstream oss;
  switch (Base) {
  case abAGL:
    if (!positive(AGL)) {
      oss << _T("GND");
    } else {
      oss << (int)Units::ToUserAltitude(AGL) << _T(" ") << Units::GetAltitudeName() << _T(" AGL");
    }
    break;
  case abFL:
    oss << _T("FL") << (int)FL;
    break;
  case abMSL:
    oss << (int)Units::ToUserAltitude(Altitude) << _T(" ") << Units::GetAltitudeName();
    break;
  case abUndef:
  default:
    break;
  };
  if (!concise && Base!=abMSL && positive(Altitude)) {
    oss << _T(" ") << (int)Units::ToUserAltitude(Altitude) << _T(" ") << Units::GetAltitudeName();
  }
  return oss.str();
}


const tstring 
AbstractAirspace::get_base_text(const bool concise) const
{
  return m_base.get_as_text_units(concise);
}

const tstring 
AbstractAirspace::get_top_text(const bool concise) const
{
  return m_top.get_as_text_units(concise);
}
