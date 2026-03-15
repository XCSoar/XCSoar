// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RASPDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Weather/Rasp/Configured.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "DataGlobals.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"

class RASPSettingsPanel final
  : public RowFormWidget {

  enum Controls {
    FILE,
  };

  std::shared_ptr<RaspStore> rasp;

public:
  explicit RASPSettingsPanel(std::shared_ptr<RaspStore> &&_rasp) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()),
     rasp(std::move(_rasp)) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
RASPSettingsPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  WndProperty *wp = AddFile(_("File"), nullptr,
                            ProfileKeys::RaspFile, "*-rasp*.dat\0",
                            FileType::RASP);
  wp->GetDataField()->SetOnModified([this]{
    if (SaveValueFileReader(FILE, ProfileKeys::RaspFile)) {
      rasp = LoadConfiguredRasp(false);
      DataGlobals::SetRasp(rasp);
      Profile::Save();
    }
  });
}

bool
RASPSettingsPanel::Save([[maybe_unused]] bool &_changed) noexcept
{
  return true;
}

std::unique_ptr<Widget>
CreateRaspWidget() noexcept
{
  auto rasp = DataGlobals::GetRasp();
  return std::make_unique<RASPSettingsPanel>(std::move(rasp));
}
