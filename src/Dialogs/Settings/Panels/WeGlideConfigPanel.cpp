// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeGlideConfigPanel.hpp"
#include "net/client/WeGlide/Settings.hpp"
#include "Profile/Keys.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"

#include <stdio.h>

// #define HAVE_WEGLIDE_PILOTNAME

enum ControlIndex {
  WeGlideEnabled,
  WeGlideAutomaticUpload,
  WeGlidePilotID,
  WeGlidePilotBirthDate,
};


class WeGlideConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  WeGlideConfigPanel() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void SetEnabled(bool enabled) noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
WeGlideConfigPanel::SetEnabled(bool enabled) noexcept
{
  SetRowEnabled(WeGlideAutomaticUpload, enabled);
  SetRowEnabled(WeGlidePilotBirthDate, enabled);
  SetRowEnabled(WeGlidePilotID, enabled);
}

void
WeGlideConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(WeGlideEnabled, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetEnabled(dfb.GetValue());
  }
}

void
WeGlideConfigPanel::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  const WeGlideSettings &weglide = CommonInterface::GetComputerSettings().weglide;

  RowFormWidget::Prepare(parent, rc);

  AddBoolean(
      _("Enable"),
      _("Allow download of declared tasks from Weglide in the Task Manager."),
      weglide.enabled, this);

  AddBoolean(_("Automatic Upload"),
             _("Asks whether to upload flight to Weglide, after flight is "
               "downloaded from external logger."),
             weglide.automatic_upload, this);

  AddInteger(_("Pilot"),
             _("Take this from your WeGlide Profile. Or set to 0 if not used."),
             "%d", "%d", 1, 99999, 1, weglide.pilot_id);

  AddDate(_("Pilot date of birth"), nullptr,
          weglide.pilot_birthdate);

  SetEnabled(weglide.enabled);
}

bool
WeGlideConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  auto &weglide = CommonInterface::SetComputerSettings().weglide;

  changed |= SaveValue(WeGlideAutomaticUpload,
                       ProfileKeys::WeGlideAutomaticUpload,
                       weglide.automatic_upload);

  changed |= SaveValueInteger(WeGlidePilotID, ProfileKeys::WeGlidePilotID,
                              weglide.pilot_id);

  changed |= SaveValue(WeGlidePilotBirthDate,
                       ProfileKeys::WeGlidePilotBirthDate,
                       weglide.pilot_birthdate);

  changed |= SaveValue(WeGlideEnabled, ProfileKeys::WeGlideEnabled,
                       weglide.enabled);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateWeGlideConfigPanel() noexcept
{
  return std::make_unique<WeGlideConfigPanel>();
}
