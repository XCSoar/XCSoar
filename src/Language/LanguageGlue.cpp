/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "OS/Path.hpp"
#include "LogFile.hpp"
#include "Profile/Profile.hpp"
#include "Util/StringCompare.hxx"
#include "Util/StringAPI.hxx"

#ifdef HAVE_NATIVE_GETTEXT
#include <locale.h>
#endif

#ifdef ANDROID
#include "Java/Global.hxx"
#include "Java/Class.hxx"
#include "Java/Object.hxx"
#endif

#ifdef WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#endif

#include <windef.h> /* for MAX_PATH */

#ifndef HAVE_NATIVE_GETTEXT

#include "MOLoader.hpp"

static MOLoader *mo_loader;

#endif

#ifdef HAVE_BUILTIN_LANGUAGES

#ifndef WIN32
/**
 * Several fake WIN32 constants.  These are not used on Android, but
 * we need them or we have to have a separate version of
 * #language_table on Android.
 */
enum {
  LANG_NULL,
  LANG_CHINESE,
  LANG_CZECH,
  LANG_DANISH,
  LANG_GERMAN,
  LANG_GREEK,
  LANG_SPANISH,
  LANG_FINNISH,
  LANG_FRENCH,
  LANG_HEBREW,
  LANG_CROATIAN,
  LANG_HUNGARIAN,
  LANG_ITALIAN,
  LANG_JAPANESE,
  LANG_KOREAN,
  LANG_LITHUANIAN,
  LANG_NORWEGIAN,
  LANG_DUTCH,
  LANG_POLISH,
  LANG_PORTUGUESE,
  LANG_ROMANIAN,
  LANG_RUSSIAN,
  LANG_SLOVAK,
  LANG_SLOVENIAN,
  LANG_SERBIAN,
  LANG_SWEDISH,
  LANG_TURKISH,
  LANG_UKRAINIAN,
  LANG_VIETNAMESE,
};
#endif

extern "C"
{
  extern const uint8_t cs_mo[];
  extern const size_t cs_mo_size;
  extern const uint8_t da_mo[];
  extern const size_t da_mo_size;
  extern const uint8_t de_mo[];
  extern const size_t de_mo_size;
  extern const uint8_t el_mo[];
  extern const size_t el_mo_size;
  extern const uint8_t es_mo[];
  extern const size_t es_mo_size;
  extern const uint8_t fi_mo[];
  extern const size_t fi_mo_size;
  extern const uint8_t fr_mo[];
  extern const size_t fr_mo_size;
  extern const uint8_t he_mo[];
  extern const size_t he_mo_size;
  extern const uint8_t hr_mo[];
  extern const size_t hr_mo_size;
  extern const uint8_t hu_mo[];
  extern const size_t hu_mo_size;
  extern const uint8_t it_mo[];
  extern const size_t it_mo_size;
  extern const uint8_t ja_mo[];
  extern const size_t ja_mo_size;
  extern const uint8_t ko_mo[];
  extern const size_t ko_mo_size;
  extern const uint8_t lt_mo[];
  extern const size_t lt_mo_size;
  extern const uint8_t nb_mo[];
  extern const size_t nb_mo_size;
  extern const uint8_t nl_mo[];
  extern const size_t nl_mo_size;
  extern const uint8_t pl_mo[];
  extern const size_t pl_mo_size;
  extern const uint8_t pt_BR_mo[];
  extern const size_t pt_BR_mo_size;
  extern const uint8_t pt_mo[];
  extern const size_t pt_mo_size;
  extern const uint8_t ro_mo[];
  extern const size_t ro_mo_size;
  extern const uint8_t ru_mo[];
  extern const size_t ru_mo_size;
  extern const uint8_t sk_mo[];
  extern const size_t sk_mo_size;
  extern const uint8_t sl_mo[];
  extern const size_t sl_mo_size;
  extern const uint8_t sr_mo[];
  extern const size_t sr_mo_size;
  extern const uint8_t sv_mo[];
  extern const size_t sv_mo_size;
  extern const uint8_t tr_mo[];
  extern const size_t tr_mo_size;
  extern const uint8_t uk_mo[];
  extern const size_t uk_mo_size;
  extern const uint8_t vi_mo[];
  extern const size_t vi_mo_size;
  extern const uint8_t zh_CN_mo[];
  extern const size_t zh_CN_mo_size;
}

