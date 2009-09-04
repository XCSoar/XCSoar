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
#include "Version.hpp"
#include "Language.hpp"

TCHAR XCSoar_Version[256] = TEXT("");

void Version() {
  // Version String
#ifdef GNAV
  _tcscat(XCSoar_Version, TEXT("Altair "));
#elif defined(PNA)
  _tcscat(XCSoar_Version, TEXT("PNA "));
#else
#ifdef WINDOWSPC
  _tcscat(XCSoar_Version, TEXT("PC "));
#else
  _tcscat(XCSoar_Version, TEXT("PPC "));
  // TODO code: consider adding PPC, 2002, 2003 (not enough room now)
#endif
#endif

  // experimental CVS

#ifdef FIVV
  _tcscat(XCSoar_Version, TEXT("5.2.5F "));
#elif defined(__MINGW32__)
  _tcscat(XCSoar_Version, TEXT("5.2.5 "));
#else
  _tcscat(XCSoar_Version, TEXT("5.2.5 "));
#endif

  _tcscat(XCSoar_Version, TEXT(__DATE__));
}
