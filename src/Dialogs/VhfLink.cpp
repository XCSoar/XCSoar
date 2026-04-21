// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VhfLink.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "RadioFrequency.hpp"
#include "util/StaticString.hxx"
#include "util/StringCompare.hxx"

#include <fmt/format.h>

#include <cstring>
#include <string>
#include <string_view>

/**
 * Success and invalid-link feedback use ShowMessageBox so the user sees
 * immediate feedback even when a modal (checklist/help) is open.
 */
bool
HandleVhfLink(const char *url, const char *station_name)
{
  if (url == nullptr || !StringStartsWith(url, "vhf:"))
    return false;

  const char *hash = std::strchr(url, '#');
  if (hash == nullptr) {
    ShowMessageBox(
      _("Add #standby or #active to the link, for example vhf:122.800#standby"),
      _("VHF link"),
      MB_OK | MB_ICONWARNING);
    return true;
  }

  const std::string_view mhz_part(url + 4, std::size_t(hash - (url + 4)));
  RadioFrequency rf = RadioFrequency::Parse(mhz_part);
  if (!rf.IsDefined()) {
    ShowMessageBox(_("This frequency is not valid for VHF airband."),
                   _("VHF link"),
                   MB_OK | MB_ICONWARNING);
    return true;
  }

  const char *frag = hash + 1;
  const std::string_view frag_sv(frag, std::strlen(frag));
  const bool standby = StringIsEqualIgnoreCase(frag_sv, "standby");
  if (!standby && !StringIsEqualIgnoreCase(frag_sv, "active")) {
    ShowMessageBox(
      _("Use #standby or #active after the frequency (example: vhf:122.800#standby)."),
      _("VHF link"),
      MB_OK | MB_ICONWARNING);
    return true;
  }

  const char *name =
    (station_name != nullptr && station_name[0] != '\0') ? station_name
                                                         : nullptr;

  if (standby)
    ActionInterface::SetStandbyFrequency(rf, name);
  else
    ActionInterface::SetActiveFrequency(rf, name);

  char freqbuf[32];
  rf.Format(freqbuf, sizeof(freqbuf));

  /* Same short labels as the Active / Standby radio InfoBoxes (Factory.cpp). */
  const char *const mode_label =
    gettext(standby ? N_("Stby Freq") : N_("Act Freq"));

  try {
    const std::string text =
      name != nullptr
        ? fmt::format(fmt::runtime(_("{0}: {1} MHz — {2}")),
                      mode_label, freqbuf, name)
        : fmt::format(fmt::runtime(_("{0}: {1} MHz")), mode_label, freqbuf);

    ShowMessageBox(text.c_str(), _("Radio"), MB_OK | MB_ICONINFORMATION);
  } catch (...) {
    /* Rare: allocation failure or bad translated format string */
    StaticString<160> fallback;
    fallback.Format("%s: %s MHz", mode_label, freqbuf);
    ShowMessageBox(fallback.c_str(), _("Radio"), MB_OK | MB_ICONINFORMATION);
  }

  return true;
}
