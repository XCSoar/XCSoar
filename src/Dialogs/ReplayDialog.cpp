/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "UIGlobals.hpp"
#include "Units/Units.hpp"
#include "Components.hpp"
#include "Replay/Replay.hpp"
#include "Form/DataField/FileReader.hpp"
#include "Form/DataField/Float.hpp"
#include "Language/Language.hpp"

static WndForm *wf = NULL;

static void
OnStopClicked()
{
  replay->Stop();
}

static void
OnStartClicked()
{
  const TCHAR *path = GetFormValueFile(*wf, _T("prpFile"));
  if (!replay->Start(path))
    ShowMessageBox(_("Could not open IGC file!"),
                   _("Replay"), MB_OK | MB_ICONINFORMATION);
}

static void
OnRateData(DataField *Sender, DataField::DataAccessMode Mode)
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

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnStopClicked),
  DeclareCallBackEntry(OnStartClicked),
  DeclareCallBackEntry(OnRateData),
  DeclareCallBackEntry(NULL)
};

void
ShowReplayDialog()
{
  wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  _T("IDR_XML_LOGGERREPLAY"));
  if (!wf)
    return;

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(_T("*.nmea"));
    dfe->ScanDirectoryTop(_T("*.igc"));
    dfe->Lookup(replay->GetFilename());
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpRate"), replay->GetTimeScale());

  wf->ShowModal();

  delete wf;
}
