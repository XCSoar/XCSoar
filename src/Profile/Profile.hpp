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

#ifndef XCSOAR_PROFILE_HPP
#define XCSOAR_PROFILE_HPP

#include "Profile/ProfileKeys.hpp"
#include "Math/fixed.hpp"

#ifdef USE_PROFILE_MAP
#include "Profile/ProfileMap.hpp"
namespace ProfileImpl = ProfileMap;
#elif defined(WIN32)
#include "Profile/Registry.hpp"
namespace ProfileImpl = Registry;
#else
#include "Profile/GConf.hpp"
namespace ProfileImpl = ProfileGConf;
#endif

#include <stddef.h>
#include <tchar.h>
#include <windows.h>

/**
 * Configuration structure for serial devices
 */
struct DeviceConfig {
  enum port_type {
    /**
     * Serial port, i.e. COMx / RS-232.
     */
    SERIAL,

    /**
     * Attempt to auto-discover the GPS source.
     *
     * On Windows CE, this opens the GPS Intermediate Driver Multiplexer.
     * @see http://msdn.microsoft.com/en-us/library/bb202042.aspx
     */
    AUTO,
  };

  port_type port_type;          /**< Type of the port */
  unsigned port_index;          /**< Index of the port */
  unsigned speed_index;         /**< Speed index (baud rate) */
  TCHAR driver_name[32];        /**< Name of the driver */
};

namespace Profile
{
  using namespace ProfileImpl;

  static inline bool
  use_files()
  {
#ifdef PROFILE_NO_FILE
    return false;
#else
    return true;
#endif
  }

  /**
   * Loads the profile files
   */
  void Load();
  /**
   * Loads the given profile file
   */
  void LoadFile(const TCHAR *szFile);

  /**
   * Saves the profile into the profile files
   */
  void Save();
  /**
   * Saves the profile into the given profile file
   */
  void SaveFile(const TCHAR *szFile);

  /**
   * Sets the profile files to load when calling Load()
   * @param override NULL or file to load when calling Load()
   */
  void SetFiles(const TCHAR* override);

  /**
   * Reads a configured path from the profile, and expands it with
   * ExpandLocalPath().
   *
   * @param value a buffer which can store at least MAX_PATH
   * characters
   */
  bool GetPath(const TCHAR *key, TCHAR *value);

  /**
   * Adjusts the application settings according to the profile settings
   */
  void Use();

  /**
   * Saves the sound settings to the profile
   */
  void SetSoundSettings();

  int GetScaleList(fixed *List, size_t Size);

  /**
   * Saves the airspace mode setting to the profile
   * @param i Airspace class index
   */
  void SetAirspaceMode(int i);
  void SetAirspaceColor(int i, int c);
  void SetAirspaceBrush(int i, int c);

  void SetInfoBoxes(int Index,int InfoType);

  void GetDeviceConfig(unsigned n, DeviceConfig &config);
  void SetDeviceConfig(unsigned n, const DeviceConfig &config);

  bool GetFont(const TCHAR *key, LOGFONT* lplf);
  void SetFont(const TCHAR *key, LOGFONT &logfont);
};

#endif
