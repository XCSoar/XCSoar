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

#ifndef XCSOAR_LANGUAGE_HPP
#define XCSOAR_LANGUAGE_HPP

#ifdef ANDROID

/* disable i18n on Android for now */
#define _(x) (x)
#define N_(x) (x)
#define gettext(x) (x)

#elif defined(HAVE_POSIX)

#include <libintl.h>

#define _(x) gettext(x)

#ifdef gettext_noop
#define N_(x) gettext_noop(x)
#else
#define N_(x) (x)
#endif

#else // !HAVE_POSIX

#include "Compiler.h"

#include <tchar.h>

gcc_const
const TCHAR* gettext(const TCHAR* text);

/**
 * For source compatibility with GNU gettext.
 */
#define _(x) gettext(_T(x))
#define N_(x) _T(x)

#if !defined(_WIN32_WCE) && !defined(NDEBUG) && defined(_MSC_VER)
#pragma warning( disable : 4786 )
#endif

#endif // !HAVE_POSIX

void ReadLanguageFile(void);

#if defined(WIN32) && !defined(HAVE_POSIX) && \
  (!defined(_WIN32_WCE) || _WIN32_WCE >= 0x500)
#define HAVE_BUILTIN_LANGUAGES

struct builtin_language {
  unsigned language;
  const TCHAR *resource;
};

extern const struct builtin_language language_table[];

#endif

#endif
