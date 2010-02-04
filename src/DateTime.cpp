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

#include "DateTime.hpp"

#ifdef HAVE_POSIX
#include <time.h>
#else
#include <windows.h>
#endif

#ifdef HAVE_POSIX

static const BrokenDateTime
ToBrokenDateTime(const struct tm &tm)
{
  BrokenDateTime dt;

  dt.year = tm.tm_year + 1900;
  dt.month = tm.tm_mon + 1;
  dt.day = tm.tm_mday;
  dt.hour = tm.tm_hour;
  dt.minute = tm.tm_min;
  dt.second = tm.tm_sec;

  return dt;
}

#else /* !HAVE_POSIX */

static const BrokenDateTime
ToBrokenDateTime(const SYSTEMTIME st)
{
  BrokenDateTime dt;

  dt.year = st.wYear;
  dt.month = st.wMonth;
  dt.day = st.wDay;
  dt.hour = st.wHour;
  dt.minute = st.wMinute;
  dt.second = st.wSecond;

  return dt;
}

#endif /* !HAVE_POSIX */

const BrokenDateTime
BrokenDateTime::NowUTC()
{
#ifdef HAVE_POSIX
  time_t t = time(NULL);
  struct tm tm;
  gmtime_r(&t, &tm);

  return ToBrokenDateTime(tm);
#else /* !HAVE_POSIX */
  SYSTEMTIME st;
  GetSystemTime(&st);

  return ToBrokenDateTime(st);
#endif /* !HAVE_POSIX */
}

const BrokenDateTime
BrokenDateTime::NowLocal()
{
#ifdef HAVE_POSIX
  time_t t = time(NULL);
  struct tm tm;
  localtime_r(&t, &tm);

  return ToBrokenDateTime(tm);
#else /* !HAVE_POSIX */
  SYSTEMTIME st;
  GetLocalTime(&st);

  return ToBrokenDateTime(st);
#endif /* !HAVE_POSIX */
}
