// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Table.hpp"

#ifdef HAVE_BUILTIN_LANGUAGES

#include <cstdint>

extern "C"
{
  extern const std::byte bg_mo[];
  extern const size_t bg_mo_size;
  extern const std::byte ca_mo[];
  extern const size_t ca_mo_size;
  extern const std::byte cs_mo[];
  extern const size_t cs_mo_size;
  extern const std::byte da_mo[];
  extern const size_t da_mo_size;
  extern const std::byte de_mo[];
  extern const size_t de_mo_size;
  extern const std::byte el_mo[];
  extern const size_t el_mo_size;
  extern const std::byte es_mo[];
  extern const size_t es_mo_size;
  extern const std::byte fi_mo[];
  extern const size_t fi_mo_size;
  extern const std::byte fr_mo[];
  extern const size_t fr_mo_size;
  extern const std::byte he_mo[];
  extern const size_t he_mo_size;
  extern const std::byte hr_mo[];
  extern const size_t hr_mo_size;
  extern const std::byte hu_mo[];
  extern const size_t hu_mo_size;
  extern const std::byte it_mo[];
  extern const size_t it_mo_size;
  extern const std::byte ja_mo[];
  extern const size_t ja_mo_size;
  extern const std::byte ko_mo[];
  extern const size_t ko_mo_size;
  extern const std::byte lt_mo[];
  extern const size_t lt_mo_size;
  extern const std::byte nb_mo[];
  extern const size_t nb_mo_size;
  extern const std::byte nl_mo[];
  extern const size_t nl_mo_size;
  extern const std::byte pl_mo[];
  extern const size_t pl_mo_size;
  extern const std::byte pt_BR_mo[];
  extern const size_t pt_BR_mo_size;
  extern const std::byte pt_mo[];
  extern const size_t pt_mo_size;
  extern const std::byte ro_mo[];
  extern const size_t ro_mo_size;
  extern const std::byte ru_mo[];
  extern const size_t ru_mo_size;
  extern const std::byte sk_mo[];
  extern const size_t sk_mo_size;
  extern const std::byte sl_mo[];
  extern const size_t sl_mo_size;
  extern const std::byte sr_mo[];
  extern const size_t sr_mo_size;
  extern const std::byte sv_mo[];
  extern const size_t sv_mo_size;
  extern const std::byte te_mo[];
  extern const size_t te_mo_size;
  extern const std::byte tr_mo[];
  extern const size_t tr_mo_size;
  extern const std::byte uk_mo[];
  extern const size_t uk_mo_size;
  extern const std::byte vi_mo[];
  extern const size_t vi_mo_size;
  extern const std::byte zh_CN_mo[];
  extern const size_t zh_CN_mo_size;
  extern const std::byte zh_Hant_mo[];
  extern const size_t zh_Hant_mo_size;
}

#ifdef _WIN32
#define L(number, locale, code_name, display_name) { number, code_name ## _mo, code_name ## _mo_size,  #code_name ".mo", display_name }
#else
#define L(number, locale, code_name, display_name) { code_name ## _mo, code_name ## _mo_size,  #code_name ".mo", display_name }
#endif

#endif // HAVE_BUILTIN_LANGUAGES

#ifdef USE_LIBINTL
#define L(number, locale, code_name, display_name) { #locale ".UTF-8",  #code_name ".mo", display_name }
#endif // USE_LIBINTL

#ifdef _WIN32
#include <windef.h>
#include <winnt.h>
#endif

const BuiltinLanguage language_table[] = {
  L(LANG_BULGARIAN, bg_BG, bg, "Bulgarian"),
  L(LANG_CATALAN, ca_ES, ca, "Catalan"),
  L(LANG_CHINESE, zh_CN, zh_CN, "Simplified Chinese"),
  L(LANG_CHINESE_TRADITIONAL, zh_Hant, zh_Hant, "Traditional Chinese"),
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
  L(LANG_JAPANESE, ja_JP, ja, "Japanese"),
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
