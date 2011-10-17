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
#include "Profile/Profile.hpp"
#include "Screen/Layout.hpp"
#include "Gauge/LogoView.hpp"
#include "DataField/FileReader.hpp"
#include "LogFile.hpp"
#include "MainWindow.hpp"
#include "Asset.hpp"
#include "StringUtil.hpp"
#include "LocalPath.hpp"
#include "OS/PathName.hpp"
#include "Compiler.h"

static WndForm *wf = NULL;
static LogoView *logo;

static void
OnLogoPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  canvas.clear_white();
  logo->draw(canvas, Sender->get_client_rect());
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static void
OnQuitClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrCancel);
}

static void
SelectProfile(const TCHAR *path)
{
  if (string_is_empty(path))
    return;

  Profile::SetFiles(path);

  /* When a profile from a secondary data path is used, this path
     becomes the primary data path */
  TCHAR temp[MAX_PATH];
  SetPrimaryDataPath(DirName(path, temp));
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnLogoPaint),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnQuitClicked),
  DeclareCallBackEntry(NULL)
};

bool
dlgStartupShowModal()
{
  LogStartUp(_T("Startup dialog"));

  logo = new LogoView();

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ? _T("IDR_XML_STARTUP_L") :
                                      _T("IDR_XML_STARTUP"));
  assert(wf != NULL);

  WndProperty* wp = ((WndProperty *)wf->FindByName(_T("prpProfile")));
  assert(wp != NULL);

  DataFieldFileReader* dfe = (DataFieldFileReader*)wp->GetDataField();
  assert(dfe != NULL);

  dfe->SetNotNullable();
  dfe->ScanDirectoryTop(_T("*.prf"));
  dfe->Lookup(Profile::GetPath());
  wp->RefreshDisplay();

  if (dfe->GetNumFiles() <= 1) {
    SelectProfile(dfe->GetPathFile());

    delete wf;
    delete logo;
    return true;
  }

  if (wf->ShowModal() != mrOK) {
    delete wf;
    delete logo;
    return false;
  }

  SelectProfile(dfe->GetPathFile());

  delete wf;
  delete logo;
  return true;
}
