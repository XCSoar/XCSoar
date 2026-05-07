// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "DataGlobals.hpp"
#include "Form/DataField/Enum.hpp"
#include "Weather/Settings.hpp"
#include "Weather/Features.hpp"
#ifdef HAVE_HTTP
#include "Weather/Skysight/Regions.hpp"
#include "Weather/Skysight/Skysight.hpp"
#endif
#include "Widget/RowFormWidget.hpp"
#include "net/http/Features.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

#include <string_view>

enum ControlIndex {
#ifdef HAVE_HTTP
  ENABLE_TIM,
#endif

#ifdef HAVE_HTTP
  SKYSIGHT_SPACER,
  SKYSIGHT_EMAIL,
  SKYSIGHT_PASSWORD,
  SKYSIGHT_REGION,
#endif
};

class WeatherConfigPanel final
  : public RowFormWidget {
public:
  WeatherConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
WeatherConfigPanel::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
#if defined(HAVE_PCMET) || defined(HAVE_HTTP)
  const auto &settings = CommonInterface::GetComputerSettings().weather;
#endif

  RowFormWidget::Prepare(parent, rc);

#ifdef HAVE_HTTP
  AddBoolean(_("Thermal Information Map"),
             _("Show thermal locations downloaded from Thermal Information Map (thermalmap.info)."),
             settings.enable_tim);
#endif

#ifdef HAVE_HTTP
  AddSpacer();

  AddText(_("SkySight Email"),
          _("The e-mail address you use to sign in to skysight.io."),
          settings.skysight.email);
  AddPassword(_("SkySight Password"),
              _("Your SkySight password."),
              settings.skysight.password);

  auto *region = AddEnum(_("SkySight Region"),
                         _("Select the SkySight region used for live weather layers."));
  if (region != nullptr) {
    auto &df = *(DataFieldEnum *)region->GetDataField();
    const auto skysight = DataGlobals::GetSkysight();
    if (skysight != nullptr) {
      for (const auto &candidate : skysight->GetRegions())
        df.addEnumText(candidate.id.c_str(), candidate.name.c_str());

      if (!df.SetValue(settings.skysight.region.c_str()))
        df.SetValue(skysight->GetRegion().data());
    } else {
      for (const auto &candidate : skysight_regions)
        df.addEnumText(candidate.id, gettext(candidate.name));

      df.SetValue(FindSkysightRegionById(settings.skysight.region.c_str()).id);
    }

    region->RefreshDisplay();
  }
#endif
}

bool
WeatherConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

#if defined(HAVE_PCMET) || defined(HAVE_HTTP)
  auto &settings = CommonInterface::SetComputerSettings().weather;
#endif

#ifdef HAVE_HTTP
  changed |= SaveValue(ENABLE_TIM, ProfileKeys::EnableThermalInformationMap,
                       settings.enable_tim);
#endif

#ifdef HAVE_HTTP
  bool skysight_changed = false;

  skysight_changed |= SaveValue(SKYSIGHT_EMAIL, ProfileKeys::SkysightEmail,
                                settings.skysight.email);
  skysight_changed |= SaveValue(SKYSIGHT_PASSWORD,
                                ProfileKeys::SkysightPassword,
                                settings.skysight.password);

  skysight_changed |= SaveValue(SKYSIGHT_REGION, ProfileKeys::SkysightRegion,
                                settings.skysight.region);

  if (skysight_changed)
    if (auto skysight = DataGlobals::GetSkysight())
      skysight->Init();

  changed |= skysight_changed;
#endif

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateWeatherConfigPanel()
{
  return std::make_unique<WeatherConfigPanel>();
}
