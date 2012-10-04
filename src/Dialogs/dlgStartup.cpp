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

#include "Dialogs/Dialogs.h"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "UIGlobals.hpp"
#include "Profile/Profile.hpp"
#include "Screen/Layout.hpp"
#include "Form/Form.hpp"
#include "Form/Draw.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Enum.hpp"
#include "Gauge/LogoView.hpp"
#include "Form/DataField/FileReader.hpp"
#include "LogFile.hpp"
#include "Asset.hpp"
#include "Util/StringUtil.hpp"
#include "LocalPath.hpp"
#include "OS/PathName.hpp"
#include "OS/FileUtil.hpp"
#include "Compiler.h"

#include <windef.h> /* for MAX_PATH */

class WndButton;

static WndForm *wf = NULL;
static LogoView *logo;

static void
OnLogoPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  canvas.ClearWhite();
  logo->draw(canvas, Sender->GetClientRect());
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
  if (StringIsEmpty(path))
    return;

  Profile::SetFiles(path);

  /* When a profile from a secondary data path is used, this path
     becomes the primary data path */
  TCHAR temp[MAX_PATH];
  SetPrimaryDataPath(DirName(path, temp));

  File::Touch(path);
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnLogoPaint),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnQuitClicked),
  DeclareCallBackEntry(NULL)
};

bool
dlgStartupShowModal()
{
  LogFormat("Startup dialog");

  logo = new LogoView();

  wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  Layout::landscape ? _T("IDR_XML_STARTUP_L") :
                                      _T("IDR_XML_STARTUP"));
  assert(wf != NULL);

  WndProperty* wp = ((WndProperty *)wf->FindByName(_T("prpProfile")));
  assert(wp != NULL);

  DataFieldFileReader* dfe = (DataFieldFileReader*)wp->GetDataField();
  assert(dfe != NULL);

  dfe->ScanDirectoryTop(_T("*.prf"));

  if (dfe->GetNumFiles() <= 1) {
    SelectProfile(dfe->GetPathFile());

    delete wf;
    delete logo;
    return true;
  }

  unsigned best_index = 0;
  uint64_t best_timestamp = 0;
  unsigned length = dfe->size();

  for (unsigned i = 0; i < length; ++i) {
    const TCHAR *path = dfe->GetItem(i);
    uint64_t timestamp = File::GetLastModification(path);
    if (timestamp > best_timestamp) {
      best_timestamp = timestamp;
      best_index = i;
    }
  }
  dfe->Set(best_index);
  wp->RefreshDisplay();

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
