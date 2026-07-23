// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightConfigPanel.hpp"

#ifdef HAVE_HTTP

#include "DataGlobals.hpp"
#include "Form/DataField/Enum.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "Weather/SkySight/Regions.hpp"
#include "Weather/SkySight/SkySightManager.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  SKYSIGHT_EMAIL,
  SKYSIGHT_PASSWORD,
  SKYSIGHT_REGION,
};

class SkySightConfigPanel final : public RowFormWidget {
public:
  SkySightConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
SkySightConfigPanel::Prepare(ContainerWindow &parent,
                             const PixelRect &rc) noexcept
{
  const auto &settings = CommonInterface::GetComputerSettings().weather;

  RowFormWidget::Prepare(parent, rc);

  AddText(_("SkySight Email"),
          _("The e-mail address you use to sign in to skysight.io."),
          settings.skysight.email);
  AddPassword(_("SkySight Password"),
              _("Your SkySight password."),
              settings.skysight.password);

  auto *region = AddEnum(_("SkySight Region"),
                         _("Select the SkySight region used for live weather layers."));
  if (region == nullptr)
    return;

  auto &df = *(DataFieldEnum *)region->GetDataField();
  if (const auto skysight = DataGlobals::GetSkySight(); skysight != nullptr) {
    for (const auto &candidate : skysight->GetRegions())
      df.addEnumText(candidate.id.c_str(), gettext(candidate.name.c_str()));

    if (!df.SetValue(settings.skysight.region.c_str()))
      df.SetValue(skysight->GetRegion().data());
  } else {
    for (const auto &candidate : SKYSIGHT_REGIONS)
      df.addEnumText(candidate.id, gettext(candidate.name));

    df.SetValue(FindSkySightRegionById(settings.skysight.region.c_str()).id);
  }

  region->RefreshDisplay();
}

bool
SkySightConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;
  auto &settings = CommonInterface::SetComputerSettings().weather;

  changed |= SaveValue(SKYSIGHT_EMAIL, ProfileKeys::SkySightEmail,
                       settings.skysight.email);
  changed |= SaveValue(SKYSIGHT_PASSWORD, ProfileKeys::SkySightPassword,
                       settings.skysight.password);
  changed |= SaveValue(SKYSIGHT_REGION, ProfileKeys::SkySightRegion,
                       settings.skysight.region);

  if (changed)
    if (auto skysight = DataGlobals::GetSkySight())
      skysight->Init();

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateSkySightConfigPanel()
{
  return std::make_unique<SkySightConfigPanel>();
}

#else

std::unique_ptr<Widget>
CreateSkySightConfigPanel()
{
  return {};
}

#endif
