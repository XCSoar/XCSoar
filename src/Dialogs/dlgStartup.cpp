/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Dialogs/Internal.hpp"
#include "resource.h"
#include "Screen/Layout.hpp"
#include "DataField/FileReader.hpp"
#include "LogFile.hpp"
#include "Screen/Bitmap.hpp"
#include "MainWindow.hpp"
#include "Compatibility/string.h"
#include "Version.hpp"
#include "Asset.hpp"
#include "StringUtil.hpp"
#include "Compiler.h"

static WndForm *wf = NULL;
extern TCHAR startProfileFile[];

/*
 * use a smaller icon for smaller screens because the "stretch" will not shrink
 */
static void
OnSplashPaint(WindowControl *Sender, Canvas &canvas)
{
  Bitmap splash_bitmap;
  if (Layout::scale_1024 > 1024 * 3 / 2)
    splash_bitmap.load(IDB_SWIFT);
  else
    splash_bitmap.load(IDB_SWIFT2);

  BitmapCanvas bitmap_canvas(canvas, splash_bitmap);
  canvas.stretch(bitmap_canvas);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static CallBackTableEntry_t CallBackTable[] = {
  DeclareCallBackEntry(OnSplashPaint),
  DeclareCallBackEntry(NULL)
};

void
dlgStartupShowModal()
{
  WndProperty* wp;
  LogStartUp(_T("Startup dialog"));

  if (Layout::landscape)
    wf = LoadDialog(CallBackTable,
                        XCSoarInterface::main_window, _T("IDR_XML_STARTUP_L"));
  else
    wf = LoadDialog(CallBackTable,
                        XCSoarInterface::main_window, _T("IDR_XML_STARTUP"));

  if (!wf)
    return;

  wp = ((WndProperty *)wf->FindByName(_T("prpProfile")));
  if (!wp)
    return;

  DataFieldFileReader* dfe;
  dfe = (DataFieldFileReader*)wp->GetDataField();
  if (!dfe)
    return;

  ((WndButton *)wf->FindByName(_T("cmdClose")))
    ->SetOnClickNotify(OnCloseClicked);

  TCHAR temp[MAX_PATH];

  _stprintf(temp, _T("XCSoar: Version %s"), XCSoar_VersionString);
  WindowControl* wc;
  wc = ((WindowControl *)wf->FindByName(_T("lblVersion")));
  if (wc)
    wc->SetCaption(temp);

  static WndFrame* wDisclaimer1 = (WndFrame*)wf->FindByName(_T("frmDisclaimer1"));
  wDisclaimer1->SetCaption(_T("Pilot assumes complete responsibility to ")
                           _T("operate the aircraft safely. Maintain ")
                           _T("effective lookout."));

  dfe->ScanDirectoryTop(is_altair() ? _T("config/*.prf") : _T("*.prf"));
  dfe->Lookup(startProfileFile);
  wp->RefreshDisplay();

  if (dfe->GetNumFiles() <= 2) {
    delete wf;
    return;
  }

  wf->ShowModal();

  if (!string_is_empty(dfe->GetPathFile()))
    _tcscpy(startProfileFile, dfe->GetPathFile());

  delete wf;
}
