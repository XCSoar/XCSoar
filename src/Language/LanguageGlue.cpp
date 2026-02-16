// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LanguageGlue.hpp"
#include "Language.hpp"
#include "Table.hpp"
#include "LocalPath.hpp"
#include "lib/fmt/PathFormatter.hpp"
#include "system/Path.hpp"
#include "LogFile.hpp"
#include "Profile/Profile.hpp"
#include "util/ScopeExit.hxx"
#include "util/StringCompare.hxx"
#include "util/StringAPI.hxx"

#ifdef USE_LIBINTL
#include <locale.h>
#endif

#ifdef ANDROID
#include "java/Global.hxx"
#include "java/Class.hxx"
#include "java/Object.hxx"
#include "java/String.hxx"
#endif

#include <cassert>

#ifdef _WIN32
#include <winnls.h>
#endif

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#endif

#include <windef.h> /* for MAX_PATH */

#ifdef HAVE_NLS

#ifndef USE_LIBINTL

#include "MOLoader.hpp"

static MOLoader *mo_loader;

#endif

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
FindLanguage(const char *resource) noexcept
{
  assert(resource != nullptr);

  // Search for supported languages matching the MO file name
  for (unsigned i = 0; language_table[i].resource != nullptr; ++i)
    if (StringIsEqual(language_table[i].resource, resource))
      // .. and return the language code
      return &language_table[i];

  return nullptr;
}

#ifdef HAVE_BUILTIN_LANGUAGES

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
  LogFormat("GetUserDefaultUILanguage() = 0x%x", (int)lang_id);
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

#ifdef USE_LIBINTL

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

#endif // USE_LIBINTL

static bool
ReadBuiltinLanguage(const BuiltinLanguage &language) noexcept
{
  LogFmt("Language: loading resource '{}'", language.resource);

#ifdef HAVE_BUILTIN_LANGUAGES
  // Load MO file from resource
  delete mo_loader;
  mo_loader = new MOLoader({language.begin, (size_t)language.size});
  if (mo_loader->error()) {
    LogFmt("Language: could not load resource '{}'", language.resource);
    delete mo_loader;
    mo_loader = nullptr;
    return false;
  }

  LogFmt("Loaded translations from resource '{}'", language.resource);

  mo_file = &mo_loader->get();
#else
  InitNativeGettext(language.locale);
#endif

  return true;
}

static bool
ReadResourceLanguageFile(const char *resource) noexcept
{
  auto language = FindLanguage(resource);
  return language != nullptr && ReadBuiltinLanguage(*language);
}

static void
AutoDetectLanguage() noexcept
{
#ifdef USE_LIBINTL
  // Set the current locale to the environment's default
  InitNativeGettext("");
#else
  // Try to detect the language by calling the OS's corresponding functions
  const auto l = DetectLanguage();
  if (l != nullptr)
    // If a language was detected -> try to load the MO file
    ReadBuiltinLanguage(*l);
#endif
}

static bool
LoadLanguageFile([[maybe_unused]] Path path) noexcept
{
#ifdef HAVE_BUILTIN_LANGUAGES
  LogFmt("Language: loading file '{}'", path);

  delete mo_loader;
  mo_loader = nullptr;

  try {
    mo_loader = new MOLoader(path);
    if (mo_loader->error()) {
      LogFmt("Language: could not load file '{}'", path);
      delete mo_loader;
      mo_loader = nullptr;
      return false;
    }
  } catch (...) {
    LogError(std::current_exception(), "Language: could not load file");
    return false;
  }

  LogFmt("Loaded translations from file '{}'", path);

  mo_file = &mo_loader->get();
  return true;
#else
  return false;
#endif
}

#endif // HAVE_NLS

void
InitLanguage() noexcept
{
#ifdef USE_LIBINTL
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
#ifdef HAVE_NLS
  CloseLanguageFile();

  LogString("Loading language file");

  auto value = Profile::GetPath(ProfileKeys::LanguageFile);

  if (value == nullptr || value.empty() || value == Path("auto")) {
    AutoDetectLanguage();
    return;
  }

  if (value == Path("none")) {
#ifdef USE_LIBINTL
    InitNativeGettext("C");
#endif
    return;
  }

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

#endif // HAVE_NLS
}

void
CloseLanguageFile() noexcept
{
#ifndef USE_LIBINTL
  mo_file = nullptr;
  reset_gettext_cache();
  delete mo_loader;
  mo_loader = nullptr;
#endif
}
