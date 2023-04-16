// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RASPDialog.hpp"
#include "Dialogs/DownloadFilePicker.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Weather/Rasp/RaspStore.hpp"
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
  : public RowFormWidget, DataFieldListener {

  enum Controls {
    ITEM,
    TIME,
    DOWNLOAD,
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
  void OnTimeModified(const DataFieldEnum &df) noexcept;
  void Download() noexcept;

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override {
    if (IsDataField(ITEM, df))
      UpdateTimeControl();
    else if (IsDataField(TIME, df))
      OnTimeModified((const DataFieldEnum &)df);
  }
};

void
RASPSettingsPanel::FillItemControl() noexcept
{
  auto &df = (DataFieldEnum &)GetDataField(ITEM);

  df.ClearChoices();
  df.AddChoice(-1, _T("none"), _T("none"), nullptr);
  for (unsigned i = 0; i < rasp->GetItemCount(); i++) {
    const auto &mi = rasp->GetItemInfo(i);
    const TCHAR *label = mi.label;
    if (label != nullptr)
      label = gettext(label);

    const TCHAR *help = mi.help;
    if (help != nullptr)
      help = gettext(help);

    df.AddChoice(i, mi.name, label, help);
  }

  const WeatherUIState &state = CommonInterface::GetUIState().weather;
  df.SetValue(state.map);
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
        TCHAR timetext[10];
        _stprintf(timetext, _T("%02u:%02u"), t.hour, t.minute);
        time_df.addEnumText(timetext, t.GetMinuteOfDay());
      });

    if (time.IsPlausible())
      time_df.SetValue(time.GetMinuteOfDay());
    GetControl(TIME).RefreshDisplay();
  }
}

inline void
RASPSettingsPanel::OnTimeModified(const DataFieldEnum &df) noexcept
{
  const int value = df.GetValue();
  time = value > 0
    ? BrokenTime::FromMinuteOfDay(value)
    : BrokenTime::Invalid();
}

void
RASPSettingsPanel::Download() noexcept
{
  auto path = DownloadFilePicker(FileType::RASP);
  if (path == nullptr)
    return;

  rasp = std::make_shared<RaspStore>(std::move(path));
  rasp->ScanAll();

  DataGlobals::SetRasp(std::shared_ptr<RaspStore>(rasp));
  FillItemControl();
}

void
RASPSettingsPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  const WeatherUIState &state = CommonInterface::GetUIState().weather;
  time = state.time;

  WndProperty *wp;

  wp = AddEnum(_("Field"), nullptr, this);
  wp->GetDataField()->EnableItemHelp(true);
  FillItemControl();

  wp->RefreshDisplay();

  AddEnum(_("Time"), nullptr, this);
  UpdateTimeControl();

  AddButton(_("Download"), [this](){ Download(); });
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
