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

#include "Language.hpp"
#include "LocalPath.hpp"
#include "UtilsText.hpp"
#include "StringUtil.hpp"
#include "OS/PathName.hpp"
#include "LogFile.hpp"
#include "Profile/Profile.hpp"
#include "Sizes.h"

#ifdef HAVE_BUILTIN_LANGUAGES
#include "ResourceLoader.hpp"
#endif

#ifdef _WIN32_WCE
#include "OS/DynamicLibrary.hpp"
#endif

#ifdef ANDROID
#include "Java/Global.hpp"
#endif

#include <windef.h> /* for MAX_PATH */

#if defined(HAVE_POSIX) && !defined(ANDROID)

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
  const char *translation = mo_file->lookup(text);
  return translation != NULL && *translation != 0
    ? translation
    : text;
#endif
}

#endif /* !HAVE_POSIX */

#ifdef HAVE_BUILTIN_LANGUAGES

#ifdef ANDROID
/**
 * Several fake WIN32 constants.  These are not used on Android, but
 * we need them or we have to have a separate version of
 * #language_table on Android.
 */
enum {
  LANG_NULL,
  LANG_CZECH,
  LANG_GERMAN,
  LANG_GREEK,
  LANG_SPANISH,
  LANG_FRENCH,
  LANG_CROATIAN,
  LANG_HUNGARIAN,
  LANG_ITALIAN,
  LANG_DUTCH,
  LANG_POLISH,
  LANG_PORTUGUESE,
  LANG_RUSSIAN,
  LANG_SLOVAK,
  LANG_SERBIAN,
  LANG_SWEDISH,
  LANG_TURKISH,
};
#endif

const struct builtin_language language_table[] = {
  { LANG_CZECH, _T("cz.mo") },
  { LANG_GERMAN, _T("de.mo") },
  { LANG_GREEK, _T("el.mo") },
  { LANG_SPANISH, _T("es.mo") },
  { LANG_FRENCH, _T("fr.mo") },
  { LANG_CROATIAN, _T("hr.mo") },
  { LANG_HUNGARIAN, _T("hu.mo") },
  { LANG_ITALIAN, _T("it.mo") },
  { LANG_DUTCH, _T("nl.mo") },
  { LANG_POLISH, _T("pl.mo") },
  { LANG_PORTUGUESE, _T("pt_BR.mo") },
  { LANG_RUSSIAN, _T("ru.mo") },
  { LANG_SLOVAK, _T("sk.mo") },
  { LANG_SERBIAN, _T("sr.mo") },
  { LANG_SWEDISH, _T("sv.mo") },
  { LANG_TURKISH, _T("tr.mo") },
  { 0, NULL }
};

#ifdef WIN32

gcc_pure
static const TCHAR *
find_language(WORD language)
{
  for (unsigned i = 0; language_table[i].resource != NULL; ++i)
    if (language_table[i].language == language)
      return language_table[i].resource;

  return NULL;
}

#endif

gcc_pure
static unsigned
find_language(const TCHAR *resource)
{
  assert(resource != NULL);

  for (unsigned i = 0; language_table[i].resource != NULL; ++i)
    if (_tcscmp(language_table[i].resource, resource) == 0)
      return language_table[i].language;

  return 0;
}

static const TCHAR *
detect_language()
{
#ifdef ANDROID

  JNIEnv *env = Java::GetEnv();

  jclass cls = env->FindClass("java/util/Locale");
  assert(cls != NULL);

  /* call Locale.getDefault() */

  jmethodID cid = env->GetStaticMethodID(cls, "getDefault",
                                         "()Ljava/util/Locale;");
  assert(cid != NULL);

  jobject obj = env->CallStaticObjectMethod(cls, cid);
  if (obj == NULL) {
    env->DeleteLocalRef(cls);
    return NULL;
  }

  /* call Locale.getLanguage() */

  cid = env->GetMethodID(cls, "getLanguage", "()Ljava/lang/String;");
  assert(cid != NULL);

  env->DeleteLocalRef(cls);

  jstring language = (jstring)env->CallObjectMethod(obj, cid);
  if (language == NULL) {
    env->DeleteLocalRef(obj);
    return NULL;
  }

  env->DeleteLocalRef(obj);

  const char *language2 = env->GetStringUTFChars(language, NULL);
  if (language2 == NULL) {
    env->DeleteLocalRef(language);
    return NULL;
  }

  /* generate the resource name */

  const char *language3 = language2;
  if (strcmp(language3, "pt") == 0)
    /* hack */
    language3 = "pt_BR";

  static char language_buffer[16];
  snprintf(language_buffer, sizeof(language_buffer), "%s.mo", language3);

  env->ReleaseStringUTFChars(language, language2);
  env->DeleteLocalRef(language);

  return language_buffer;

#else /* !ANDROID */

#if defined(_WIN32_WCE)
  /* the GetUserDefaultUILanguage() prototype is missing on
     mingw32ce, we have to look it up dynamically */
  DynamicLibrary coreloc_dll(_T("coredll"));
  if (!coreloc_dll.defined()) {
    LogStartUp(_T("Language: coredll.dll not found"));
    return NULL;
  }

  typedef LANGID WINAPI (*GetUserDefaultUILanguage_t)();
  GetUserDefaultUILanguage_t GetUserDefaultUILanguage =
    (GetUserDefaultUILanguage_t)
    coreloc_dll.lookup(_T("GetUserDefaultUILanguage"));
  if (GetUserDefaultUILanguage == NULL) {
    LogStartUp(_T("Language: GetUserDefaultUILanguage() not available"));
    return NULL;
  }
#endif

  LANGID lang_id = GetUserDefaultUILanguage();
  LogStartUp(_T("Language: GetUserDefaultUILanguage()=0x%x"), (int)lang_id);
  if (lang_id == 0)
    return NULL;

  return find_language(PRIMARYLANGID(lang_id));

#endif /* !ANDROID */
}

static bool
ReadResourceLanguageFile(const TCHAR *resource)
{
  if (!find_language(resource))
    /* refuse to load resources which are not in the language table */
    return false;

  LogStartUp(_T("Language: loading resource '%s'"), resource);

  ResourceLoader::Data data = ResourceLoader::Load(resource, _T("MO"));
  if (data.first == NULL) {
    LogStartUp(_T("Language: resource '%s' not found"), resource);
    return false;
  }

  mo_loader.reset(new MOLoader(data.first, data.second));
  if (mo_loader->error()) {
    LogStartUp(_T("Language: could not load resource '%s'"), resource);
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

static void
AutoDetectLanguage()
{
#if defined(HAVE_POSIX) && !defined(ANDROID)

  setlocale(LC_ALL, "");
  // allways use a dot as decimal point in printf/scanf.
  setlocale(LC_NUMERIC, "C");
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
#if defined(HAVE_POSIX) && !defined(ANDROID)

  /* not supported on UNIX */
  return false;

#else /* !HAVE_POSIX */

  LogStartUp(_T("Language: loading file '%s'"), path);

  mo_loader.reset(new MOLoader(path));
  if (mo_loader->error()) {
    LogStartUp(_T("Language: could not load file '%s'"), path);
    mo_loader.reset();
    return false;
  }

  LogStartUp(_T("Loaded translations from file '%s'"), path);

  mo_file = &mo_loader->get();
  return true;

#endif /* !HAVE_POSIX */
}

/**
 * Reads the selected LanguageFile into the cache
 */
void
ReadLanguageFile()
{
  LogStartUp(_T("Loading language file"));

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
  }

  if (!LoadLanguageFile(value) && !ReadResourceLanguageFile(base))
    AutoDetectLanguage();
}
