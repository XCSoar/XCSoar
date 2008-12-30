/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008  

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

#include "StdAfx.h"

#include "Statistics.h"

#include "externs.h"
#include "Units.h"
#include "Waypointparser.h"

#include "dlgTools.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;

static void OnYesClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(wpTerrainBoundsYes);
}

static void OnYesAllClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(wpTerrainBoundsYesAll);
}

static void OnNoClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(wpTerrainBoundsNo);
}

static void OnNoAllClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(wpTerrainBoundsNoAll);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnYesClicked),
  DeclareCallBackEntry(OnYesAllClicked),
  DeclareCallBackEntry(OnNoClicked),
  DeclareCallBackEntry(OnNoAllClicked),
  DeclareCallBackEntry(NULL)
};

int dlgWaypointOutOfTerrain(TCHAR *Message){

  WndFrame* wfrm;
  int res = 0;

#ifdef HAVEEXCEPTIONS
  __try{
#endif

    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgWaypointOutOfTerrain.xml"));
    wf = dlgLoadFromXML(CallBackTable, 
		        
                        filename, 
		        hWndMainWindow,
		        TEXT("IDR_XML_WAYPOINTTERRAIN"));

    if (wf) {

    
      wfrm = (WndFrame*)wf->FindByName(TEXT("frmWaypointOutOfTerrainText"));

      wfrm->SetCaption(Message);
      wfrm->SetCaptionStyle(
          DT_EXPANDTABS
        | DT_CENTER
        | DT_NOCLIP
        | DT_WORDBREAK);


      res = wf->ShowModal();
      delete wf;

    }

    wf = NULL;

#ifdef HAVEEXCEPTIONS
  }__except(EXCEPTION_EXECUTE_HANDLER ){

    res = 0; 
    // ToDo: log that problem

  };
#endif

  return(res);

}

