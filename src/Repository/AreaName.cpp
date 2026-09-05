// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AreaName.hpp"
#include "Language/Language.hpp"
#include "util/StringAPI.hxx"

namespace {

struct AreaNameEntry {
  const char *code, *name;
};

/**
 * ISO 3166 alpha-2 codes (the way xcsoar-data-content names its
 * files), plus a few spellings older repositories used.  Sorted by
 * code for the reader's sake; the lookup is linear anyway.
 */
constexpr AreaNameEntry area_names[] = {
  { "ar", N_("Argentina") },
  { "at", N_("Austria") },
  { "au", N_("Australia") },
  { "be", N_("Belgium") },
  { "bg", N_("Bulgaria") },
  { "br", N_("Brazil") },
  { "ca", N_("Canada") },
  { "ch", N_("Switzerland") },
  { "cl", N_("Chile") },
  { "co", N_("Colombia") },
  { "cz", N_("Czechia") },
  { "de", N_("Germany") },
  { "deu", N_("Germany") },
  { "dk", N_("Denmark") },
  { "ee", N_("Estonia") },
  { "es", N_("Spain") },
  { "fi", N_("Finland") },
  { "fr", N_("France") },
  { "gb", N_("United Kingdom") },
  { "gr", N_("Greece") },
  { "hr", N_("Croatia") },
  { "hu", N_("Hungary") },
  { "ie", N_("Ireland") },
  { "il", N_("Israel") },
  { "in", N_("India") },
  { "is", N_("Iceland") },
  { "it", N_("Italy") },
  { "jp", N_("Japan") },
  { "ke", N_("Kenya") },
  { "lt", N_("Lithuania") },
  { "lu", N_("Luxembourg") },
  { "lv", N_("Latvia") },
  { "ma", N_("Morocco") },
  { "mx", N_("Mexico") },
  { "na", N_("Namibia") },
  { "nl", N_("Netherlands") },
  { "no", N_("Norway") },
  { "nz", N_("New Zealand") },
  { "pl", N_("Poland") },
  { "pt", N_("Portugal") },
  { "ro", N_("Romania") },
  { "rs", N_("Serbia") },
  { "ru", N_("Russia") },
  { "se", N_("Sweden") },
  { "si", N_("Slovenia") },
  { "sk", N_("Slovakia") },
  { "tr", N_("Türkiye") },
  { "ua", N_("Ukraine") },
  { "uk", N_("United Kingdom") },
  { "us", N_("United States") },
  { "usa", N_("United States") },
  { "uy", N_("Uruguay") },
  { "za", N_("South Africa") },
};

} // namespace

const char *
GetAreaDisplayName(const char *area) noexcept
{
  if (area == nullptr || *area == '\0')
    return nullptr;

  for (const auto &i : area_names)
    if (StringIsEqualIgnoreCase(area, i.code))
      return i.name;

  return nullptr;
}
