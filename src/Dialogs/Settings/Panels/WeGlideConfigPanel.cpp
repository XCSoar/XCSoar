// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeGlideConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Dialogs/DateEntry.hpp"
#include "Dialogs/NumberEntry.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "net/client/WeGlide/Settings.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"

/** The WeGlide account: whether it is used, and who the pilot is. */
class WeGlideConfigPanel final : public ConfigListPanel {
  bool enabled, automatic_upload;
  unsigned pilot_id;
  BrokenDate pilot_birthdate;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
WeGlideConfigPanel::LoadSettings() noexcept
{
  const WeGlideSettings &weglide =
    CommonInterface::GetComputerSettings().weglide;

  enabled = weglide.enabled;
  automatic_upload = weglide.automatic_upload;
  pilot_id = weglide.pilot_id;
  pilot_birthdate = weglide.pilot_birthdate;
}

void
WeGlideConfigPanel::Fill() noexcept
{
  AddGroup();

  AddToggleItem(_("Enable"),
                _("Allow download of declared tasks from Weglide in the Task Manager."),
                enabled);

  AddToggleItem(_("Automatic Upload"),
                _("Asks whether to upload flight to Weglide, after flight is "
                  "downloaded from external logger."),
                automatic_upload, nullptr, !enabled);

  /* the pilot */
  AddGroup();

  StaticString<16> id;
  id.Format("%u", pilot_id);

  ItemOptions id_options{.chevron = true,
                         .help = _("Take this from your WeGlide Profile. Or set to 0 if not used."),
                         .disabled = !enabled};
  if (pilot_id != 0)
    id_options.value = id.c_str();
  else
    id_options.badge = C_("Badge", "none");

  AddItem(_("Pilot"), [this](){
    unsigned value = pilot_id;
    if (NumberEntryDialog(_("Pilot"), value, 5) && value != pilot_id) {
      pilot_id = value;
      Refresh();
    }
  }, id_options);

  char date[0x10];
  ItemOptions date_options{.chevron = true, .disabled = !enabled};
  if (pilot_birthdate.IsPlausible()) {
    FormatISO8601(date, pilot_birthdate);
    date_options.value = date;
  } else
    date_options.badge = C_("Badge", "none");

  AddItem(_("Pilot date of birth"), [this](){
    BrokenDate value = pilot_birthdate;
    if (DateEntryDialog(_("Pilot date of birth"), value) &&
        value.IsPlausible() && value != pilot_birthdate) {
      pilot_birthdate = value;
      Refresh();
    }
  }, date_options);
}

bool
WeGlideConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  auto &weglide = CommonInterface::SetComputerSettings().weglide;

  changed |= Profile::Update(ProfileKeys::WeGlideAutomaticUpload,
                             weglide.automatic_upload, automatic_upload);
  changed |= Profile::Update(ProfileKeys::WeGlidePilotID,
                             weglide.pilot_id, uint32_t(pilot_id));

  if (pilot_birthdate.IsPlausible() &&
      pilot_birthdate != weglide.pilot_birthdate) {
    weglide.pilot_birthdate = pilot_birthdate;

    char buffer[0x10];
    FormatISO8601(buffer, pilot_birthdate);
    Profile::Set(ProfileKeys::WeGlidePilotBirthDate, buffer);
    changed = true;
  }

  changed |= Profile::Update(ProfileKeys::WeGlideEnabled,
                             weglide.enabled, enabled);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateWeGlideConfigPanel() noexcept
{
  return std::make_unique<WeGlideConfigPanel>();
}
