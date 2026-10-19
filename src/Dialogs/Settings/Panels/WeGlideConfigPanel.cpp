// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeGlideConfigPanel.hpp"
#include "net/client/WeGlide/Settings.hpp"
#include "Profile/Keys.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"

#ifdef HAVE_HTTP
#include "Dialogs/Settings/WeGlideDeviceSignIn.hpp"
#endif

#include <stdio.h>

// #define HAVE_WEGLIDE_PILOTNAME

enum ControlIndex {
  WeGlideEnabled,
  WeGlideAutomaticUpload,
  WeGlidePilotID,
  WeGlidePilotBirthDate,
#ifdef HAVE_HTTP
  WeGlideConnect,
#endif
};


class WeGlideConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  WeGlideConfigPanel() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void SetEnabled(bool enabled) noexcept;

#ifdef HAVE_HTTP
  Button *account_button = nullptr;

  void UpdateAccountButton() noexcept;
#endif

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
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
#ifdef HAVE_HTTP
  UpdateAccountButton();
#endif
}

#ifdef HAVE_HTTP
void
WeGlideConfigPanel::UpdateAccountButton() noexcept
{
  if (account_button == nullptr)
    return;

  const bool connected = WeGlideAccountConnected();
  account_button->SetCaption(connected
                              ? C_("Button", "Disconnect")
                              : C_("Button", "Connect"));
  SetRowEnabled(WeGlideConnect,
                connected || GetValueBoolean(WeGlideEnabled));
}
#endif

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

#ifdef HAVE_HTTP
  account_button = AddButton(C_("Button", "Connect"), [this]() {
    if (WeGlideAccountConnected())
      ForgetWeGlideAccount();
    else if (GetValueBoolean(WeGlideEnabled))
      RunWeGlideDeviceSignIn();
    UpdateAccountButton();
  });
#endif

  SetEnabled(weglide.enabled);
}

void
WeGlideConfigPanel::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);
#ifdef HAVE_HTTP
  UpdateAccountButton();
#endif
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