const BuiltinLanguage language_table[] = {
  { LANG_CHINESE, zh_CN_mo, zh_CN_mo_size, _T("zh_CN.mo"), _T("Simplified Chinese") },
  { LANG_CZECH, cs_mo, cs_mo_size, _T("cs.mo"), _T("Czech") },
  { LANG_DANISH, da_mo, da_mo_size, _T("da.mo"), _T("Danish") },
  { LANG_GERMAN, de_mo, de_mo_size, _T("de.mo"), _T("German") },
  { LANG_GREEK, el_mo, el_mo_size, _T("el.mo"), _T("Greek") },
  { LANG_SPANISH, es_mo, es_mo_size, _T("es.mo"), _T("Spanish") },
  { LANG_FINNISH, fi_mo, fi_mo_size, _T("fi.mo"), _T("Finnish") },
  { LANG_FRENCH, fr_mo, fr_mo_size, _T("fr.mo"), _T("French") },
  { LANG_HEBREW, he_mo, he_mo_size, _T("he.mo"), _T("Hebrew") },
  { LANG_CROATIAN, hr_mo, hr_mo_size, _T("hr.mo"), _T("Croatian") },
  { LANG_HUNGARIAN, hu_mo, hu_mo_size, _T("hu.mo"), _T("Hungarian") },
  { LANG_ITALIAN, it_mo, it_mo_size, _T("it.mo"), _T("Italian") },
  { LANG_JAPANESE, ja_mo, ja_mo_size, _T("ja.mo"), _T("Japanese") },
  { LANG_KOREAN, ko_mo, ko_mo_size, _T("ko.mo"), _T("Korean") },
  { LANG_LITHUANIAN, lt_mo, lt_mo_size, _T("lt.mo"), _T("Lithuanian") },
  { LANG_NORWEGIAN, nb_mo, nb_mo_size, _T("nb.mo"), _T("Norwegian") },
  { LANG_DUTCH, nl_mo, nl_mo_size, _T("nl.mo"), _T("Dutch") },
  { LANG_POLISH, pl_mo, pl_mo_size, _T("pl.mo"), _T("Polish") },
  { LANG_PORTUGUESE, pt_BR_mo, pt_BR_mo_size, _T("pt_BR.mo"), _T("Brazilian Portuguese") },

  /* our Portuguese translation is less advanced than Brazilian
     Portuguese */
  { LANG_PORTUGUESE, pt_mo, pt_mo_size, _T("pt.mo"), _T("Portuguese") },

  { LANG_ROMANIAN, ro_mo, ro_mo_size, _T("ro.mo"), _T("Romanian") },
  { LANG_RUSSIAN, ru_mo, ru_mo_size, _T("ru.mo"), _T("Russian") },
  { LANG_SLOVAK, sk_mo, sk_mo_size, _T("sk.mo"), _T("Slovak") },
  { LANG_SLOVENIAN, sl_mo, sl_mo_size, _T("sl.mo"), _T("Slovenian") },
  { LANG_SERBIAN, sr_mo, sr_mo_size, _T("sr.mo"), _T("Serbian") },
  { LANG_SWEDISH, sv_mo, sv_mo_size, _T("sv.mo"), _T("Swedish") },
  { LANG_TURKISH, tr_mo, tr_mo_size, _T("tr.mo"), _T("Turkish") },
  { LANG_UKRAINIAN, uk_mo, uk_mo_size, _T("uk.mo"), _T("Ukranian") },
  { LANG_VIETNAMESE, vi_mo, vi_mo_size, _T("vi.mo"), _T("Vietnamese") },
  { 0, nullptr, 0, nullptr, nullptr }
};

#ifdef WIN32

gcc_pure
static const BuiltinLanguage *
FindLanguage(WORD language)
{
  // Search for supported languages matching the language code
  for (unsigned i = 0; language_table[i].resource != NULL; ++i)
    if (language_table[i].language == language)
      // .. and return the MO file name if found
      return &language_table[i];

  return NULL;
}

#endif

gcc_pure
static const BuiltinLanguage *
FindLanguage(const TCHAR *resource)
{
  assert(resource != NULL);

  // Search for supported languages matching the MO file name
  for (unsigned i = 0; language_table[i].resource != NULL; ++i)
    if (StringIsEqual(language_table[i].resource, resource))
      // .. and return the language code
      return &language_table[i];

  return nullptr;
}

static const BuiltinLanguage *
DetectLanguage()
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
  return FindLanguage(language_buffer);

#elif defined(WIN32)

  // Retrieve the default user language identifier from the OS
  LANGID lang_id = GetUserDefaultUILanguage();
  LogFormat("Language: GetUserDefaultUILanguage()=0x%x", (int)lang_id);
  if (lang_id == 0)
    return NULL;

  // Try to convert the primary language part of the language identifier
  // to a MO file name in the language table
  return FindLanguage(PRIMARYLANGID(lang_id));

