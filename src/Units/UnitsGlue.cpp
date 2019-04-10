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

#include "Units/UnitsGlue.hpp"
#include "Units/UnitsStore.hpp"
#include "LogFile.hpp"
#include "Util/StringAPI.hxx"

#include <tchar.h>

#ifndef HAVE_POSIX
#include <windows.h>
#endif

#ifdef ANDROID
#include "Java/Global.hxx"
#include "Java/Class.hxx"
#include "Java/Object.hxx"
#endif

struct language_unit_map {
  unsigned region_id;
  const TCHAR* region_code;
  unsigned store_index;
};

#ifdef ANDROID
/**
 * Several fake WIN32 constants. These are not used on Android, but
 * we need them or we have to have a separate version of
 * #language_table on Android.
 */
enum {
  LANG_ENGLISH,
};
enum {
  SUBLANG_ENGLISH_UK,
  SUBLANG_ENGLISH_US,
  SUBLANG_ENGLISH_AUS,
};

#define MAKELANGID(x,y) 0

#endif

#if !defined(HAVE_POSIX) || defined(ANDROID)

const struct language_unit_map language_table[] = {
  { MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK), _T("en_UK"), 1 },
  { MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), _T("en_US"), 2 },
  { MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_AUS), _T("en_AU"), 3 },
  { 0, nullptr, 0 }
};

#endif

#ifndef HAVE_POSIX
static unsigned
FindLanguage(LANGID lang)
{
  // Search for supported languages matching the language code
  for (unsigned i = 0; language_table[i].region_code != nullptr; ++i)
    if (language_table[i].region_id == lang)
      return language_table[i].store_index;

  return 0;
}
#elif defined(ANDROID)
static unsigned
FindLanguage(const TCHAR* lang)
{
  // Search for supported languages matching the language code
  for (unsigned i = 0; language_table[i].region_code != nullptr; ++i)
    if (StringIsEqual(language_table[i].region_code, lang))
      return language_table[i].store_index;

  return 0;
}
#endif

static unsigned
AutoDetect()
{
#ifndef HAVE_POSIX

  // Retrieve the default user language identifier from the OS
  LANGID lang_id = GetUserDefaultUILanguage();
  LogFormat("Units: GetUserDefaultUILanguage() = 0x%x", (int)lang_id);
  if (lang_id == 0)
    return 0;

  return FindLanguage(lang_id);

#elif defined(ANDROID)
  JNIEnv *env = Java::GetEnv();

  Java::Class cls(env, "java/util/Locale");

  // Call static function Locale.getDefault() that
  // returns the user's default Locale object

  jmethodID cid = env->GetStaticMethodID(cls, "getDefault",
                                         "()Ljava/util/Locale;");
  assert(cid != nullptr);

  Java::LocalObject obj(env, env->CallStaticObjectMethod(cls, cid));
  if (!obj)
    return 0;

  // Call function Locale.getLanguage() that
  // returns a two-letter language string

  jstring language = Java::Object::toString(env, obj);
  if (language == nullptr)
    return 0;

  // Convert the jstring to a char string
  const char *language2 = env->GetStringUTFChars(language, nullptr);
  if (language2 == nullptr) {
    env->DeleteLocalRef(language);
    return 0;
  }

  unsigned id = FindLanguage(language2);

  // Clean up the memory
  env->ReleaseStringUTFChars(language, language2);
  env->DeleteLocalRef(language);

  // Return e.g. "de.mo"
  return id;

#else
  // Metric default on Linux
  return 0;
#endif
}

const UnitSetting &
Units::LoadFromOSLanguage()
{
  unsigned index = AutoDetect();
  return Units::Store::Read(index);
}
