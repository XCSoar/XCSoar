// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef IS_OPENVARIO
// don't use (and compile) this code outside an OpenVario project!

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
OpenVarioConfigPanel::SetEnabled([[maybe_unused]] bool enabled) noexcept
{
#ifdef OPENVARIO_CONFIG
  // out commented currently:
  SetRowEnabled(WeGlideAutomaticUpload, enabled);
  SetRowEnabled(WeGlidePilotBirthDate, enabled);
  SetRowEnabled(WeGlidePilotID, enabled);
#endif
}

void
OpenVarioConfigPanel::OnModified([[maybe_unused]] DataField &df) noexcept
{
#ifdef OPENVARIO_CONFIG
// out commented currently:
  if (IsDataField(WeGlideEnabled, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    SetEnabled(dfb.GetValue());
  }
#endif
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
OpenVarioConfigPanel::Save([[maybe_unused]] bool &_changed) noexcept
{
#ifdef OPENVARIO_CONFIG
  // out commented currently:
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
 
  #endif
  return true;
}

std::unique_ptr<Widget>
CreateOpenVarioConfigPanel() noexcept
{
  return std::make_unique<OpenVarioConfigPanel>();
}
#endif