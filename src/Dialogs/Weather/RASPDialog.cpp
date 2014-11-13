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
#include "Form/DataField/Listener.hpp"
#include "Components.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"

#include <stdio.h>

class RASPSettingsPanel final : public RowFormWidget, DataFieldListener {
  enum Controls {
    ITEM,
    TIME,
  };

  RasterWeather &rasp;

public:
  RASPSettingsPanel(RasterWeather &_rasp)
    :RowFormWidget(UIGlobals::GetDialogLook()), rasp(_rasp) {}

  void UpdateTimeControl() {
    const DataFieldEnum &item = (const DataFieldEnum &)GetDataField(ITEM);
    SetRowEnabled(TIME, item.GetValue() > 0);
  }

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

  /* virtual methods from DataFieldListener */
  void OnModified(DataField &) override {
    UpdateTimeControl();
  }
};

void
RASPSettingsPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  WndProperty *wp;

  wp = AddEnum(_("Field"), nullptr, this);
  DataFieldEnum *dfe = (DataFieldEnum *)wp->GetDataField();
  dfe->EnableItemHelp(true);
  for (unsigned i = 0; i < RasterWeather::MAX_WEATHER_MAP; i++) {
    const TCHAR *label = rasp.ItemLabel(i);
    if (label != nullptr) {
      label = gettext(label);
      const TCHAR *help = rasp.ItemHelp(i);
      if (help != nullptr)
        help = gettext(help);
      dfe->AddChoice(i, label, nullptr, help);
    }
  }

  dfe->Set(rasp.GetParameter());
  wp->RefreshDisplay();

  wp = AddEnum(_("Time"), nullptr);
  dfe = (DataFieldEnum *)wp->GetDataField();
  dfe->addEnumText(_("Now"));

  rasp.ForEachTime([dfe](BrokenTime t){
      TCHAR timetext[10];
      _stprintf(timetext, _T("%02u:%02u"), t.hour, t.minute);
      dfe->addEnumText(timetext, t.GetMinuteOfDay());
    });

  const BrokenTime t = rasp.GetTime();
  dfe->Set(t.IsPlausible() ? t.GetMinuteOfDay() : 0);
  wp->RefreshDisplay();

  UpdateTimeControl();
}

bool
RASPSettingsPanel::Save(bool &_changed)
{
  rasp.SetParameter(GetValueInteger(ITEM));

  unsigned t = GetValueInteger(TIME);
  rasp.SetTime(t == 0
               ? BrokenTime::Invalid()
               : BrokenTime::FromMinuteOfDay(t));
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
