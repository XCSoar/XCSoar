// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MacCreadySetup.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"
#include "UIGlobals.hpp"
#include "Widget/RowFormWidget.hpp"

class MacCreadySetupPanel : public RowFormWidget,
                            private DataFieldListener {
  enum Controls {
    AUTO_MC,
    STF_IAS,
  };

public:
  MacCreadySetupPanel() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

  void Show(const PixelRect &rc) noexcept override;

private:
  void OnModified(DataField &df) noexcept override;
};

void
MacCreadySetupPanel::Prepare(ContainerWindow &parent,
                             const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &computer = CommonInterface::GetComputerSettings();
  const UISettings &ui = CommonInterface::GetUISettings();

  AddBoolean(C_("Setting", "Auto"), nullptr,
             computer.task.auto_mc, this);
  /* IAS is an ICAO abbreviation and stays untranslated. */
  AddBoolean("IAS",
             _("Show the speed-to-fly as indicated airspeed."),
             ui.maccready_stf_ias, this);
}

void
MacCreadySetupPanel::Show(const PixelRect &rc) noexcept
{
  LoadValue(AUTO_MC,
            CommonInterface::GetComputerSettings().task.auto_mc);
  LoadValue(STF_IAS, CommonInterface::GetUISettings().maccready_stf_ias);
  RowFormWidget::Show(rc);
}

void
MacCreadySetupPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(AUTO_MC, df)) {
    TaskBehaviour &task = CommonInterface::SetComputerSettings().task;
    task.auto_mc = ((const DataFieldBoolean &)df).GetValue();
    Profile::Set(ProfileKeys::AutoMc, task.auto_mc);
    return;
  }

  if (IsDataField(STF_IAS, df)) {
    UISettings &ui = CommonInterface::SetUISettings();
    ui.maccready_stf_ias = ((const DataFieldBoolean &)df).GetValue();
    Profile::Set(ProfileKeys::MacCreadySTFIAS, ui.maccready_stf_ias);
  }
}

std::unique_ptr<Widget>
LoadMacCreadySetupPanel([[maybe_unused]] unsigned id)
{
  return std::make_unique<MacCreadySetupPanel>();
}
