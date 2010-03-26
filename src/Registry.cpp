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
#include "StringUtil.hpp"
#include "LogFile.hpp"
#include "Defines.h"
#include "Sizes.h"
#include "IO/FileLineReader.hpp"
#include "IO/TextWriter.hpp"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef WIN32
#include <gconf/gconf.h>
#endif

#ifndef WIN32

class GConf {
protected:
  GConfEngine *engine;

public:
  GConf():engine(gconf_engine_get_default()) {}
  ~GConf() {
    gconf_engine_unref(engine);
  }

  bool get(const char *key, int &value) {
    GError *error = NULL;
    value = gconf_engine_get_int(engine, key, &error);
    if (value == 0 && error != NULL) {
      g_error_free(error);
      return false;
    }

    return true;
  }

  bool get(const char *key, char *value, size_t max_length) {
    gchar *buffer = gconf_engine_get_string(engine, key, NULL);
    if (buffer == NULL)
      return false;

    g_strlcpy(value, buffer, max_length);
    g_free(buffer);
    return true;
  }

  bool set(const char *key, int value) {
    return gconf_engine_set_int(engine, key, value, NULL);
  }

  bool set(const char *key, const char *value) {
    return gconf_engine_set_string(engine, key, value, NULL);
  }
};

#endif /* !WIN32 */

//
// NOTE: all registry variables are unsigned!
//
bool
GetFromRegistryD(const TCHAR *szRegValue, DWORD &pPos)
{
// returns 0 on SUCCESS, else the non-zero error code
#ifdef WIN32

  HKEY hKey;
  DWORD dwSize, dwType;
  long hRes;
  DWORD defaultVal;

  hRes = RegOpenKeyEx(HKEY_CURRENT_USER, szProfileKey, 0, KEY_ALL_ACCESS, &hKey);
  if (hRes != ERROR_SUCCESS)
    return hRes;

  defaultVal = pPos;
  dwSize = sizeof(DWORD);
  hRes = RegQueryValueEx(hKey, szRegValue, 0, &dwType, (LPBYTE) & pPos, &dwSize);
  if (hRes != ERROR_SUCCESS)
    pPos = defaultVal;

  RegCloseKey(hKey);
  return hRes;

#else /* !WIN32 */

  int value;
  if (!GConf().get(szRegValue, value))
    return false;

  pPos = (DWORD)value;
  return true;

#endif /* !WIN32 */
}

bool
GetFromRegistry(const TCHAR *szRegValue, int &pPos)
{
  DWORD Temp = pPos;
  long res;
  if ((res = GetFromRegistryD(szRegValue, Temp)) == ERROR_SUCCESS)
    pPos = Temp;

  return res;
}

bool
GetFromRegistry(const TCHAR *szRegValue, short &pPos)
{
  DWORD Temp = pPos;
  long res;
  if ((res = GetFromRegistryD(szRegValue, Temp)) == ERROR_SUCCESS)
    pPos = Temp;

  return res;
}

bool
GetFromRegistry(const TCHAR *szRegValue, bool &pPos)
{
  DWORD Temp = pPos;
  long res;
  if ((res = GetFromRegistryD(szRegValue, Temp)) == ERROR_SUCCESS)
    pPos = Temp > 0;

  return res;
}

bool
GetFromRegistry(const TCHAR *szRegValue, unsigned &pPos)
{
  DWORD Temp = pPos;
  long res;
  if ((res = GetFromRegistryD(szRegValue, Temp)) == ERROR_SUCCESS)
    pPos = Temp;

  return res;
}

bool
GetFromRegistry(const TCHAR *szRegValue, double &pPos)
{
  DWORD Temp = (DWORD)pPos;
  long res;
  if ((res = GetFromRegistryD(szRegValue, Temp)) == ERROR_SUCCESS)
    pPos = (double)Temp;

  return res;
}

bool
GetFromRegistry(const TCHAR *szRegValue, fixed &pPos)
{
  DWORD Temp = (DWORD)(long)pPos;
  fixed res;
  if ((res = GetFromRegistryD(szRegValue, Temp)) == ERROR_SUCCESS)
    pPos = (fixed)Temp;

  return res;
}

HRESULT
SetToRegistry(const TCHAR *szRegValue, DWORD Pos)
{
#ifdef WIN32

  HKEY hKey;
  DWORD Disp;
  HRESULT hRes;

  hRes = RegCreateKeyEx(HKEY_CURRENT_USER, szProfileKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &Disp);
  if (hRes != ERROR_SUCCESS)
    return FALSE;

  hRes = RegSetValueEx(hKey, szRegValue, 0, REG_DWORD, (LPBYTE)&Pos,
                       sizeof(DWORD));
  RegCloseKey(hKey);

  return hRes;

#else /* !WIN32 */

  GConf().set(szRegValue, (int)Pos);
  return 0;

#endif /* !WIN32 */
}

