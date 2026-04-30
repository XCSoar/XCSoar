// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "Weather/Features.hpp"
#include "Widget/RowFormWidget.hpp"
#include "net/http/Features.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "util/NumberParser.hpp"

enum ControlIndex {
#ifdef HAVE_PCMET
  PCMET_USER,
  PCMET_PASSWORD,
#if 0
  PCMET_FTP_USER,
  PCMET_FTP_PASSWORD,
#endif
#endif

#ifdef HAVE_HTTP
  ENABLE_TIM,
  XCTHERM_ENABLED,
  XCTHERM_SHOW_ON_MAIN_MAP,
  XCTHERM_AUTO_SWITCH,
  XCTHERM_EMAIL,
  XCTHERM_PASSWORD,
#endif
};

class WeatherConfigPanel final
  : public RowFormWidget {
public:
  WeatherConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  /* methods from Widget */
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

#ifdef HAVE_PCMET
  AddText("pc_met Username", "",
          settings.pcmet.www_credentials.username);
  AddPassword("pc_met Password", "",
              settings.pcmet.www_credentials.password);

#if 0
  // code disabled because DWD has terminated our access */
  AddText("pc_met FTP Username", "",
          settings.pcmet.ftp_credentials.username);
  AddPassword("pc_met FTP Password", "",
              settings.pcmet.ftp_credentials.password);
#endif
#endif

#ifdef HAVE_HTTP
  AddBoolean("Thermal Information Map",
             _("Show thermal locations downloaded from Thermal Information Map (thermalmap.info)."),
             settings.enable_tim);

  AddBoolean("XCTherm enabled",
             "Enable XCTherm wave forecast download and overlay.",
             settings.xctherm.enabled);

  AddBoolean("XCTherm overlay on map",
             "Show XCTherm forecast overlay on normal map pages.",
             settings.xctherm.show_on_main_map);

  AddBoolean("XCTherm Auto Layer/Time",
             "Automatically switch altitude layer based on GPS altitude and forecast time based on UTC clock.",
             settings.xctherm.auto_switch);

  AddText("XCTherm email",
          "Email address for your XCTherm account.",
          settings.xctherm.credentials.email);

  AddPassword("XCTherm password",
              "Password for your XCTherm account.",
              settings.xctherm.credentials.password);
#endif
}

bool
WeatherConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

#if defined(HAVE_PCMET) || defined(HAVE_HTTP)
  auto &settings = CommonInterface::SetComputerSettings().weather;
#endif

#ifdef HAVE_PCMET
  changed |= SaveValue(PCMET_USER, ProfileKeys::PCMetUsername,
                       settings.pcmet.www_credentials.username);

  changed |= SaveValue(PCMET_PASSWORD, ProfileKeys::PCMetPassword,
                       settings.pcmet.www_credentials.password);

#if 0
  // code disabled because DWD has terminated our access */
  changed |= SaveValue(PCMET_FTP_USER, ProfileKeys::PCMetFtpUsername,
                       settings.pcmet.ftp_credentials.username);

  changed |= SaveValue(PCMET_FTP_PASSWORD, ProfileKeys::PCMetFtpPassword,
                       settings.pcmet.ftp_credentials.password);
#endif
#endif

#ifdef HAVE_HTTP
  changed |= SaveValue(ENABLE_TIM, ProfileKeys::EnableThermalInformationMap,
                       settings.enable_tim);

  changed |= SaveValue(XCTHERM_ENABLED, ProfileKeys::XCThermEnabled,
                       settings.xctherm.enabled);

  changed |= SaveValue(XCTHERM_SHOW_ON_MAIN_MAP,
                       ProfileKeys::XCThermShowOnMainMap,
                       settings.xctherm.show_on_main_map);

  changed |= SaveValue(XCTHERM_AUTO_SWITCH,
                       ProfileKeys::XCThermAutoSwitch,
                       settings.xctherm.auto_switch);

  changed |= SaveValue(XCTHERM_EMAIL, ProfileKeys::XCThermEmail,
                       settings.xctherm.credentials.email);

  changed |= SaveValue(XCTHERM_PASSWORD, ProfileKeys::XCThermPassword,
                       settings.xctherm.credentials.password);
#endif

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateWeatherConfigPanel()
{
  return std::make_unique<WeatherConfigPanel>();
}
