/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "system/Path.hpp"
#include "LogFile.hpp"
#include "Profile/Profile.hpp"
#include "util/ScopeExit.hxx"
#include "util/StringCompare.hxx"
#include "util/StringAPI.hxx"

#ifdef HAVE_NATIVE_GETTEXT
#include <locale.h>
#endif

#ifdef ANDROID
#include "java/Global.hxx"
#include "java/Class.hxx"
#include "java/Object.hxx"
#include "java/String.hxx"
#endif

#ifdef _WIN32
#include <winnls.h>
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

#ifndef _WIN32
/**
 * Several fake WIN32 constants.  These are not used on Android, but
 * we need them or we have to have a separate version of
 * #language_table on Android.
 */
enum {
  LANG_NULL,
  LANG_BULGARIAN,
  LANG_CATALAN,
  LANG_CHINESE,
  LANG_CHINESE_TRADITIONAL,
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
  LANG_TELUGU,
  LANG_TURKISH,
  LANG_UKRAINIAN,
  LANG_VIETNAMESE,
};
#endif

extern "C"
{
  extern const uint8_t bg_mo[];
  extern const size_t bg_mo_size;
  extern const uint8_t ca_mo[];
  extern const size_t ca_mo_size;
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
  extern const uint8_t te_mo[];
  extern const size_t te_mo_size;
  extern const uint8_t tr_mo[];
  extern const size_t tr_mo_size;
  extern const uint8_t uk_mo[];
  extern const size_t uk_mo_size;
  extern const uint8_t vi_mo[];
  extern const size_t vi_mo_size;
  extern const uint8_t zh_CN_mo[];
  extern const size_t zh_CN_mo_size;
  extern const uint8_t zh_Hant_mo[];
  extern const size_t zh_Hant_mo_size;
}

#define L(number, code_name, display_name) { number, code_name ## _mo, code_name ## _mo_size, _T( #code_name ".mo"), _T(display_name) }

const BuiltinLanguage language_table[] = {
  L(LANG_BULGARIAN, bg, "Bulgarian"),
  L(LANG_CATALAN, ca, "Catalan"),
  L(LANG_CHINESE, zh_CN, "Simplified Chinese"),
  L(LANG_CHINESE_TRADITIONAL, zh_Hant, "Traditional Chinese"),
  L(LANG_CZECH, cs, "Czech"),
  L(LANG_DANISH, da, "Danish"),
  L(LANG_GERMAN, de, "German"),
  L(LANG_GREEK, el, "Greek"),
  L(LANG_SPANISH, es, "Spanish"),
  L(LANG_FINNISH, fi, "Finnish"),
  L(LANG_FRENCH, fr, "French"),
  L(LANG_HEBREW, he, "Hebrew"),
  L(LANG_CROATIAN, hr, "Croatian"),
  L(LANG_HUNGARIAN, hu, "Hungarian"),
  L(LANG_ITALIAN, it, "Italian"),
  L(LANG_JAPANESE, ja, "Japanese"),
  L(LANG_KOREAN, ko, "Korean"),
  L(LANG_LITHUANIAN, lt, "Lithuanian"),
  L(LANG_NORWEGIAN, nb, "Norwegian"),
  L(LANG_DUTCH, nl, "Dutch"),
  L(LANG_POLISH, pl, "Polish"),
  L(LANG_PORTUGUESE, pt_BR, "Brazilian Portuguese"),

  /* our Portuguese translation is less advanced than Brazilian
     Portuguese */
  L(LANG_PORTUGUESE, pt, "Portuguese"),

  L(LANG_ROMANIAN, ro, "Romanian"),
  L(LANG_RUSSIAN, ru, "Russian"),
  L(LANG_SLOVAK, sk, "Slovak"),
  L(LANG_SLOVENIAN, sl, "Slovenian"),
  L(LANG_SERBIAN, sr, "Serbian"),
  L(LANG_SWEDISH, sv, "Swedish"),
  L(LANG_TELUGU, te, "Telugu"),
  L(LANG_TURKISH, tr, "Turkish"),
  L(LANG_UKRAINIAN, uk, "Ukranian"),
  L(LANG_VIETNAMESE, vi, "Vietnamese"),

  {},
};

#ifdef _WIN32

[[gnu::pure]]
static const BuiltinLanguage *
FindLanguage(WORD language) noexcept
{
  // Search for supported languages matching the language code
  for (unsigned i = 0; language_table[i].resource != nullptr; ++i)
    if (language_table[i].language == language)
      // .. and return the MO file name if found
      return &language_table[i];

  return nullptr;
}

#endif

[[gnu::pure]]
static const BuiltinLanguage *
FindLanguage(const TCHAR *resource) noexcept
{
  assert(resource != nullptr);

  // Search for supported languages matching the MO file name
  for (unsigned i = 0; language_table[i].resource != nullptr; ++i)
    if (StringIsEqual(language_table[i].resource, resource))
      // .. and return the language code
      return &language_table[i];

  return nullptr;
}

