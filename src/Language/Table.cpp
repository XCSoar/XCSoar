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

#include "Table.hpp"

#ifdef HAVE_BUILTIN_LANGUAGES

#include <cstdint>

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

#ifdef _WIN32
#define L(number, locale, code_name, display_name) { number, code_name ## _mo, code_name ## _mo_size, _T( #code_name ".mo"), _T(display_name) }
#else
#define L(number, locale, code_name, display_name) { code_name ## _mo, code_name ## _mo_size, _T( #code_name ".mo"), _T(display_name) }
#endif

#endif // HAVE_BUILTIN_LANGUAGES

#ifdef USE_LIBINTL
#define L(number, locale, code_name, display_name) { #locale ".UTF-8", _T( #code_name ".mo"), _T(display_name) }
#endif // USE_LIBINTL

#ifdef _WIN32
#include <windef.h>
#include <winnt.h>
#endif

const BuiltinLanguage language_table[] = {
  L(LANG_BULGARIAN, bg_BG, bg, "Bulgarian"),
  L(LANG_CATALAN, ca_ES, ca, "Catalan"),
  L(LANG_CHINESE, zh_CN, zh_CN, "Simplified Chinese"),
  L(LANG_CHINESE_TRADITIONAL, zh_HK, zh_Hant, "Traditional Chinese"),
  L(LANG_CZECH, cs_CZ, cs, "Czech"),
  L(LANG_DANISH, da_DK, da, "Danish"),
  L(LANG_GERMAN, de_DE, de, "German"),
  L(LANG_GREEK, el_GR, el, "Greek"),
  L(LANG_SPANISH, es_ES, es, "Spanish"),
  L(LANG_FINNISH, fi_FI, fi, "Finnish"),
  L(LANG_FRENCH, fr_FR, fr, "French"),
  L(LANG_HEBREW, he_IL, he, "Hebrew"),
  L(LANG_CROATIAN, hr_HR, hr, "Croatian"),
  L(LANG_HUNGARIAN, hu_HU, hu, "Hungarian"),
  L(LANG_ITALIAN, it_IT, it, "Italian"),
  L(LANG_JAPANESE, ja_HP, ja, "Japanese"),
  L(LANG_KOREAN, ko_KR, ko, "Korean"),
  L(LANG_LITHUANIAN, lt_LT, lt, "Lithuanian"),
  L(LANG_NORWEGIAN, nb_NO, nb, "Norwegian"),
  L(LANG_DUTCH, nl_NL, nl, "Dutch"),
  L(LANG_POLISH, pl_PL, pl, "Polish"),
  L(LANG_PORTUGUESE, pt_BR, pt_BR, "Brazilian Portuguese"),

  /* our Portuguese translation is less advanced than Brazilian
     Portuguese */
  L(LANG_PORTUGUESE, pt_PT, pt, "Portuguese"),

  L(LANG_ROMANIAN, ro_RO, ro, "Romanian"),
  L(LANG_RUSSIAN, ru_RU, ru, "Russian"),
  L(LANG_SLOVAK, sk_SK, sk, "Slovak"),
  L(LANG_SLOVENIAN, sl_SI, sl, "Slovenian"),
  L(LANG_SERBIAN, sr_RS, sr, "Serbian"),
  L(LANG_SWEDISH, sv_SE, sv, "Swedish"),
  L(LANG_TELUGU, te_IN, te, "Telugu"),
  L(LANG_TURKISH, tr_TR, tr, "Turkish"),
  L(LANG_UKRAINIAN, uk_UA, uk, "Ukranian"),
  L(LANG_VIETNAMESE, vi_VN, vi, "Vietnamese"),

  {},
};
