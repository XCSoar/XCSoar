// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
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
  const auto &settings = CommonInterface::GetComputerSettings().weather;

  RowFormWidget::Prepare(parent, rc);

#ifdef HAVE_PCMET
  AddText(_T("pc_met Username"), _T(""),
          settings.pcmet.www_credentials.username);
  AddPassword(_T("pc_met Password"), _T(""),
              settings.pcmet.www_credentials.password);

#if 0
  // code disabled because DWD has terminated our access */
  AddText(_T("pc_met FTP Username"), _T(""),
          settings.pcmet.ftp_credentials.username);
  AddPassword(_T("pc_met FTP Password"), _T(""),
              settings.pcmet.ftp_credentials.password);
#endif
#endif

#ifdef HAVE_HTTP
  AddBoolean(_T("Thermal Information Map"),
             _("Show thermal locations downloaded from Thermal Information Map (thermalmap.info)."),
             settings.enable_tim);
#endif
}

bool
WeatherConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  auto &settings = CommonInterface::SetComputerSettings().weather;

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
#endif

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateWeatherConfigPanel()
{
  return std::make_unique<WeatherConfigPanel>();
}
