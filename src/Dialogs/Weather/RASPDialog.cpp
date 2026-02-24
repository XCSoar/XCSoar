// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RASPDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Weather/Rasp/Configured.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "ui/control/List.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "DataGlobals.hpp"
#include "UIGlobals.hpp"
#include "UIState.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"

#include <stdio.h>

class RASPSettingsPanel final
  : public RowFormWidget {

  enum Controls {
    FILE,
    ITEM,
    TIME,
  };

  std::shared_ptr<RaspStore> rasp;

  BrokenTime time;

public:
  explicit RASPSettingsPanel(std::shared_ptr<RaspStore> &&_rasp) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()),
     rasp(std::move(_rasp)) {}

private:
  void FillItemControl() noexcept;
  void UpdateTimeControl() noexcept;

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
RASPSettingsPanel::FillItemControl() noexcept
{
  auto &df = (DataFieldEnum &)GetDataField(ITEM);

  df.ClearChoices();
  df.AddChoice(-1, "none", "none", nullptr);
  for (unsigned i = 0; i < rasp->GetItemCount(); i++) {
    const auto &mi = rasp->GetItemInfo(i);
    const char *label = mi.label;
    if (label != nullptr)
      label = gettext(label);

    const char *help = mi.help;
    if (help != nullptr)
      help = gettext(help);

    df.AddChoice(i, mi.name, label, help);
  }

  const WeatherUIState &state = CommonInterface::GetUIState().weather;
  df.SetValue(state.map);

  GetControl(ITEM).RefreshDisplay();
}

void
RASPSettingsPanel::UpdateTimeControl() noexcept
{
  const DataFieldEnum &item = (const DataFieldEnum &)GetDataField(ITEM);

  const int item_index = item.GetValue();
  SetRowEnabled(TIME, item_index >= 0);

  if (item_index >= 0) {
    DataFieldEnum &time_df = (DataFieldEnum &)GetDataField(TIME);
    time_df.ClearChoices();
    time_df.addEnumText(_("Now"));

    rasp->ForEachTime(item_index, [&time_df](BrokenTime t){
        char timetext[10];
        sprintf(timetext, "%02u:%02u", t.hour, t.minute);
        time_df.addEnumText(timetext, t.GetMinuteOfDay());
      });

    if (time.IsPlausible())
      time_df.SetValue(time.GetMinuteOfDay());
    GetControl(TIME).RefreshDisplay();
  }
}

void
RASPSettingsPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  const WeatherUIState &state = CommonInterface::GetUIState().weather;
  time = state.time;

  WndProperty *wp;

  wp = AddFile(_("File"), nullptr,
               ProfileKeys::RaspFile, "*-rasp*.dat\0",
               FileType::RASP);
  wp->GetDataField()->SetOnModified([this]{
    if (SaveValueFileReader(FILE, ProfileKeys::RaspFile)) {
      rasp = LoadConfiguredRasp(false);
      DataGlobals::SetRasp(rasp);
      FillItemControl();
      UpdateTimeControl();
      Profile::Save();
    }
  });

  wp = AddEnum(_("Field"), nullptr);
  wp->GetDataField()->SetOnModified([this]{
      UpdateTimeControl();
  });
  wp->GetDataField()->EnableItemHelp(true);
  FillItemControl();

  wp = AddEnum(_("Time"), nullptr);
  // Initialize TIME field with at least "Now" to prevent assertion failures
  // when GetValue() is called before UpdateTimeControl() completes
  {
    DataFieldEnum &time_df = (DataFieldEnum &)GetDataField(TIME);
    time_df.ClearChoices();
    time_df.addEnumText(_("Now"));
  }
  wp->GetDataField()->SetOnModified([this]{
    const unsigned value = GetValueEnum(TIME);
    time = value > 0
      ? BrokenTime::FromMinuteOfDay(value)
      : BrokenTime::Invalid();
  });
  UpdateTimeControl();
}

bool
RASPSettingsPanel::Save([[maybe_unused]] bool &_changed) noexcept
{
  WeatherUIState &state = CommonInterface::SetUIState().weather;

  state.map = GetValueEnum(ITEM);
  state.time = time;

  ActionInterface::SendUIState(true);

  return true;
}

std::unique_ptr<Widget>
CreateRaspWidget() noexcept
{
  auto rasp = DataGlobals::GetRasp();
  return std::make_unique<RASPSettingsPanel>(std::move(rasp));
}

/*
  Todo:
  - time based search
  - Draw a legend on screen?
  - Auto-advance time index of forecast if before current time
*/
