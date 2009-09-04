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

#ifndef OPTIONS_H
#define OPTIONS_H

#include "Debug.h"				// DEBUG OPTIONS FOR EVERYONE
#define   MONOCHROME_SCREEN     1             // optimize for monochrom screen
#define   EXPERIMENTAL          0             // ????

#define   LOGGDEVICEINSTREAM    0             // log device in stream
#define   LOGGDEVCOMMANDLINE    NULL          // device in-stream logger command line
                                              // ie TEXT("-logA=\\Speicherkarte\\logA.log ""-logB=\\SD Card\\logB.log""")
#define   AIRSPACEUSEBINFILE    0             // use and maintain binary airspace file

#define   FONTQUALITY           NONANTIALIASED_QUALITY

#ifdef WINDOWSPC
#ifdef _DEBUG
// leak checking
#define CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif

#define DISABLEAUDIOVARIO

#if defined(GNAV)
#define DISABLEAUDIOVARIO
// use exception handling
#ifndef ALTAIRPROTOTYPE
#ifndef __MINGW32__
#define HAVEEXCEPTIONS
#endif
#endif
// disable internally generated sounds
#define DISABLEAUDIO
#else
#ifndef BIGDISPLAY
#define BIGDISPLAY
#endif
#endif


#ifdef BIGDISPLAY
#define IBLSCALE(x) (   (InfoBoxLayout::IntScaleFlag) ? ((x)*InfoBoxLayout::scale) : ((int)((x)*InfoBoxLayout::dscale)))

#else
#define IBLSCALE(x) (x)
#endif

#ifdef __MINGW32__
#ifndef WINDOWSPC
#define NEWFLARMDB
#endif
#endif

#ifdef WIN32_PLATFORM_PSPC /* Pocket PC */

#if _WIN32_WCE == 300 /* Pocket PC 2000 (unsupported?) */
#define OLDPPC
#define NOCLEARTYPE
#define NOLINETO
#define NOTIME_H
#endif /* _WIN32_WCE == 300 */

#if WIN32_PLATFORM_PSPC == 310 /* Pocket PC 2002 */
#define NOCLEARTYPE
#define NOLINETO
#endif /* WIN32_PLATFORM_PSPC == 310 */

#if WIN32_PLATFORM_PSPC == 400 /* Pocket PC 2003 */
#endif /* WIN32_PLATFORM_PSPC == 400 */

#ifdef PNA
#define NOLINETO
#endif

#endif /* WIN32_PLATFORM_PSPC */

// VENTA2 - FIVV and PNA new inner setups
#ifdef FIVV
#define CREDITS_FIVV
#define LOOK8000
#endif

#ifndef NDEBUG

// display debug messages for virtual keys
/* #define DEBUG_VIRTUALKEYS */

#endif

#endif

/*
 * Put here special settings

#define VENTA_NOREGFONT 	// don't load font settings from registry values
#define LOOK8000		// Make it lookalike an LX8000 whenever possible

 */


/*
 * Put here debug defines, so that other developers can activate them if needed.

#define DRAWLOAD		// show cpu load (set also by DEBUG mode)
#define DEBUG_DBLCLK		// show double click is being pressed
#define VENTA_DEBUG_EVENT	// show key events, actually very few.
#define VENTA_DEBUG_KEY		// activates scan key codes, so you know what an hardware key is mapped to

 */

