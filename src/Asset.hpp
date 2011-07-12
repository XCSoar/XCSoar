/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#define HAVE_MODEL_TYPE

extern ModelType GlobalModelType;

#else

#define GlobalModelType MODELTYPE_PNA_PNA

#endif

static inline bool
have_model_type()
{
#ifdef HAVE_MODEL_TYPE
  return true;
#else
  return false;
#endif
}

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
#if defined(_WIN32_WCE) || defined(ANDROID)
  return true;
#else
  return false;
#endif
}

/**
 * Returns whether the application is running on Pocket PC / Windows
 * CE / Windows Mobile.
 */
static inline bool
is_windows_ce()
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
 * Is XCSoar running on ancient and slow hardware?  If yes, then some
 * expensive UI features are disabled.
 */
static inline bool
is_ancient_hardware()
{
#if defined(_WIN32_WCE) && _WIN32_WCE < 0x0400
  /* Windows CE 3.0 (PPC2000 & PPC2002) */
  return true;
#else
  /* we assume that all other platforms are fast enough */
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
 * Returns whether the application is running on Android
 */
static inline bool
is_android()
{
#if defined(ANDROID)
  return true;
#else
  return false;
#endif
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
