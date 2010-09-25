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

#ifndef OS_PATH_HPP
#define OS_PATH_HPP

#ifdef _UNICODE
#include "Util/StringUtil.hpp"

#include <windows.h>
#include <string.h>
#endif

#include <tchar.h>

/**
 * Representation of a file name.  It is automatically converted to
 * the file system character set.  If no conversion is needed, then
 * this object will hold a pointer to the original input string; it
 * must not be invalidated.
 */
class PathName {
#ifdef _UNICODE
  TCHAR *allocated;
#endif
  const TCHAR *value;

public:
#ifdef _UNICODE
  explicit PathName(const TCHAR *_value)
    :allocated(NULL), value(_value) {}

  explicit PathName(const char *_value)
    :allocated(convert(_value)), value(allocated) {}

  ~PathName() {
    delete allocated;
  }

protected:
  static TCHAR *convert(const char *src) {
    if (string_is_empty(src))
      return _tcsdup(_T(""));

    TCHAR dest[MAX_PATH];
    int length = ::MultiByteToWideChar(CP_ACP, 0, src, -1, dest, MAX_PATH);
    if (length == 0)
      return NULL;

    return _tcsdup(dest);
  }
#else /* !_UNICODE */
  explicit PathName(const TCHAR *_value):value(_value) {}
#endif /* !_UNICODE */

public:
  bool defined() const {
#ifdef _UNICODE
    return value != NULL;
#else
    return true;
#endif
  }

  operator const TCHAR *() const {
    return value;
  }
};

/**
 * Representation of a file name in narrow characters.  If no
 * conversion is needed, then this object will hold a pointer to the
 * original input string; it must not be invalidated.
 */
class NarrowPathName {
#ifdef _UNICODE
  char *allocated;
#endif
  const char *value;

public:
#ifdef _UNICODE
  explicit NarrowPathName(const char *_value)
    :allocated(NULL), value(_value) {}

  explicit NarrowPathName(const TCHAR *_value)
    :allocated(convert(_value)), value(allocated) {}

  ~NarrowPathName() {
    delete allocated;
  }

protected:
  static char *convert(const TCHAR *src) {
    if (string_is_empty(src))
      return strdup("");

    char dest[MAX_PATH];
    int length = ::WideCharToMultiByte(CP_ACP, 0, src, -1, dest, MAX_PATH,
                                       NULL, NULL);
    if (length == 0)
      return NULL;

    return strdup(dest);
  }
#else /* !_UNICODE */
  explicit NarrowPathName(const char *_value):value(_value) {}
#endif /* !_UNICODE */

public:
  bool defined() const {
#ifdef _UNICODE
    return value != NULL;
#else
    return true;
#endif
  }

  operator const char *() const {
    return value;
  }
};

/**
 * Returns the base name of the specified path, i.e. the part after
 * the last separator.  May return NULL if there is no base name.
 */
const TCHAR *
BaseName(const TCHAR *path);

/**
 * Returns the directory name of the specified path, i.e. the part
 * before the last separator.  Returns "." if there is no directory
 * name.
 */
const TCHAR *
DirName(const TCHAR *path, TCHAR *buffer);

#endif