// Set bool value to registry as 1 or 0
HRESULT
SetToRegistry(const TCHAR *szRegValue, bool bVal)
{
  return SetToRegistry(szRegValue, bVal ? DWORD(1) : DWORD(0));
}

// Set int value to registry
HRESULT
SetToRegistry(const TCHAR *szRegValue, int nVal)
{
  return SetToRegistry(szRegValue, DWORD(nVal));
}

#ifndef HAVE_POSIX /* DWORD==unsigned on WINE, would be duplicate */
HRESULT
SetToRegistry(const TCHAR *szRegValue, unsigned nVal)
{
  return SetToRegistry(szRegValue, DWORD(nVal));
}
#endif

/**
 * Reads a value from the registry file
 * @param szRegValue Name of the value that should be read
 * @param pPos Pointer to the output buffer
 * @param dwSize Maximum size of the output buffer
 */
bool
GetRegistryString(const TCHAR *szRegValue, TCHAR *pPos, DWORD dwSize)
{
#ifdef WIN32

  HKEY hKey;
  DWORD dwType = REG_SZ;
  long hRes;
  unsigned int i;
  for (i = 0; i < dwSize; i++) {
    pPos[i] = 0;
  }

  pPos[0]= '\0';
  hRes = RegOpenKeyEx(HKEY_CURRENT_USER, szProfileKey, 0,
                      KEY_READ, &hKey);
  if (hRes != ERROR_SUCCESS)
    return false;

  dwSize *= sizeof(pPos[0]);

  hRes = RegQueryValueEx(hKey, szRegValue, 0, &dwType, (LPBYTE)pPos, &dwSize);

  RegCloseKey(hKey);
  return hRes == ERROR_SUCCESS;

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
SetRegistryString(const TCHAR *szRegValue, const TCHAR *Pos)
{
#ifdef WIN32

  HKEY    hKey;
  DWORD    Disp;
  HRESULT hRes;

  hRes = RegCreateKeyEx(HKEY_CURRENT_USER, szProfileKey, 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &hKey, &Disp);
  if (hRes != ERROR_SUCCESS)
    return false;

  hRes = RegSetValueEx(hKey, szRegValue, 0, REG_SZ, (LPBYTE)Pos,
                       (_tcslen(Pos) + 1) * sizeof(TCHAR));
  RegCloseKey(hKey);

  return hRes == ERROR_SUCCESS;

#else /* !WIN32 */
  return GConf().set(szRegValue, Pos);
#endif /* !WIN32 */
}

// Registry file handling

const static size_t nMaxValueValueSize = MAX_PATH * 2 + 6; // max regkey name is 256 chars + " = "
const static size_t nMaxKeyNameSize = MAX_PATH + 6;

void
LoadRegistryFromFile(const TCHAR *szFile)
{
  if (string_is_empty(szFile))
    return;

  LogStartUp(TEXT("Loading registry from %s"), szFile);
  FileLineReader reader(szFile);
  if (reader.error())
    return;

  const TCHAR *winval;
  TCHAR wname[nMaxValueValueSize];
  TCHAR wvalue[nMaxValueValueSize];
  int j;

  while ((winval = reader.read()) != NULL) {
    if (_stscanf(winval, TEXT("%[^#=\r\n ]=\"%[^\r\n\"]\"[\r\n]"),
                 wname, wvalue) == 2) {
      if (!string_is_empty(wname))
        SetRegistryString(wname, wvalue);
    } else if (_stscanf(winval, TEXT("%[^#=\r\n ]=%d[\r\n]"), wname, &j) == 2) {
      if (!string_is_empty(wname))
        SetToRegistry(wname, j);
    } else if (_stscanf(winval, TEXT("%[^#=\r\n ]=\"\"[\r\n]"), wname) == 1) {
      if (!string_is_empty(wname))
        SetRegistryString(wname, TEXT(""));
    } else {
      // assert(false); // Invalid line reached
    }
  }
}

void
SaveRegistryToFile(const TCHAR *szFile)
{
#ifdef WIN32
  TCHAR lpstrName[nMaxKeyNameSize+1];

  union {
    BYTE pValue[nMaxValueValueSize+4];
    DWORD dValue;
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

      /// @todo SCOTT - Check that the output data (lpstrName and pValue) do not contain \r or \n
      // Force null-termination
      uValue.pValue[nValueSize] = 0;
      uValue.pValue[nValueSize + 1] = 0;

      // If the value string is not empty
      if (!string_is_empty((const TCHAR*)uValue.pValue))
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
