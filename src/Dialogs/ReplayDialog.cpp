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

#include "ReplayDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Listener.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "Replay/Replay.hpp"
#include "Form/DataField/File.hpp"
#include "Form/DataField/Float.hpp"
#include "Language/Language.hpp"

class ReplayControlWidget final
  : public RowFormWidget, DataFieldListener {
  enum Controls {
    FILE,
    RATE,
  };

public:
  ReplayControlWidget(const DialogLook &look)
    :RowFormWidget(look) {}

  void CreateButtons(WidgetDialog &dialog) {
    dialog.AddButton(_("Start"), [this](){ OnStartClicked(); });
    dialog.AddButton(_("Stop"), [this](){ OnStopClicked(); });
    dialog.AddButton(_T("+10'"), [this](){ OnFastForwardClicked(); });
  }

private:
  void OnStopClicked();
  void OnStartClicked();
  void OnFastForwardClicked();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;
};

void
ReplayControlWidget::Prepare(ContainerWindow &parent,
                             const PixelRect &rc) noexcept
{
  auto *file =
    AddFile(_("File"),
            _("Name of file to replay.  Can be an IGC file (.igc), a raw NMEA log file (.nmea), or if blank, runs the demo."),
            nullptr,
            _T("*.nmea\0*.igc\0"),
            true);
  ((FileDataField *)file->GetDataField())->Lookup(Path(replay->GetFilename()));
  file->RefreshDisplay();

  AddFloat(_("Rate"),
           _("Time acceleration of replay. Set to 0 for pause, 1 for normal real-time replay."),
           _T("%.0f x"), _T("%.0f"),
           0, 10, 1, false, replay->GetTimeScale(), this);
}

inline void
ReplayControlWidget::OnStopClicked()
{
  replay->Stop();
}

inline void
ReplayControlWidget::OnStartClicked()
{
  const auto &df = (const FileDataField &)GetDataField(FILE);
  const Path path = df.GetPathFile();

  try {
    replay->Start(path);
  } catch (...) {
    ShowError(std::current_exception(), _("Replay"));
  }
}

inline void
ReplayControlWidget::OnFastForwardClicked()
{
  replay->FastForward(10 * 60);
}

void
ReplayControlWidget::OnModified(DataField &_df)
{
  const DataFieldFloat &df = (const DataFieldFloat &)_df;

  replay->SetTimeScale(df.GetAsFixed());
}

void
ShowReplayDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  ReplayControlWidget *widget = new ReplayControlWidget(look);
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      look, _("Replay"), widget);
  widget->CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);

  dialog.ShowModal();
}
