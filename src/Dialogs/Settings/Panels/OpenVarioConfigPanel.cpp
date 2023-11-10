// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Profile/Keys.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"

#include <stdio.h>

// #define HAVE_WEGLIDE_PILOTNAME

class OpenVarioConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  OpenVarioConfigPanel() noexcept
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
OpenVarioConfigPanel::SetEnabled(bool enabled) noexcept
{
  /*/
  SetRowEnabled(WeGlideAutomaticUpload, enabled);
  SetRowEnabled(WeGlidePilotBirthDate, enabled);
  SetRowEnabled(WeGlidePilotID, enabled);
  /***/
}

void
OpenVarioConfigPanel::OnModified(DataField &df) noexcept
{
  /*/
  if (IsDataField(WeGlideEnabled, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetEnabled(dfb.GetValue());
  }
  /**/
}

void
OpenVarioConfigPanel::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  // const WeGlideSettings &weglide = CommonInterface::GetComputerSettings().weglide;

  RowFormWidget::Prepare(parent, rc);

  bool bTest = false;
  unsigned iTest = 0;

  // void AddReadOnly(label, help,text;
  auto version = _("3.2.20");
  AddReadOnly(_("OV-Firmware-Version"), _("Current firmware version of OpenVario"), version);
  AddBoolean(
      _("Boolean Test"),
      _("Boolean Test."),
      bTest, this);

   AddInteger(_("Integer Test"),
             _("Integer Test."),
             _T("%d"), _T("%d"), 1, 99999, 1, iTest);

  SetEnabled(bTest);
}

bool
OpenVarioConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

/*

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
  */

  return true;
}

std::unique_ptr<Widget>
CreateOpenVarioConfigPanel() noexcept
{
  return std::make_unique<OpenVarioConfigPanel>();
}
