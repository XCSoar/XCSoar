/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Dialogs/Dialogs.h"
#include "Dialogs/Internal.hpp"
#include "Units/Units.hpp"
#include "Components.hpp"
#include "Replay/Replay.hpp"
#include "DataField/FileReader.hpp"
#include "DataField/Float.hpp"
#include "MainWindow.hpp"

static WndForm *wf = NULL;

static void
OnStopClicked(gcc_unused WndButton &Sender)
{
  replay->Stop();
}

static void
OnStartClicked(gcc_unused WndButton &Sender)
{
  WndProperty* wp;
  wp = (WndProperty*)wf->FindByName(_T("prpFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    replay->SetFilename(dfe->GetPathFile());
  }
  replay->Start();
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void
OnRateData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  DataFieldFloat &df = *(DataFieldFloat *)Sender;

  switch (Mode) {
  case DataField::daChange:
    replay->SetTimeScale(df.GetAsFixed());
    break;

  case DataField::daSpecial:
    return;
  }
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnStopClicked),
  DeclareCallBackEntry(OnStartClicked),
  DeclareCallBackEntry(OnRateData),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgLoggerReplayShowModal(void)
{
  wf = LoadDialog(CallBackTable,
                      XCSoarInterface::main_window, _T("IDR_XML_LOGGERREPLAY"));
  if (!wf)
    return;

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpRate"));
  if (wp) {
    DataFieldFloat &df = *(DataFieldFloat *)wp->GetDataField();
    df.SetAsFloat(replay->GetTimeScale());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.nmea"));
    dfe->ScanDirectoryTop(_T("*.igc"));
    dfe->Lookup(replay->GetFilename());
    wp->RefreshDisplay();
  }

  wf->ShowModal();

  delete wf;
}
