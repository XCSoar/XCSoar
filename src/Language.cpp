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

#include "Language.hpp"
#include "LocalPath.hpp"
#include "UtilsText.hpp"
#include "StringUtil.hpp"
#include "OS/PathName.hpp"
#include "LogFile.hpp"
#include "Profile/Profile.hpp"
#include "Sizes.h"

#ifdef WIN32
#include "ResourceLoader.hpp"
#endif

#ifdef _WIN32_WCE
#include "OS/DynamicLibrary.hpp"
#endif

#ifdef ANDROID

#elif defined(HAVE_POSIX)

#include <locale.h>

#else

#include "MOLoader.hpp"

#include <memory>

static std::auto_ptr<MOLoader> mo_loader;
static const MOFile *mo_file;

#ifdef _UNICODE
#include "Util/tstring.hpp"
#include <map>
typedef std::map<tstring,tstring> translation_map;
static translation_map translations;
#endif

/**
 * Looks up a string of text from the current language file
 *
 * Currently very simple. Looks up the current string and current language
 * to find the appropriate string response. On failure will return
 * the string itself.
 *
 * NOTES CACHING:
 * - Could load the whole file or part
 * - qsort/bsearch good idea
 * - cache misses in data structure for future use
 * @param text The text to search for
 * @return The translation if found, otherwise the text itself
 */
const TCHAR*
gettext(const TCHAR* text)
{
  assert(text != NULL);

  if (string_is_empty(text) || mo_file == NULL)
    return text;

#ifdef _UNICODE
  const tstring text2(text);
  translation_map::const_iterator it = translations.find(text2);
  if (it != translations.end())
    return it->second.c_str();

  size_t wide_length = _tcslen(text);
  char original[wide_length * 4 + 1];

  if (::WideCharToMultiByte(CP_UTF8, 0, text, -1,
                            original, sizeof(original), NULL, NULL) <= 0)
    return text;

  const char *translation = mo_file->lookup(original);
  if (translation == NULL || *translation == 0 ||
      strcmp(original, translation) == 0)
    return text;

  TCHAR translation2[strlen(translation) + 1];
  if (::MultiByteToWideChar(CP_UTF8, 0, translation, -1, translation2,
                            sizeof(translation2) / sizeof(translation2[0])) <= 0)
    return text;

  translations[text2] = translation2;
  return translations[text2].c_str();
#else
  return mo_file->lookup(text);
#endif
}

#endif /* !HAVE_POSIX */

#ifdef HAVE_BUILTIN_LANGUAGES

const struct builtin_language language_table[] = {
  { LANG_GERMAN, _T("de.mo") },
  { LANG_FRENCH, _T("fr.mo") },
  { LANG_HUNGARIAN, _T("hu.mo") },
  { LANG_DUTCH, _T("nl.mo") },
  { LANG_POLISH, _T("pl.mo") },
  { LANG_PORTUGUESE, _T("pt_BR.mo") },
  { LANG_SLOVAK, _T("sk.mo") },
  { 0, NULL }
};

static const TCHAR *
find_language(WORD language)
{
  for (unsigned i = 0; language_table[i].resource != NULL; ++i)
    if (language_table[i].language == language)
      return language_table[i].resource;

  return NULL;
}

static const TCHAR *
detect_language()
{
#if defined(_WIN32_WCE)
  /* the GetUserDefaultUILanguage() prototype is missing on
     mingw32ce, we have to look it up dynamically */
  DynamicLibrary coreloc_dll(_T("coredll"));
  if (!coreloc_dll.defined())
    return NULL;

  typedef LANGID WINAPI (*GetUserDefaultUILanguage_t)();
  GetUserDefaultUILanguage_t GetUserDefaultUILanguage =
    (GetUserDefaultUILanguage_t)
    coreloc_dll.lookup(_T("GetUserDefaultUILanguage"));
  if (GetUserDefaultUILanguage == NULL)
    return NULL;
#endif

  LANGID lang_id = GetUserDefaultUILanguage();
  if (lang_id == 0)
    return NULL;

  return find_language(PRIMARYLANGID(lang_id));
}

static bool
ReadResourceLanguageFile(const TCHAR *resource)
{
  ResourceLoader::Data data = ResourceLoader::Load(resource, _T("MO"));
  if (data.first == NULL)
    return false;

  mo_loader.reset(new MOLoader(data.first, data.second));
  if (mo_loader->error()) {
    mo_loader.reset();
    return false;
  }

  LogStartUp(_T("Loaded translations from resource '%s'"), resource);

  mo_file = &mo_loader->get();
  return true;
}

#else /* !HAVE_BUILTIN_LANGUAGES */

static inline const TCHAR *
detect_language()
{
  return NULL;
}

static inline bool
ReadResourceLanguageFile(const TCHAR *resource)
{
  return false;
}

#endif /* !HAVE_BUILTIN_LANGUAGES */

#ifndef ANDROID

static void
AutoDetectLanguage()
{
#if defined(HAVE_POSIX)

  setlocale(LC_ALL, "");
  bindtextdomain("xcsoar", "/usr/share/locale");
  textdomain("xcsoar");

#else /* !HAVE_POSIX */

  const TCHAR *resource = detect_language();
  if (resource != NULL)
    ReadResourceLanguageFile(resource);

#endif /* !HAVE_POSIX */
}

static bool
LoadLanguageFile(const TCHAR *path)
{
#if defined(HAVE_POSIX)

  /* not supported on UNIX */
  return false;

#else /* !HAVE_POSIX */

  mo_loader.reset(new MOLoader(path));
  if (mo_loader->error()) {
    mo_loader.reset();
    return false;
  }

  LogStartUp(_T("Loaded translations from file '%s'"), path);

  mo_file = &mo_loader->get();
  return true;

#endif /* !HAVE_POSIX */
}

#endif /* !ANDROID */

/**
 * Reads the selected LanguageFile into the cache
 */
void
ReadLanguageFile()
{
  LogStartUp(_T("Loading language file"));

#ifndef ANDROID

  TCHAR buffer[MAX_PATH], second_buffer[MAX_PATH];
  const TCHAR *value = Profile::GetPath(szProfileLanguageFile, buffer)
    ? buffer : _T("");

  if (_tcscmp(value, _T("none")) == 0)
    return;

  if (string_is_empty(value) || _tcscmp(value, _T("auto")) == 0) {
    AutoDetectLanguage();
    return;
  }

  const TCHAR *base = BaseName(value);
  if (base == NULL)
    base = value;

  if (base == value) {
    LocalPath(second_buffer, value);
    value = second_buffer;

    if (!LoadLanguageFile(value) && !ReadResourceLanguageFile(base))
      AutoDetectLanguage();
  } else {
    if (!LoadLanguageFile(value))
      AutoDetectLanguage();
  }

#endif /* !ANDROID */
}
