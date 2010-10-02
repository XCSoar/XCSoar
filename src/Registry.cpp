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

#include "Registry.hpp"
#include "Profile.hpp"
#include "ProfileKeys.hpp"
#include "StringUtil.hpp"
#include "LogFile.hpp"
#include "Defines.h"
#include "Sizes.h"
#include "IO/FileLineReader.hpp"
#include "IO/TextWriter.hpp"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#include "Config/Registry.hpp"
#else
#include "Config/GConf.hpp"
#endif

const static size_t nMaxValueValueSize = MAX_PATH * 2 + 6;
// max regkey name is 256 chars + " = "
const static size_t nMaxKeyNameSize = MAX_PATH + 6;

//
// NOTE: all registry variables are unsigned!
//
bool
Registry::Get(const TCHAR *szRegValue, DWORD &pPos)
{
// returns 0 on SUCCESS, else the non-zero error code
#ifdef WIN32
  RegistryKey registry(HKEY_CURRENT_USER, szProfileKey, true);
  return !registry.error() &&
    registry.get_value(szRegValue, pPos);

#else /* !WIN32 */

  int value;
  if (!GConf().get(szRegValue, value))
    return false;

  pPos = (DWORD)value;
  return true;

#endif /* !WIN32 */
}

bool
Registry::Set(const TCHAR *szRegValue, DWORD Pos)
{
#ifdef WIN32

  RegistryKey registry(HKEY_CURRENT_USER, szProfileKey, false);
  return !registry.error() &&
    registry.set_value(szRegValue, Pos);

#else /* !WIN32 */

  return GConf().set(szRegValue, (int)Pos);

#endif /* !WIN32 */
}

/**
 * Reads a value from the registry file
 * @param szRegValue Name of the value that should be read
 * @param pPos Pointer to the output buffer
 * @param dwSize Maximum size of the output buffer
 */
bool
Registry::Get(const TCHAR *szRegValue, TCHAR *pPos, size_t dwSize)
{
#ifdef WIN32

  RegistryKey registry(HKEY_CURRENT_USER, szProfileKey, true);
  return !registry.error() && registry.get_value(szRegValue, pPos, dwSize);

#else /* !WIN32 */
  return GConf().get(szRegValue, pPos, dwSize);
#endif /* !WIN32 */
}

/**
 * Writes a value to the registry
 * @param szRegValue Name of the value that should be written
 * @param Pos Value that should be written
 */
bool
Registry::Set(const TCHAR *szRegValue, const TCHAR *Pos)
{
#ifdef WIN32

  RegistryKey registry(HKEY_CURRENT_USER, szProfileKey, false);
  return !registry.error() &&
    registry.set_value(szRegValue, Pos);

#else /* !WIN32 */
  return GConf().set(szRegValue, Pos);
#endif /* !WIN32 */
}

void
Registry::Import(const TCHAR *szFile)
{
  if (string_is_empty(szFile))
    return;

  LogStartUp(_T("Loading registry from %s"), szFile);
  FileLineReader reader(szFile);
  if (reader.error())
    return;

  TCHAR *line;
  while ((line = reader.read()) != NULL) {
    if (string_is_empty(line) || *line == _T('#'))
      continue;

    TCHAR *p = _tcschr(line, _T('='));
    if (p == line || p == NULL)
      continue;

    *p = _T('\0');
    TCHAR *value = p + 1;

#ifdef PROFILE_KEY_PREFIX
    TCHAR key[sizeof(PROFILE_KEY_PREFIX) + _tcslen(line)];
    _tcscpy(key, PROFILE_KEY_PREFIX);
    _tcscat(key, line);
#else
    const TCHAR *key = line;
#endif

    if (*value == _T('"')) {
      ++value;
      p = _tcschr(value, _T('"'));
      if (p == NULL)
        continue;

      *p = _T('\0');

      Set(key, value);
    } else {
      long l = _tcstol(value, &p, 10);
      if (p > value)
        Set(key, l);
    }
  }
}

void
Registry::Export(const TCHAR *szFile)
{
#ifdef WIN32
  TCHAR lpstrName[nMaxKeyNameSize+1];

  union {
    BYTE pValue[nMaxValueValueSize+4];
    DWORD dValue;
    TCHAR string_value[1];
  } uValue;

  // If no file is given -> return
  if (string_is_empty(szFile))
    return;

  // Try to open the file for writing
  TextWriter writer(szFile);
  // ... on error -> return
  if (writer.error())
    return;

  // Try to open the XCSoar registry key
  HKEY hkFrom;
  LONG res = ::RegOpenKeyEx(HKEY_CURRENT_USER, szProfileKey,
                            0, KEY_ALL_ACCESS, &hkFrom);
  // ... on error -> return
  if (ERROR_SUCCESS != res)
    return;

  // Iterate through the registry subkeys
  for (int i = 0;; i++) {
    DWORD nType;
    // Reset the buffer sizes
    DWORD nValueSize = nMaxValueValueSize;
    DWORD nNameSize = nMaxKeyNameSize;

    // Reset the key-name buffer
    lpstrName[0] = _T('\0');

    // Get i-th subkey from the registry key defined by hkFrom
    res = ::RegEnumValue(hkFrom, i, lpstrName, &nNameSize, 0, &nType,
                         uValue.pValue, &nValueSize);

    // If we iterated to the end of the subkey "array" -> quit the for-loop
    if (ERROR_NO_MORE_ITEMS == res)
      break;

    // If the size of the name is <= 0 or the buffer is to small
    // -> skip this subkey
    if ((nNameSize <= 0) || (nNameSize > nMaxKeyNameSize))
      continue;

    // If the string length of the name is smaller then one character
    // -> skip this subkey
    if (_tcslen(lpstrName) <= 1)
      continue;

    if (nType == REG_DWORD) {
      // If the subkey type is DWORD
      writer.printfln(_T("%s=%d"), lpstrName, uValue.dValue);
    } else if (nType == REG_SZ) {
      // If the subkey type is STRING

      // If the value is empty
      if (nValueSize <= 0) {
        // -> write ="" to the output file an continue with the next subkey
        writer.printfln(_T("%s=\"\""), lpstrName);
        continue;
      }

      // does it contain invalid characters?
      if (_tcspbrk(uValue.string_value, _T("\r\n\"")) != NULL) {
        // -> write ="" to the output file an continue with the next subkey
        writer.printfln(_T("%s=\"\""), lpstrName);
        continue;
      }

      /// @todo SCOTT - Check that the output data (lpstrName and pValue) do not contain \r or \n
      // Force null-termination
      uValue.pValue[nValueSize] = 0;
      uValue.pValue[nValueSize + 1] = 0;

      // If the value string is not empty
      if (!string_is_empty(uValue.string_value))
        // -> write the value to the output file
        writer.printfln(_T("%s=\"%s\""), lpstrName, uValue.pValue);
      else
        // otherwise -> write ="" to the output file
        writer.printfln(_T("%s=\"\""), lpstrName);
    }
  }

  // Close the XCSoar registry key
  ::RegCloseKey(hkFrom);
#else /* !WIN32 */
  // XXX implement
#endif /* !WIN32 */
}