static const BuiltinLanguage *
DetectLanguage() noexcept
{
#ifdef ANDROID

  JNIEnv *env = Java::GetEnv();

  Java::Class cls(env, "java/util/Locale");

  // Call static function Locale.getDefault() that
  // returns the user's default Locale object

  jmethodID cid = env->GetStaticMethodID(cls, "getDefault",
                                         "()Ljava/util/Locale;");
  assert(cid != nullptr);

  Java::LocalObject obj(env, env->CallStaticObjectMethod(cls, cid));
  if (!obj)
    return nullptr;

  // Call function Locale.getLanguage() that
  // returns a two-letter language string

  cid = env->GetMethodID(cls, "getLanguage", "()Ljava/lang/String;");
  assert(cid != nullptr);

  Java::String language{env, (jstring)env->CallObjectMethod(obj, cid)};
  if (language == nullptr)
    return nullptr;

  // Convert the jstring to a char string
  const auto language2 = language.GetUTFChars();

  /* generate the resource name */

  const char *language3 = language2.c_str();
  if (strcmp(language3, "pt") == 0)
    /* hack */
    language3 = "pt_BR";

  // Attach .mo to the language identifier
  static char language_buffer[16];
  snprintf(language_buffer, sizeof(language_buffer), "%s.mo", language3);

  // Return e.g. "de.mo"
  return FindLanguage(language_buffer);

#elif defined(_WIN32)

  // Retrieve the default user language identifier from the OS
  LANGID lang_id = GetUserDefaultUILanguage();
  LogFormat("Language: GetUserDefaultUILanguage()=0x%x", (int)lang_id);
  if (lang_id == 0)
    return nullptr;

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

#endif // HAVE_BUILTIN_LANGUAGES

#ifdef HAVE_NATIVE_GETTEXT

static void
InitNativeGettext(const char *locale) noexcept
{
  const char *const domain = "xcsoar";

  /* we want to get UTF-8 strings from gettext() */
  bind_textdomain_codeset(domain, "utf8");

  setlocale(LC_ALL, locale);
  // always use a dot as decimal point in printf/scanf()
  setlocale(LC_NUMERIC, "C");
  bindtextdomain(domain, "/usr/share/locale");
  textdomain(domain);

}

#endif // HAVE_NATIVE_GETTEXT

#ifdef HAVE_BUILTIN_LANGUAGES

static bool
ReadBuiltinLanguage(const BuiltinLanguage &language) noexcept
{
  LogFormat(_T("Language: loading resource '%s'"), language.resource);

  // Load MO file from resource
  delete mo_loader;
  mo_loader = new MOLoader(language.begin, (size_t)language.size);
  if (mo_loader->error()) {
    LogFormat(_T("Language: could not load resource '%s'"), language.resource);
    delete mo_loader;
    mo_loader = nullptr;
    return false;
  }

  LogFormat(_T("Loaded translations from resource '%s'"), language.resource);

  mo_file = &mo_loader->get();
  return true;
}

static bool
ReadResourceLanguageFile(const TCHAR *resource) noexcept
{
  auto language = FindLanguage(resource);
  return language != nullptr && ReadBuiltinLanguage(*language);
}

#else /* !HAVE_BUILTIN_LANGUAGES */

#ifndef HAVE_NATIVE_GETTEXT

static inline const char *
DetectLanguage() noexcept
{
  return nullptr;
}

static inline bool
ReadBuiltinLanguage(char dummy) noexcept
{
  return false;
}

static bool
ReadResourceLanguageFile(const TCHAR *resource) noexcept
{
  return false;
}

#endif /* HAVE_NATIVE_GETTEXT */

#endif /* !HAVE_BUILTIN_LANGUAGES */

#ifndef HAVE_NATIVE_GETTEXT

static void
AutoDetectLanguage() noexcept
{
  // Try to detect the language by calling the OS's corresponding functions
  const auto l = DetectLanguage();
  if (l != nullptr)
    // If a language was detected -> try to load the MO file
    ReadBuiltinLanguage(*l);
}

static bool
LoadLanguageFile(Path path) noexcept
{
  LogFormat(_T("Language: loading file '%s'"), path.c_str());

  delete mo_loader;
  mo_loader = nullptr;

  try {
    mo_loader = new MOLoader(path);
    if (mo_loader->error()) {
      LogFormat(_T("Language: could not load file '%s'"), path.c_str());
      delete mo_loader;
      mo_loader = nullptr;
      return false;
    }
  } catch (...) {
    LogError(std::current_exception(), "Language: could not load file");
    return false;
  }

  LogFormat(_T("Loaded translations from file '%s'"), path.c_str());

  mo_file = &mo_loader->get();
  return true;
}

#endif /* !HAVE_NATIVE_GETTEXT */

void
InitLanguage() noexcept
{
#ifdef HAVE_NATIVE_GETTEXT
  // Set the current locale to the environment's default
  InitNativeGettext("");
#endif
}

/**
 * Reads the selected LanguageFile into the cache
 */
void
ReadLanguageFile() noexcept
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
CloseLanguageFile() noexcept
{
#ifndef HAVE_NATIVE_GETTEXT
  mo_file = nullptr;
  reset_gettext_cache();
  delete mo_loader;
  mo_loader = nullptr;
#endif
}
