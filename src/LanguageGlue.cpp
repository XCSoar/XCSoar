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

#include "LanguageGlue.hpp"
#include "Language.hpp"
#include "LocalPath.hpp"
#include "OS/PathName.hpp"
#include "LogFile.hpp"
#include "Profile/Profile.hpp"
#include "Util/StringUtil.hpp"

#if defined(HAVE_POSIX) && !defined(ANDROID)
#include <locale.h>
#endif

#ifdef HAVE_BUILTIN_LANGUAGES
#include "ResourceLoader.hpp"
#endif

#ifdef _WIN32_WCE
#include "OS/DynamicLibrary.hpp"
#endif

#ifdef ANDROID
#include "Java/Global.hpp"
#include "Java/Class.hpp"
#include "Java/Object.hpp"
#endif

#include <windef.h> /* for MAX_PATH */

#if !defined(HAVE_POSIX) || defined(ANDROID)

#include "MOLoader.hpp"

static MOLoader *mo_loader;

#endif

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
  LANG_DANISH,
  LANG_GERMAN,
  LANG_GREEK,
  LANG_SPANISH,
  LANG_FRENCH,
  LANG_CROATIAN,
  LANG_HUNGARIAN,
  LANG_ITALIAN,
  LANG_NORWEGIAN,
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
  { LANG_CZECH, _T("cs.mo") },
  { LANG_DANISH, _T("da.mo") },
  { LANG_GERMAN, _T("de.mo") },
  { LANG_GREEK, _T("el.mo") },
  { LANG_SPANISH, _T("es.mo") },
  { LANG_FRENCH, _T("fr.mo") },
  { LANG_CROATIAN, _T("hr.mo") },
  { LANG_HUNGARIAN, _T("hu.mo") },
  { LANG_ITALIAN, _T("it.mo") },
  { LANG_NORWEGIAN, _T("nb.mo") },
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
  // Search for supported languages matching the language code
  for (unsigned i = 0; language_table[i].resource != NULL; ++i)
    if (language_table[i].language == language)
      // .. and return the MO file name if found
      return language_table[i].resource;

  return NULL;
}

#endif

gcc_pure
static unsigned
find_language(const TCHAR *resource)
{
  assert(resource != NULL);

  // Search for supported languages matching the MO file name
  for (unsigned i = 0; language_table[i].resource != NULL; ++i)
    if (_tcscmp(language_table[i].resource, resource) == 0)
      // .. and return the language code
      return language_table[i].language;

  return 0;
}

static const TCHAR *
detect_language()
{
#ifdef ANDROID

  JNIEnv *env = Java::GetEnv();

  Java::Class cls(env, "java/util/Locale");

  // Call static function Locale.getDefault() that
  // returns the user's default Locale object

  jmethodID cid = env->GetStaticMethodID(cls, "getDefault",
                                         "()Ljava/util/Locale;");
  assert(cid != NULL);

  jobject _obj = env->CallStaticObjectMethod(cls, cid);
  if (_obj == NULL)
    return NULL;

  Java::LocalObject obj(env, _obj);

  // Call function Locale.getLanguage() that
  // returns a two-letter language string

  cid = env->GetMethodID(cls, "getLanguage", "()Ljava/lang/String;");
  assert(cid != NULL);

  jstring language = (jstring)env->CallObjectMethod(obj, cid);
  if (language == NULL)
    return NULL;

  // Convert the jstring to a char string
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

  // Attach .mo to the language identifier
  static char language_buffer[16];
  snprintf(language_buffer, sizeof(language_buffer), "%s.mo", language3);

  // Clean up the memory
  env->ReleaseStringUTFChars(language, language2);
  env->DeleteLocalRef(language);

  // Return e.g. "de.mo"
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

  // Retrieve the default user language identifier from the OS
  LANGID lang_id = GetUserDefaultUILanguage();
  LogStartUp(_T("Language: GetUserDefaultUILanguage()=0x%x"), (int)lang_id);
  if (lang_id == 0)
    return NULL;

  // Try to convert the primary language part of the language identifier
  // to a MO file name in the language table
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

  // Load resource
  ResourceLoader::Data data = ResourceLoader::Load(resource, _T("MO"));
  if (data.first == NULL) {
    LogStartUp(_T("Language: resource '%s' not found"), resource);
    return false;
  }

  // Load MO file from resource
  delete mo_loader;
  mo_loader = new MOLoader(data.first, data.second);
  if (mo_loader->error()) {
    LogStartUp(_T("Language: could not load resource '%s'"), resource);
    delete mo_loader;
    mo_loader = NULL;
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

  // Set the current locale to the environment's default
  setlocale(LC_ALL, "");
  // always use a dot as decimal point in printf/scanf()
  setlocale(LC_NUMERIC, "C");
  bindtextdomain("xcsoar", "/usr/share/locale");
  textdomain("xcsoar");

#else /* !HAVE_POSIX */

  // Try to detect the language by calling the OS's corresponding functions
  const TCHAR *resource = detect_language();
  if (resource != NULL)
    // If a language was detected -> try to load the MO file
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

  delete mo_loader;
  mo_loader = new MOLoader(path);
  if (mo_loader->error()) {
    LogStartUp(_T("Language: could not load file '%s'"), path);
    delete mo_loader;
    mo_loader = NULL;
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
#if !defined(HAVE_POSIX) || defined(ANDROID)
  CloseLanguageFile();
#endif

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

void
CloseLanguageFile()
{
#if !defined(HAVE_POSIX) || defined(ANDROID)
  mo_file = NULL;
  reset_gettext_cache();
  delete mo_loader;
  mo_loader = NULL;
#endif
}
