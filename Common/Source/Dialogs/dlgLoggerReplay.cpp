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

#include "XCSoar.h"
#include "Interface.hpp"
#include "Units.h"
#include "ReplayLogger.hpp"
#include "Dialogs/dlgTools.h"
#include "DataField/FileReader.hpp"
#include "Screen/MainWindow.hpp"

static WndForm *wf=NULL;


static void OnStopClicked(WindowControl * Sender){
	(void)Sender;
  ReplayLogger::Stop();
}

static void OnStartClicked(WindowControl * Sender){
	(void)Sender;
  WndProperty* wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpIGCFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    ReplayLogger::SetFilename(dfe->GetPathFile());
  }
  ReplayLogger::Start();
}


static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}


static void OnRateData(DataField *Sender, DataField::DataAccessKind_t Mode){

  switch(Mode){
    case DataField::daGet:
      Sender->Set(ReplayLogger::TimeScale);
    break;
    case DataField::daPut:
    case DataField::daChange:
      ReplayLogger::TimeScale = Sender->GetAsFloat();
    break;
  }

}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnStopClicked),
  DeclareCallBackEntry(OnStartClicked),
  DeclareCallBackEntry(OnRateData),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};


void dlgLoggerReplayShowModal(void){
  wf = dlgLoadFromXML(CallBackTable,
                      TEXT("dlgLoggerReplay.xml"),
		      hWndMainWindow,
		      TEXT("IDR_XML_LOGGERREPLAY"));

  WndProperty* wp;

  if (wf) {

    wp = (WndProperty*)wf->FindByName(TEXT("prpRate"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(ReplayLogger::TimeScale);
      wp->RefreshDisplay();
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpIGCFile"));
    if (wp) {
      DataFieldFileReader* dfe;
      dfe = (DataFieldFileReader*)wp->GetDataField();
      dfe->ScanDirectoryTop(TEXT("*.igc"));
      dfe->Lookup(ReplayLogger::GetFilename());
      wp->RefreshDisplay();
    }

    wf->ShowModal();
    delete wf;
  }
  wf = NULL;
}

