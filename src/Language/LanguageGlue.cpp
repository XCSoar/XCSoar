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
#include "util/StringCompare.hxx"
#include "util/StringUtil.hpp"
#include "util/AllocatedString.hxx"

#ifdef USE_LIBINTL

#include <cstdlib>
#include <fmt/format.h>
#include <locale.h>
#include <string_view>
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

static bool
SetLanguageEnvironment(const char *locale) noexcept
{
  if (locale == nullptr || *locale == '\0')
    return true;

  std::string_view locale_view{locale};

  if (const auto dot = locale_view.find('.'); dot != std::string_view::npos)
    locale_view = locale_view.substr(0, dot);

  char base[16]{};
  if (locale_view.empty() || locale_view.size() > sizeof(base) - 1)
    return false;

  CopyString(base, sizeof(base), locale_view);

  const auto country_separator = locale_view.find('_');
  if (country_separator == std::string_view::npos)
    return setenv("LANGUAGE", base, 1) == 0;

  const std::string_view language_view =
    locale_view.substr(0, country_separator);
  char language[8]{};
  if (language_view.empty() || language_view.size() > sizeof(language) - 1)
    return false;

  CopyString(language, sizeof(language), language_view);

  char lang_override[24]{};
  auto [end, size] = fmt::format_to_n(lang_override,
                                      sizeof(lang_override) - 1,
                                      "{}:{}", base, language);
  if (size >= sizeof(lang_override))
    return false;
  *end = '\0';

  return setenv("LANGUAGE", lang_override, 1) == 0;
}

static bool
RestoreLanguageEnvironment() noexcept
{
  static const AllocatedString initial_language = []() {
    const char *const value = getenv("LANGUAGE");
    return value != nullptr ? AllocatedString(value) : AllocatedString::Empty();
  }();

  if (initial_language.empty())
    return unsetenv("LANGUAGE") == 0;

  return setenv("LANGUAGE", initial_language.c_str(), 1) == 0;
}

static void
InitNativeGettext(const char *locale) noexcept
{
  const char *const domain = "xcsoar";

  /* we want to get UTF-8 strings from gettext() */
  bind_textdomain_codeset(domain, "utf8");

  if (locale != nullptr && *locale != '\0') {
    if (!SetLanguageEnvironment(locale))
      LogFmt("Language: failed to set LANGUAGE from '{}'", locale);
  } else if (!RestoreLanguageEnvironment()) {
    LogString("Language: failed to restore LANGUAGE");
  }

  bool locale_ok = setlocale(LC_ALL, locale) != nullptr;

  if (!locale_ok && locale != nullptr && *locale != '\0') {
    std::string_view locale_view{locale};
    if (locale_view.ends_with(".UTF-8")) {
      char locale_utf8[32]{};
      const size_t prefix = locale_view.size() - 6;
      auto [end, size] = fmt::format_to_n(locale_utf8,
                                          sizeof(locale_utf8) - 1,
                                          "{}.utf8",
                                          locale_view.substr(0, prefix));
      if (size < sizeof(locale_utf8)) {
        *end = '\0';
        locale_ok = setlocale(LC_ALL, locale_utf8) != nullptr;
      }
    }

    if (!locale_ok) {
      LogFmt("Language: failed to activate locale '{}'", locale);

      if (setlocale(LC_ALL, "") != nullptr)
        LogString("Language: using system locale fallback");
      else
        LogString("Language: failed to activate system locale fallback");
    }
  }

  // always use a dot as decimal point in printf/scanf()
  setlocale(LC_NUMERIC, "C");
  bindtextdomain(domain, "/usr/share/locale");
  textdomain(domain);

  /* trigger gettext's locale fallback initialization eagerly */
  [[maybe_unused]] const char *dummy = dcgettext(domain, "", LC_MESSAGES);

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
