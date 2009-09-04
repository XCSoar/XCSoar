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

#ifndef XCSOAR_COMPATIBILITY_VK_H
#define XCSOAR_COMPATIBILITY_VK_H

#ifdef GNAV
/* Triadis Altair */

#define VK_APP1 VK_F1
#define VK_APP2 VK_F2
#define VK_APP3 VK_F3
#define VK_APP4 VK_F4
#define VK_APP5 VK_F5
#define VK_APP6 VK_F6

#elif defined(WINDOWSPC)

#define VK_APP1 0x31
#define VK_APP2 0x32
#define VK_APP3 0x33
#define VK_APP4 0x34
#define VK_APP5 0x35
#define VK_APP6 0x36

#elif defined(WIN32_PLATFORM_PSPC)
// Pocket PC

	#if (_WIN32_WCE == 300)
	// Pocket PC 2000
		// App keys
		#define VK_APP1     0xC1
		#define VK_APP2     0xC2
		#define VK_APP3     0xC3
		#define VK_APP4     0xC4
		#define VK_APP5     0xC5
		#define VK_APP6     0xC6
	    // Note - note used on most builds...
		// #define VK_APP7     0xC7
		// #define VK_APP8     0xC8
	#endif
#endif

#endif
