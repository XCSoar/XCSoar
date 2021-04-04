/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "Dialogs/ListPicker.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Weather/Rasp/Providers.hpp"
#include "Form/Edit.hpp"
#include "Form/List.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Protection.hpp"
#include "DataGlobals.hpp"
#include "UIGlobals.hpp"
#include "UIState.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "net/http/Init.hpp"
#include "net/http/ToFile.hpp"
#include "io/FileTransaction.hpp"

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
  void OnModified(DataField &df) override {
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
  df.Set(state.map);
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
      time_df.Set(time.GetMinuteOfDay());
    GetControl(TIME).RefreshDisplay();
  }
}

inline void
RASPSettingsPanel::OnTimeModified(const DataFieldEnum &df) noexcept
{
  const int value = df.GetValue();
  time = value >= 0
    ? BrokenTime::FromMinuteOfDay(value)
    : BrokenTime::Invalid();
}

class RaspProviderRenderer : public ListItemRenderer {
  TextRowRenderer row_renderer;

public:
  unsigned CalculateLayout(const DialogLook &look) noexcept {
    return row_renderer.CalculateLayout(*look.list.font);
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned i) noexcept override {
    row_renderer.DrawTextRow(canvas, rc, rasp_providers[i].name);
  }
};

void
RASPSettingsPanel::Download() noexcept
{
  unsigned n = 0;
  for (auto i = rasp_providers; i->url != nullptr; ++i)
    ++n;

  assert(n > 0);

  RaspProviderRenderer renderer;
  int i = ListPicker(_("Download"), n, 0, renderer.CalculateLayout(GetLook()),
                     renderer);
  if (i < 0)
    return;

  const char *url = rasp_providers[i].url;
  auto path = LocalPath(_T(RASP_FILENAME));

  try {
    DialogJobRunner runner(UIGlobals::GetMainWindow(),
                           GetLook(),
                           _("Download"), true);

    FileTransaction transaction(path);
    Net::DownloadToFileJob job(*Net::curl, url, transaction.GetTemporaryPath());
    if (!runner.Run(job))
      return;

    transaction.Commit();
  } catch (...) {
    ShowError(std::current_exception(), _("Download"));
    return;
  }

  rasp = std::make_shared<RaspStore>(std::move(path));
  rasp->ScanAll();

  DataGlobals::SetRasp(std::shared_ptr<RaspStore>(rasp));
  FillItemControl();
}

void
RASPSettingsPanel::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
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
RASPSettingsPanel::Save(bool &_changed) noexcept
{
  WeatherUIState &state = CommonInterface::SetUIState().weather;

  state.map = GetValueInteger(ITEM);
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
