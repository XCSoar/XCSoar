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
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "DataGlobals.hpp"
#include "Weather/Skysight/Skysight.hpp"

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

#ifdef HAVE_SKYSIGHT
  SPACER,
  SKYSIGHT_EMAIL,
  SKYSIGHT_PASSWORD,
  SKYSIGHT_REGION
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

static void
FillRegionControl(WndProperty &wp, const TCHAR *setting)
{
  DataFieldEnum *df = (DataFieldEnum *)wp.GetDataField();
  auto skysight = DataGlobals::GetSkysight();

  for (auto &i: skysight->GetRegions())
    df->addEnumText(i.first.c_str(), i.second.c_str());

  // if old region doesn't exist any more this will fall back to first element
  df->SetValue(setting);
  wp.RefreshDisplay();
}

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

#ifdef HAVE_SKYSIGHT
  AddSpacer();

  AddText(_T("Skysight Email"), _T("The e-mail you use to log in to the skysight.io site."),
          settings.skysight.email);
  AddPassword(_T("Skysight Password"), _T("Your Skysight password."),
              settings.skysight.password);  
  WndProperty *wp = AddEnum(_T("Skysight Region"), _T("The Skysight region to load data for."), (DataFieldListener*)nullptr);
  FillRegionControl(*wp, settings.skysight.region);
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

#ifdef HAVE_SKYSIGHT
  changed |= SaveValue(SKYSIGHT_EMAIL, ProfileKeys::SkysightEmail,
                       settings.skysight.email);

  changed |= SaveValue(SKYSIGHT_PASSWORD, ProfileKeys::SkysightPassword,
                       settings.skysight.password);

  changed |= SaveValue(SKYSIGHT_REGION, ProfileKeys::SkysightRegion,
                    settings.skysight.region);        
  DataGlobals::GetSkysight()->Init();         
#endif

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateWeatherConfigPanel()
{
  return std::make_unique<WeatherConfigPanel>();
}
