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
extern TCHAR strRegKey[];

void ReadAssetNumber(void);

// model info

#if defined(PNA) || defined(FIVV)  // VENTA2- ADD GlobalEllipse
extern int	GlobalModelType;
extern TCHAR	GlobalModelName[];
extern float	GlobalEllipse;
extern const TCHAR *gmfpathname();
extern const TCHAR *gmfbasename();
extern int		GetGlobalModelName();
extern void		SmartGlobalModelType();
extern short		InstallFonts();
extern bool		CheckDataDir();
extern bool		CheckRegistryProfile();
extern void		ConvToUpper( TCHAR *);

static inline void
SetGlobalEllipse(float value)
{
  GlobalEllipse = value;
}

#else

#define GlobalEllipse 1.1f

static inline void
SetGlobalEllipse(float value)
{
  (void)value; // ignored on this platform
}

#endif

/*
    Here we declare Model Types for embedded custom versions. Initially for PNAs only.
	We don't need a "type" and a "model" such as "pna" and "hp310". Instead we use a
	single int value with subsets made of ranges.
	We use modeltypes currently for extraclipping, hardware key transcoding, and we should
	also handle embedded gps com ports and adjustments (TODO)

    types     0 -    99 are reserved and 0 is generic/unknown
    types   100 -   999 are special devices running embedded XCSoar
    types  1000 -  9999 are PDAs
    types 10000 - 99999 are PNAs, each brand with 200 units slots for inner types
                                 (initially we try to stay below 32767 within a short var)
    types over 100000	are reserved and should not be used
 */

#if defined(PNA) || defined(FIVV) // VENTA
#define MODELTYPE_UNKNOWN		0
#define MODELTYPE_GENERIC		0

#define MODELTYPE_EMBEDDED		 100	// generic embedded
#define MODELTYPE_ALTAIR		 101

#define MODELTYPE_PDA_PDA		1000	// generic PDA
#define MODELTYPE_PDA			1000

#define MODELTYPE_PNA_PNA		10000	// generic PNA
#define MODELTYPE_PNA			10000
#define MODELTYPE_PNA_HP		10200	// Generic HP
#define MODELTYPE_PNA_HP31X		10201	// HP310, 312, 314, 316

#define MODELTYPE_PNA_DAYTON	10400	// Generic VDO Dayton
#define MODELTYPE_PNA_PN6000	10401

#define MODELTYPE_PNA_MIO		10600	// Generic definitions
#define MODELTYPE_PNA_MIO520	10601
#define	MODELTYPE_PNA_MIOP350	10602

#define MODELTYPE_PNA_NAVMAN	10800
#define MODELTYPE_PNA_GARMIN	11000
#define MODELTYPE_PNA_CLARION	11200
#define MODELTYPE_PNA_MEDION	11400
#define MODELTYPE_PNA_MEDION_P5	11401	// clipping problems for P5430 and P5 family
#define MODELTYPE_PNA_SAMSUNG	11600
#define MODELTYPE_PNA_NAVIGO	11800
#define MODELTYPE_PNA_NOKIA	12000
#define MODELTYPE_PNA_NOKIA_500	12001 // 480x272


#endif

/**
 * Returns whether the application is running on an embedded platform.
 * @return True if host hardware is an embedded platform, False otherwise
 */
static inline bool is_embedded()
{
#if defined(WINDOWSPC) || !defined(WIN32)
  return false;
#else
  return true;
#endif
}

/**
 * Returns whether the application is running on a PNA
 * @return True if host hardware is a PNA, False otherwise
 */
static inline bool is_pna()
{
#if defined(PNA)
  return true;
#else
  return false;
#endif
}

/**
 * Returns whether the application is running on a HP31x
 * @return True if host hardware is a HP31x, False otherwise
 */
static inline bool model_is_hp31x()
{
#if defined(PNA) || defined(FIVV)
  return GlobalModelType == MODELTYPE_PNA_HP31X;
#else
  return false;
#endif
}

/**
 * Returns whether the application is running on a Medion P5
 * @return True if host hardware is a Medion P5, False otherwise
 */
static inline bool model_is_medion_p5()
{
#if defined(PNA) || defined(FIVV)
  return GlobalModelType == MODELTYPE_PNA_MEDION_P5;
#else
  return false;
#endif
}

/**
 * Returns whether the application is running on an Altair
 * @return True if host hardware is an Altair, False otherwise
 */
static inline bool is_altair()
{
#if defined(GNAV)
  return true;
#else
  return false;
#endif
}

/**
 * Returns whether the simulator application is running
 * @return True if simulator, False if fly application
 */
static inline bool is_simulator()
{
#ifdef _SIM_
  return true;
#else
  return false;
#endif
}

/**
 * Returns whether the application is compiled in "FIVV" mode
 * (enables old experimental code)
 * @return True if compiled in "FIVV" mode, False otherwise
 */
static inline bool is_fivv()
{
#if defined(FIVV)
  return true;
#else
  return false;
#endif
}

/**
 * Flag to activate extra clipping for some PNAs.
 * @return True if extra clipping needs to be done, False otherwise
 */
static inline bool need_clipping()
{
  return model_is_hp31x() || model_is_medion_p5();
}

/**
 * Does this device have a pointer device? (mouse or touch screen)
 * @return True if a touch screen or mouse is assumed for the hardware
 * that XCSoar is running on, False if the hardware has only buttons
 */
static inline bool has_pointer()
{
  return !is_altair();
}

// This could be also used for PDA in landscape..
typedef enum
{
  ssnone = 0,
  ss240x320,
  ss480x640,
  ss480x800,
  sslandscape, //  <landscape=portrait modes, >landscape=landscape modes
  ss320x240,
  ss480x234,
  ss480x272,
  ss640x480,
  ss800x480
} ScreenSize_t;

#ifdef WINDOWSPC
extern int SCREENWIDTH;
extern int SCREENHEIGHT;
#endif

void InitAsset();

#endif
