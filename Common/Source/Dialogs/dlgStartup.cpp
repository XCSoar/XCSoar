/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Dialogs.h"
#include "XCSoar.h"
#include "Dialogs/dlgTools.h"
#include "InfoBoxLayout.h"
#include "DataField/FileReader.hpp"
#include "LogFile.hpp"
#include "Screen/MainWindow.hpp"
#include "Compatibility/string.h"
#include "Version.hpp"

static WndForm *wf=NULL;
static WndOwnerDrawFrame *wSplash=NULL;
static HBITMAP hSplash;
extern HINSTANCE hInst;

static void OnSplashPaint(WindowControl * Sender, HDC hDC){

  RECT  rc;

  hSplash=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_DISCLAIMER));

  CopyRect(&rc, Sender->GetBoundRect());
  HDC hDCTemp = CreateCompatibleDC(hDC);

  SelectObject(hDCTemp, hSplash);
  StretchBlt(hDC,
	     rc.left, rc.top,
	     rc.right, rc.bottom,
	     hDCTemp, 0, 0, 318, 163, SRCCOPY);

  DeleteObject(hSplash);
  DeleteDC(hDCTemp);

}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnSplashPaint),
  DeclareCallBackEntry(NULL)
};

extern TCHAR startProfileFile[];

void dlgStartupShowModal(void){
  WndProperty* wp;
  StartupStore(TEXT("Startup dialog\n"));

  if (!InfoBoxLayout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgStartup_L.xml"),
                        main_window,
                        TEXT("IDR_XML_STARTUP_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgStartup.xml"),
                        main_window,
                        TEXT("IDR_XML_STARTUP"));
  }
  if (!wf) return;

  wSplash = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmSplash"));

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))
    ->SetOnClickNotify(OnCloseClicked);

  TCHAR temp[MAX_PATH];

  _stprintf(temp,TEXT("XCSoar: Version %s"), XCSoar_Version);
  wf->SetCaption(temp);

  wp = ((WndProperty *)wf->FindByName(TEXT("prpDisclaimer")));
  if (wp)
    wp->SetText(TEXT("Pilot assumes complete\r\nresponsibility to operate\r\nthe aircraft safely.\r\nMaintain effective lookout.\r\n"));

  wp = ((WndProperty *)wf->FindByName(TEXT("prpProfile")));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
#ifdef GNAV
    dfe->ScanDirectoryTop(TEXT("config/*.prf"));
#else
    dfe->ScanDirectoryTop(TEXT("*.prf"));
#endif
    dfe->Lookup(startProfileFile);
    wp->RefreshDisplay();
    if (dfe->GetNumFiles()<=2) {
      delete wf;
      wf = NULL;
      return;
    }
  }

  wf->ShowModal();

  wp = (WndProperty*)wf->FindByName(TEXT("prpProfile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    if (_tcslen(dfe->GetPathFile())>0) {
      _tcscpy(startProfileFile,dfe->GetPathFile());
    }
  }

  delete wf;

  wf = NULL;

}

