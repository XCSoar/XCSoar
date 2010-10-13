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
#ifndef ASSET_H
#define ASSET_H

#include <tchar.h>

// asset/registration data
extern TCHAR strAssetNumber[];

void ReadAssetNumber(void);

// model info

enum ModelType {
  MODELTYPE_PNA_PNA,
  MODELTYPE_PNA_HP31X,
  MODELTYPE_PNA_MEDION_P5,
  MODELTYPE_PNA_MIO,
  MODELTYPE_PNA_NOKIA_500,
  MODELTYPE_PNA_PN6000,
};

#if defined(_WIN32_WCE) && !defined(GNAV)

extern ModelType GlobalModelType;

#else

#define GlobalModelType MODELTYPE_PNA_PNA

#endif

/**
 * Returns whether this is a debug build.
 */
static inline bool
is_debug()
{
#ifdef NDEBUG
  return false;
#else
  return true;
#endif
}

/**
 * Returns whether the application is running on an embedded platform.
 * @return True if host hardware is an embedded platform, False otherwise
 */
static inline bool
is_embedded()
{
#ifdef _WIN32_WCE
  return true;
#else
  return false;
#endif
}

/**
 * Returns whether the application is running on an old version of
 * Windows CE (pre 5.0).  Starting with version 5.0, several bug
 * workarounds are disabled at compile time.
 */
static inline bool
is_old_ce()
{
#if defined(_WIN32_WCE) && _WIN32_WCE < 0x0500
  return true;
#else
  return false;
#endif
}

/**
 * Returns whether the application is running on a HP31x
 * @return True if host hardware is a HP31x, False otherwise
 */
static inline bool
model_is_hp31x()
{
  return GlobalModelType == MODELTYPE_PNA_HP31X;
}

/**
 * Returns whether the application is running on a Medion P5
 * @return True if host hardware is a Medion P5, False otherwise
 */
static inline bool
model_is_medion_p5()
{
  return GlobalModelType == MODELTYPE_PNA_MEDION_P5;
}

/**
 * Returns whether the application is running on an Altair
 * @return True if host hardware is an Altair, False otherwise
 */
static inline bool
is_altair()
{
#if defined(GNAV)
  return true;
#else
  return false;
#endif
}

/**
 * Flag to activate extra clipping for some PNAs.
 * @return True if extra clipping needs to be done, False otherwise
 */
static inline bool
need_clipping()
{
  if (!is_old_ce())
    return false;

  return model_is_hp31x() || model_is_medion_p5();
}

/**
 * Does this device have a pointer device? (mouse or touch screen)
 * @return True if a touch screen or mouse is assumed for the hardware
 * that XCSoar is running on, False if the hardware has only buttons
 */
static inline bool
has_pointer()
{
  return !is_altair();
}

/**
 * Does this device have a keyboard device?
 * @return True if a keyboard is assumed for the hardware
 * that XCSoar is running on, False if the hardware has no keyboard
 */
static inline bool
has_keyboard()
{
  return !is_embedded();
}

/**
 * Does this device have a display with colors?
 *
 * XXX not yet implemented!
 */
static inline bool
has_colors()
{
  return true;
}

#if defined(WIN32) && !defined(_WIN32_WCE)
extern int SCREENWIDTH;
extern int SCREENHEIGHT;
#endif

void InitAsset();

#endif
