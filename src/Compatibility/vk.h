/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifndef XCSOAR_COMPATIBILITY_VK_H
#define XCSOAR_COMPATIBILITY_VK_H

#ifndef WIN32

#elif !defined(_WIN32_WCE)

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