#elif defined(__APPLE__)

  NSArray *preferred_languages = [NSLocale preferredLanguages];
  if (nil != preferred_languages) {
    for (NSString *lang_str in preferred_languages) {
      if ([lang_str hasPrefix: @"en"])
        return nullptr;
      NSString *lang_res = [NSString stringWithFormat: @"%@.mo", lang_str];
      const BuiltinLanguage *lang = FindLanguage([lang_res UTF8String]);
      if (nullptr != lang)
        return lang;
      if ([lang_str length] > 2) {
        lang_res = [NSString stringWithFormat: @"%@.mo",
            [lang_str substringToIndex: 2]];
        lang = FindLanguage([lang_res UTF8String]);
        if (nullptr != lang)
          return lang;
      }
    }
  }

  return nullptr;

#else

  return nullptr;
#endif
}

static bool
ReadBuiltinLanguage(const BuiltinLanguage &language)
{
  LogFormat(_T("Language: loading resource '%s'"), language.resource);

  // Load MO file from resource
  delete mo_loader;
  mo_loader = new MOLoader(language.begin, (size_t)language.size);
  if (mo_loader->error()) {
    LogFormat(_T("Language: could not load resource '%s'"), language.resource);
    delete mo_loader;
    mo_loader = NULL;
    return false;
  }

  LogFormat(_T("Loaded translations from resource '%s'"), language.resource);

  mo_file = &mo_loader->get();
  return true;
}

static bool
ReadResourceLanguageFile(const TCHAR *resource)
{
  auto language = FindLanguage(resource);
  return language != nullptr && ReadBuiltinLanguage(*language);
}

#else /* !HAVE_BUILTIN_LANGUAGES */

#ifndef HAVE_NATIVE_GETTEXT

static inline const char *
DetectLanguage()
{
  return NULL;
}

static inline bool
ReadBuiltinLanguage(char dummy)
{
  return false;
}

static bool
ReadResourceLanguageFile(const TCHAR *resource)
{
  return false;
}

#endif /* HAVE_NATIVE_GETTEXT */

#endif /* !HAVE_BUILTIN_LANGUAGES */

#ifndef HAVE_NATIVE_GETTEXT

static void
AutoDetectLanguage()
{
  // Try to detect the language by calling the OS's corresponding functions
  const auto l = DetectLanguage();
  if (l != nullptr)
    // If a language was detected -> try to load the MO file
    ReadBuiltinLanguage(*l);
}

static bool
LoadLanguageFile(Path path)
{
  LogFormat(_T("Language: loading file '%s'"), path.c_str());

  delete mo_loader;
  mo_loader = new MOLoader(path);
  if (mo_loader->error()) {
    LogFormat(_T("Language: could not load file '%s'"), path.c_str());
    delete mo_loader;
    mo_loader = NULL;
    return false;
  }

  LogFormat(_T("Loaded translations from file '%s'"), path.c_str());

  mo_file = &mo_loader->get();
  return true;
}

#endif /* !HAVE_NATIVE_GETTEXT */

void
InitLanguage()
{
#ifdef HAVE_NATIVE_GETTEXT

  const char *const domain = "xcsoar";

  /* we want to get UTF-8 strings from gettext() */
  bind_textdomain_codeset(domain, "utf8");

  // Set the current locale to the environment's default
  setlocale(LC_ALL, "");
  // always use a dot as decimal point in printf/scanf()
  setlocale(LC_NUMERIC, "C");
  bindtextdomain(domain, "/usr/share/locale");
  textdomain(domain);

#endif
}

/**
 * Reads the selected LanguageFile into the cache
 */
void
ReadLanguageFile()
{
#ifndef HAVE_NATIVE_GETTEXT
  CloseLanguageFile();

  LogFormat("Loading language file");

  auto value = Profile::GetPath(ProfileKeys::LanguageFile);

  if (value == nullptr || value.IsEmpty() || value == Path(_T("auto"))) {
    AutoDetectLanguage();
    return;
  }

  if (value == Path(_T("none")))
    return;

  Path base = value.GetBase();
  if (base == nullptr)
    base = value;

  if (base == value) {
    value = LocalPath(value);

    /* need to refresh "base" because the allocated string returned by
       Profile::Path() has just been freed */
    base = value.GetBase();
    assert(base != nullptr);
  }

  if (!LoadLanguageFile(value) && !ReadResourceLanguageFile(base.c_str()))
    AutoDetectLanguage();
#endif
}

void
CloseLanguageFile()
{
#ifndef HAVE_NATIVE_GETTEXT
  mo_file = NULL;
  reset_gettext_cache();
  delete mo_loader;
  mo_loader = NULL;
#endif
}
