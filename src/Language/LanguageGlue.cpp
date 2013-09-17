/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "OS/PathName.hpp"
#include "LogFile.hpp"
#include "Profile/Profile.hpp"
#include "Util/StringUtil.hpp"

#ifdef HAVE_NATIVE_GETTEXT
#include <locale.h>
#endif

#ifdef _WIN32_WCE
#include "OS/DynamicLibrary.hpp"
#endif

#ifdef ANDROID
#include "Java/Global.hpp"
#include "Java/Class.hpp"
#include "Java/Object.hpp"
#endif

#ifdef WIN32
#include <windows.h>
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
  LANG_NORWEGIAN,
  LANG_DUTCH,
  LANG_POLISH,
  LANG_PORTUGUESE,
  LANG_ROMANIAN,
  LANG_RUSSIAN,
  LANG_SLOVAK,
  LANG_SERBIAN,
  LANG_SWEDISH,
  LANG_TURKISH,
  LANG_UKRAINIAN,
  LANG_VIETNAMESE,
};
#endif

extern const uint8_t cs_start[] asm("_binary_cs_mo_start");
extern const uint8_t cs_size[] asm("_binary_cs_mo_size");
extern const uint8_t da_start[] asm("_binary_da_mo_start");
extern const uint8_t da_size[] asm("_binary_da_mo_size");
extern const uint8_t de_start[] asm("_binary_de_mo_start");
extern const uint8_t de_size[] asm("_binary_de_mo_size");
extern const uint8_t el_start[] asm("_binary_el_mo_start");
extern const uint8_t el_size[] asm("_binary_el_mo_size");
extern const uint8_t es_start[] asm("_binary_es_mo_start");
extern const uint8_t es_size[] asm("_binary_es_mo_size");
extern const uint8_t fi_start[] asm("_binary_fi_mo_start");
extern const uint8_t fi_size[] asm("_binary_fi_mo_size");
extern const uint8_t fr_start[] asm("_binary_fr_mo_start");
extern const uint8_t fr_size[] asm("_binary_fr_mo_size");
extern const uint8_t he_start[] asm("_binary_he_mo_start");
extern const uint8_t he_size[] asm("_binary_he_mo_size");
extern const uint8_t hr_start[] asm("_binary_hr_mo_start");
extern const uint8_t hr_size[] asm("_binary_hr_mo_size");
extern const uint8_t hu_start[] asm("_binary_hu_mo_start");
extern const uint8_t hu_size[] asm("_binary_hu_mo_size");
extern const uint8_t it_start[] asm("_binary_it_mo_start");
extern const uint8_t it_size[] asm("_binary_it_mo_size");
extern const uint8_t ja_start[] asm("_binary_ja_mo_start");
extern const uint8_t ja_size[] asm("_binary_ja_mo_size");
extern const uint8_t ko_start[] asm("_binary_ko_mo_start");
extern const uint8_t ko_size[] asm("_binary_ko_mo_size");
extern const uint8_t nb_start[] asm("_binary_nb_mo_start");
extern const uint8_t nb_size[] asm("_binary_nb_mo_size");
extern const uint8_t nl_start[] asm("_binary_nl_mo_start");
extern const uint8_t nl_size[] asm("_binary_nl_mo_size");
extern const uint8_t pl_start[] asm("_binary_pl_mo_start");
extern const uint8_t pl_size[] asm("_binary_pl_mo_size");
extern const uint8_t pt_BR_start[] asm("_binary_pt_BR_mo_start");
extern const uint8_t pt_BR_size[] asm("_binary_pt_BR_mo_size");
extern const uint8_t pt_start[] asm("_binary_pt_mo_start");
extern const uint8_t pt_size[] asm("_binary_pt_mo_size");
extern const uint8_t ro_start[] asm("_binary_ro_mo_start");
extern const uint8_t ro_size[] asm("_binary_ro_mo_size");
extern const uint8_t ru_start[] asm("_binary_ru_mo_start");
extern const uint8_t ru_size[] asm("_binary_ru_mo_size");
extern const uint8_t sk_start[] asm("_binary_sk_mo_start");
extern const uint8_t sk_size[] asm("_binary_sk_mo_size");
extern const uint8_t sr_start[] asm("_binary_sr_mo_start");
extern const uint8_t sr_size[] asm("_binary_sr_mo_size");
extern const uint8_t sv_start[] asm("_binary_sv_mo_start");
extern const uint8_t sv_size[] asm("_binary_sv_mo_size");
extern const uint8_t tr_start[] asm("_binary_tr_mo_start");
extern const uint8_t tr_size[] asm("_binary_tr_mo_size");
extern const uint8_t uk_start[] asm("_binary_uk_mo_start");
extern const uint8_t uk_size[] asm("_binary_uk_mo_size");
extern const uint8_t vi_start[] asm("_binary_vi_mo_start");
extern const uint8_t vi_size[] asm("_binary_vi_mo_size");

