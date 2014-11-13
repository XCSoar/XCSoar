/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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

#include "WeatherDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Enum.hpp"
#include "Components.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"

#include <stdio.h>

class RASPSettingsPanel final : public RowFormWidget {
  enum Controls {
    ITEM,
    TIME,
  };

  RasterWeather &rasp;

public:
  RASPSettingsPanel(RasterWeather &_rasp)
    :RowFormWidget(UIGlobals::GetDialogLook()), rasp(_rasp) {}

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;
};

void
RASPSettingsPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  WndProperty *wp;

  wp = AddEnum(_("Field"), nullptr);
  DataFieldEnum *dfe = (DataFieldEnum *)wp->GetDataField();
  dfe->EnableItemHelp(true);
  dfe->addEnumText(_("Terrain"));
  for (unsigned i = 1; i < RasterWeather::MAX_WEATHER_MAP; i++) {
    const TCHAR *label = rasp.ItemLabel(i);
    if (label != nullptr)
      dfe->AddChoice(i, label, nullptr, rasp.ItemHelp(i));
  }

  dfe->Set(rasp.GetParameter());
  wp->RefreshDisplay();

  wp = AddEnum(_("Time"), nullptr);
  dfe = (DataFieldEnum *)wp->GetDataField();
  dfe->addEnumText(_("Now"));

  rasp.ForEachTime([dfe](unsigned i){
      TCHAR timetext[10];
      const BrokenTime t = RasterWeather::IndexToTime(i);
      _stprintf(timetext, _T("%02u:%02u"), t.hour, t.minute);
      dfe->addEnumText(timetext, i);
    });

  dfe->Set(rasp.GetTime());
  wp->RefreshDisplay();
}

bool
RASPSettingsPanel::Save(bool &_changed)
{
  rasp.SetParameter(GetValueInteger(ITEM));
  rasp.SetTime(GetValueInteger(TIME));
  return true;
}

void
dlgWeatherShowModal()
{
  RASPSettingsPanel *widget = new RASPSettingsPanel(*rasp);

  WidgetDialog dialog(UIGlobals::GetDialogLook());
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _("Weather Forecast"), widget);
  dialog.AddButton(_("OK"), mrOK);
  dialog.ShowModal();
}

/*
  Todo:
  - time based search
  - Draw a legend on screen?
  - Auto-advance time index of forecast if before current time
*/
