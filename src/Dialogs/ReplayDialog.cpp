/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Dialogs/Message.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/ActionListener.hpp"
#include "Form/DataField/Listener.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "Replay/Replay.hpp"
#include "Form/DataField/FileReader.hpp"
#include "Form/DataField/Float.hpp"
#include "Language/Language.hpp"

enum Buttons {
  START,
  STOP,
};

class ReplayControlWidget final
  : public RowFormWidget, ActionListener, DataFieldListener {
  enum Controls {
    FILE,
    RATE,
  };

public:
  ReplayControlWidget(const DialogLook &look)
    :RowFormWidget(look) {}

  void CreateButtons(WidgetDialog &dialog) {
    dialog.AddButton(_("Start"), *this, START);
    dialog.AddButton(_("Stop"), *this, STOP);
  }

private:
  void OnStopClicked();
  void OnStartClicked();

public:
  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;

private:
  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;

  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;
};

void
ReplayControlWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  auto *file =
    AddFileReader(_("File"),
                  _("Name of file to replay.  Can be an IGC file (.igc), a raw NMEA log file (.nmea), or if blank, runs the demo."),
                  nullptr,
                  _T("*.nmea\0*.igc\0"),
                  true);
  ((DataFieldFileReader *)file->GetDataField())->Lookup(replay->GetFilename());
  file->RefreshDisplay();

  AddFloat(_("Rate"),
           _("Time acceleration of replay. Set to 0 for pause, 1 for normal real-time replay."),
           _("%.0f x"), _T("%.0f"),
           fixed(0), fixed(10), fixed(1), false,
           replay->GetTimeScale(),
           this);
}

inline void
ReplayControlWidget::OnStopClicked()
{
  replay->Stop();
}

inline void
ReplayControlWidget::OnStartClicked()
{
  const DataFieldFileReader &df = (const DataFieldFileReader &)
    GetDataField(FILE);
  const TCHAR *path = df.GetPathFile();
  if (!replay->Start(path))
    ShowMessageBox(_("Could not open IGC file!"),
                   _("Replay"), MB_OK | MB_ICONINFORMATION);
}

void
ReplayControlWidget::OnAction(int id)
{
  switch (id) {
  case START:
    OnStartClicked();
    break;

  case STOP:
    OnStopClicked();
    break;
  }
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
  WidgetDialog dialog(look);
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _("Replay"), widget);
  widget->CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrOK);

  dialog.ShowModal();
}