#ifdef __clang__
/* gcc gives "redeclaration differs in 'constexpr'" */
constexpr
#endif
const BuiltinLanguage language_table[] = {
  { LANG_CZECH, cs_start, cs_size, _T("cs.mo"), _T("Czech") },
  { LANG_DANISH, da_start, da_size, _T("da.mo"), _T("Danish") },
  { LANG_GERMAN, de_start, de_size, _T("de.mo"), _T("German") },
  { LANG_GREEK, el_start, el_size, _T("el.mo"), _T("Greek") },
  { LANG_SPANISH, es_start, es_size, _T("es.mo"), _T("Spanish") },
  { LANG_FINNISH, fi_start, fi_size, _T("fi.mo"), _T("Finnish") },
  { LANG_FRENCH, fr_start, fr_size, _T("fr.mo"), _T("French") },
  { LANG_HEBREW, he_start, he_size, _T("he.mo"), _T("Hebrew") },
  { LANG_CROATIAN, hr_start, hr_size, _T("hr.mo"), _T("Croatian") },
  { LANG_HUNGARIAN, hu_start, hu_size, _T("hu.mo"), _T("Hungarian") },
  { LANG_ITALIAN, it_start, it_size, _T("it.mo"), _T("Italian") },
  { LANG_JAPANESE, ja_start, ja_size, _T("ja.mo"), _T("Japanese") },
  { LANG_KOREAN, ko_start, ko_size, _T("ko.mo"), _T("Korean") },
  { LANG_NORWEGIAN, nb_start, nb_size, _T("nb.mo"), _T("Norwegian") },
  { LANG_DUTCH, nl_start, nl_size, _T("nl.mo"), _T("Dutch") },
  { LANG_POLISH, pl_start, pl_size, _T("pl.mo"), _T("Polish") },
  { LANG_PORTUGUESE, pt_BR_start, pt_BR_size, _T("pt_BR.mo"), _T("Brazilian Portuguese") },

  /* our Portuguese translation is less advanced than Brazilian
     Portuguese */
  { LANG_PORTUGUESE, pt_start, pt_size, _T("pt.mo"), _T("Portuguese") },

  { LANG_ROMANIAN, ro_start, ro_size, _T("ro.mo"), _T("Romanian") },
  { LANG_RUSSIAN, ru_start, ru_size, _T("ru.mo"), _T("Russian") },
  { LANG_SLOVAK, sk_start, sk_size, _T("sk.mo"), _T("Slovak") },
  { LANG_SERBIAN, sr_start, sr_size, _T("sr.mo"), _T("Serbian") },
  { LANG_SWEDISH, sv_start, sv_size, _T("sv.mo"), _T("Swedish") },
  { LANG_TURKISH, tr_start, tr_size, _T("tr.mo"), _T("Turkish") },
  { LANG_UKRAINIAN, uk_start, uk_size, _T("uk.mo"), _T("Ukranian") },
  { LANG_VIETNAMESE, vi_start, vi_size, _T("vi.mo"), _T("Vietnamese") },
  { 0, NULL }
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

#if defined(_WIN32_WCE)
  /* the GetUserDefaultUILanguage() prototype is missing on
     mingw32ce, we have to look it up dynamically */
  DynamicLibrary coreloc_dll(_T("coredll"));
  if (!coreloc_dll.IsDefined()) {
    LogFormat("Language: coredll.dll not found");
    return NULL;
  }

  typedef LANGID WINAPI (*GetUserDefaultUILanguage_t)();
  GetUserDefaultUILanguage_t GetUserDefaultUILanguage =
    (GetUserDefaultUILanguage_t)
    coreloc_dll.Lookup(_T("GetUserDefaultUILanguage"));
  if (GetUserDefaultUILanguage == NULL) {
    LogFormat("Language: GetUserDefaultUILanguage() not available");
    return NULL;
  }
#endif

  // Retrieve the default user language identifier from the OS
  LANGID lang_id = GetUserDefaultUILanguage();
  LogFormat("Language: GetUserDefaultUILanguage()=0x%x", (int)lang_id);
  if (lang_id == 0)
    return NULL;

  // Try to convert the primary language part of the language identifier
  // to a MO file name in the language table
  return FindLanguage(PRIMARYLANGID(lang_id));

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

#endif /* !HAVE_BUILTIN_LANGUAGES */

static void
AutoDetectLanguage()
{
#ifdef HAVE_NATIVE_GETTEXT

  // Set the current locale to the environment's default
  setlocale(LC_ALL, "");
  // always use a dot as decimal point in printf/scanf()
  setlocale(LC_NUMERIC, "C");
  bindtextdomain("xcsoar", "/usr/share/locale");
  textdomain("xcsoar");

#else /* !HAVE_NATIVE_GETTEXT */

  // Try to detect the language by calling the OS's corresponding functions
  const auto l = DetectLanguage();
  if (l != nullptr)
    // If a language was detected -> try to load the MO file
    ReadBuiltinLanguage(*l);

#endif /* !HAVE_NATIVE_GETTEXT */
}

static bool
LoadLanguageFile(const TCHAR *path)
{
#ifdef HAVE_NATIVE_GETTEXT

  /* not supported on UNIX */
  return false;

#else /* !HAVE_NATIVE_GETTEXT */

  LogFormat(_T("Language: loading file '%s'"), path);

  delete mo_loader;
  mo_loader = new MOLoader(path);
  if (mo_loader->error()) {
    LogFormat(_T("Language: could not load file '%s'"), path);
    delete mo_loader;
    mo_loader = NULL;
    return false;
  }

  LogFormat(_T("Loaded translations from file '%s'"), path);

  mo_file = &mo_loader->get();
  return true;

#endif /* !HAVE_NATIVE_GETTEXT */
}

/**
 * Reads the selected LanguageFile into the cache
 */
void
ReadLanguageFile()
{
  CloseLanguageFile();

  LogFormat("Loading language file");

  TCHAR buffer[MAX_PATH], second_buffer[MAX_PATH];
  const TCHAR *value = Profile::GetPath(ProfileKeys::LanguageFile, buffer)
    ? buffer : _T("");

  if (StringIsEqual(value, _T("none")))
    return;

  if (StringIsEmpty(value) || StringIsEqual(value, _T("auto"))) {
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
#ifndef HAVE_NATIVE_GETTEXT
  mo_file = NULL;
  reset_gettext_cache();
  delete mo_loader;
  mo_loader = NULL;
#endif
}
