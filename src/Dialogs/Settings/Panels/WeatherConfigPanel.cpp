/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "WeatherConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Settings.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Util/NumberParser.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "DataGlobals.hpp"
#include "Weather/Skysight/Skysight.hpp"

enum ControlIndex {
#ifdef HAVE_PCMET
  PCMET_USER,
  PCMET_PASSWORD,
  PCMET_FTP_USER,
  PCMET_FTP_PASSWORD,
#endif
#ifdef HAVE_SKYSIGHT
  SKYSIGHT_EMAIL,
  SKYSIGHT_PASSWORD,
  SKYSIGHT_REGION
#endif
};

class WeatherConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  WeatherConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

  private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override {};
};

static void FillRegionControl(WndProperty &wp, const TCHAR *setting)
{
  DataFieldEnum *df = (DataFieldEnum *)wp.GetDataField();
  auto skysight = DataGlobals::GetSkysight();

  for(auto &i : skysight->GetRegions())
    df->addEnumText(i.first.c_str(), i.second.c_str());

  // if old region doesn't exist any more this will fall back to first element
  df->Set(setting);
  wp.RefreshDisplay();
}
void
WeatherConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const auto &settings = CommonInterface::GetComputerSettings().weather;
  WndProperty *wp;
  RowFormWidget::Prepare(parent, rc);

  AddText(_T("pc_met Username"), _T(""),
          settings.pcmet.www_credentials.username);
  AddPassword(_T("pc_met Password"), _T(""),
              settings.pcmet.www_credentials.password);

  AddText(_T("pc_met FTP Username"), _T(""),
          settings.pcmet.ftp_credentials.username);
  AddPassword(_T("pc_met FTP Password"), _T(""),
              settings.pcmet.ftp_credentials.password);

  AddText(_T("Skysight Email"), _T("The e-mail you use to log in to the skysight.io site."),
          settings.skysight.email);
  AddPassword(_T("Skysight Password"), _T("Your Skysight password."),
              settings.skysight.password);  
  wp = AddEnum(_T("Skysight Region"), _T("The Skysight region to load data for."), this);
  FillRegionControl(*wp, settings.skysight.region);

}

bool
WeatherConfigPanel::Save(bool &_changed)
{
  bool changed = false;

  auto &settings = CommonInterface::SetComputerSettings().weather;

#ifdef HAVE_PCMET
  changed |= SaveValue(PCMET_USER, ProfileKeys::PCMetUsername,
                       settings.pcmet.www_credentials.username);

  changed |= SaveValue(PCMET_PASSWORD, ProfileKeys::PCMetPassword,
                       settings.pcmet.www_credentials.password);

  changed |= SaveValue(PCMET_FTP_USER, ProfileKeys::PCMetFtpUsername,
                       settings.pcmet.ftp_credentials.username);

  changed |= SaveValue(PCMET_FTP_PASSWORD, ProfileKeys::PCMetFtpPassword,
                       settings.pcmet.ftp_credentials.password);
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

Widget *
CreateWeatherConfigPanel()
{
  return new WeatherConfigPanel();
}
