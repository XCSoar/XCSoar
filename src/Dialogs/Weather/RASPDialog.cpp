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

#include "RASPDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Weather/Rasp/RaspCache.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Components.hpp"
#include "UIGlobals.hpp"
#include "UIState.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"

#include <stdio.h>

class RASPSettingsPanel final : public RowFormWidget, DataFieldListener {
  enum Controls {
    ITEM,
    TIME,
  };

  RasterWeatherStore &rasp;

  BrokenTime time;

public:
  RASPSettingsPanel(RasterWeatherStore &_rasp)
    :RowFormWidget(UIGlobals::GetDialogLook()), rasp(_rasp) {}

private:
  void UpdateTimeControl();
  void OnTimeModified(const DataFieldEnum &df);

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) override {
    if (IsDataField(ITEM, df))
      UpdateTimeControl();
    else if (IsDataField(TIME, df))
      OnTimeModified((const DataFieldEnum &)df);
  }
};

void
RASPSettingsPanel::UpdateTimeControl()
{
  const DataFieldEnum &item = (const DataFieldEnum &)GetDataField(ITEM);

  const unsigned item_index = item.GetValue();
  SetRowEnabled(TIME, item_index > 0);

  if (item_index > 0) {
    DataFieldEnum &time_df = (DataFieldEnum &)GetDataField(TIME);
    time_df.ClearChoices();
    time_df.addEnumText(_("Now"));

    rasp.ForEachTime(item_index, [&time_df](BrokenTime t){
        TCHAR timetext[10];
        _stprintf(timetext, _T("%02u:%02u"), t.hour, t.minute);
        time_df.addEnumText(timetext, t.GetMinuteOfDay());
      });

    if (time.IsPlausible())
      time_df.Set(time.GetMinuteOfDay());
    GetControl(TIME).RefreshDisplay();
  }
}

inline void
RASPSettingsPanel::OnTimeModified(const DataFieldEnum &df)
{
  const unsigned value = df.GetValue();
  time = value == 0
    ? BrokenTime::Invalid()
    : BrokenTime::FromMinuteOfDay(value);
}

void
RASPSettingsPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const WeatherUIState &state = CommonInterface::GetUIState().weather;
  time = state.time;

  WndProperty *wp;

  wp = AddEnum(_("Field"), nullptr, this);
  DataFieldEnum *dfe = (DataFieldEnum *)wp->GetDataField();
  dfe->EnableItemHelp(true);
  for (unsigned i = 0; i < rasp.GetItemCount(); i++) {
    const auto &mi = rasp.GetItemInfo(i);
    const TCHAR *label = mi.label;
    if (label != nullptr)
      label = gettext(label);

    const TCHAR *help = mi.help;
    if (help != nullptr)
      help = gettext(help);

    dfe->AddChoice(i, mi.name, label, help);
  }

  dfe->Set(state.map);
  wp->RefreshDisplay();

  AddEnum(_("Time"), nullptr, this);
  UpdateTimeControl();
}

bool
RASPSettingsPanel::Save(bool &_changed)
{
  WeatherUIState &state = CommonInterface::SetUIState().weather;

  state.map = GetValueInteger(ITEM);
  state.time = time;

  ActionInterface::SendUIState(true);

  return true;
}

Widget *
CreateRaspWidget()
{
  if (rasp == nullptr || rasp->GetItemCount() == 0)
    return new LargeTextWidget(UIGlobals::GetDialogLook(),
                               _T("No RASP data"));

  return new RASPSettingsPanel(*rasp);
}

/*
  Todo:
  - time based search
  - Draw a legend on screen?
  - Auto-advance time index of forecast if before current time
*/
