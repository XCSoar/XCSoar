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
#include "Version.hpp"
#include "Asset.hpp"
#include "StringUtil.hpp"
#include "LocalPath.hpp"
#include "OS/PathName.hpp"
#include "Compiler.h"

#include <stdio.h>

static WndForm *wf = NULL;
static Bitmap bitmap_title;
static Bitmap bitmap_logo;
extern TCHAR startProfileFile[];

static void
OnLogoPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  BitmapCanvas bitmap_canvas(canvas, bitmap_logo);
  canvas.stretch(bitmap_canvas);
}

static void
OnTitlePaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  BitmapCanvas bitmap_canvas(canvas, bitmap_title);
  canvas.stretch(bitmap_canvas);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static void
OnQuit(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrCancel);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnTitlePaint),
  DeclareCallBackEntry(OnLogoPaint),
  DeclareCallBackEntry(NULL)
};

bool
dlgStartupShowModal()
{
  LogStartUp(_T("Startup dialog"));

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ? _T("IDR_XML_STARTUP_L") :
                                      _T("IDR_XML_STARTUP"));
  assert(wf != NULL);

  WndProperty* wp = ((WndProperty *)wf->FindByName(_T("prpProfile")));
  assert(wp != NULL);

  DataFieldFileReader* dfe = (DataFieldFileReader*)wp->GetDataField();
  assert(dfe != NULL);

  ((WndButton *)wf->FindByName(_T("cmdClose")))
    ->SetOnClickNotify(OnCloseClicked);

  ((WndButton *)wf->FindByName(_T("cmdQuit")))->SetOnClickNotify(OnQuit);

  TCHAR temp[MAX_PATH];

  _stprintf(temp, _T("XCSoar v%s"), XCSoar_VersionString);
  WindowControl* wc;
  wc = ((WindowControl *)wf->FindByName(_T("lblVersion")));
  if (wc)
    wc->SetCaption(temp);

  dfe->ScanDirectoryTop(is_altair() ? _T("config/*.prf") : _T("*.prf"));
  dfe->Lookup(startProfileFile);
  wp->RefreshDisplay();

  if (dfe->GetNumFiles() <= 2) {
    delete wf;
    return true;
  }

  // Determine window size
  int window_width = wf->get_width();
  int window_height = wf->get_height();

  bitmap_title.load(window_width > 272 && window_height > 272 ?
                    IDB_TITLE_HD : IDB_TITLE);
  SIZE title_size = bitmap_title.get_size();

  // Determine logo size
  bitmap_logo.load(window_width > 272 && window_height > 272 ?
                   IDB_SWIFT_HD : IDB_SWIFT);
  SIZE logo_size = bitmap_logo.get_size();

  // Determine logo and title positions
  bool hidetitle = false;
  int logox, logoy, titlex, titley;
  if (window_width > window_height) {
    // Landscape
    logox = (window_width - (logo_size.cx + title_size.cy + title_size.cx)) / 2;
    logoy = (window_height - logo_size.cy - window_height / 10) / 2;
    titlex = logox + logo_size.cx + title_size.cy;
    titley = (window_height - title_size.cy - window_height / 10) / 2;
  } else if (window_width < window_height) {
    // Portrait
    logox = (window_width - logo_size.cx) / 2;
    logoy = (window_height - (logo_size.cy + title_size.cy * 2) -
             window_height / 10) / 2;
    titlex = (window_width - title_size.cx) / 2;
    titley = logoy + logo_size.cy + title_size.cy;
  } else {
    // Square screen
    logox = (window_width - logo_size.cx) / 2;
    logoy = (window_height - logo_size.cy - window_height / 10) / 2;
    hidetitle = true;
  }

  wc = ((WindowControl*)wf->FindByName(_T("frmLogo")));
  wc->move(logox, logoy, logo_size.cx, logo_size.cy);

  wc = ((WindowControl*)wf->FindByName(_T("frmTitle")));
  if (hidetitle)
    wc->hide();
  else
    wc->move(titlex, titley, title_size.cx, title_size.cy);

  if (wf->ShowModal() != mrOK) {
    delete wf;
    return false;
  }

  const TCHAR *path = dfe->GetPathFile();
  if (!string_is_empty(path)) {
    _tcscpy(startProfileFile, path);

    /* When a profile from a secondary data path is used, this path
       becomes the primary data path */
    SetPrimaryDataPath(DirName(path, temp));
  }

  delete wf;
  return true;
}
