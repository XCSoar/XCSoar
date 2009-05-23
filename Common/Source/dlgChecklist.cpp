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
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>

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

  $Id$
}
*/


#include "StdAfx.h"
#include <aygshell.h>

#include "Defines.h"
#include "XCSoar.h"
#include "externs.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"

#define MAXTITLE 200
#define MAXDETAILS 5000

static int page=0;
static WndForm *wf=NULL;
static WndListFrame *wDetails=NULL;
static WndOwnerDrawFrame *wDetailsEntry = NULL;

#define MAXLINES 100
#define MAXLISTS 20
static int LineOffsets[MAXLINES];
static int DrawListIndex=0;
static int nTextLines=0;
static int nLists=0;
static TCHAR *ChecklistText[MAXLISTS];
static TCHAR *ChecklistTitle[MAXLISTS];

static void NextPage(int Step){
  TCHAR buffer[80];
  page += Step;
  if (page>=nLists) {
    page=0;
  }
  if (page<0) {
    page= nLists-1;
  }

  nTextLines = TextToLineOffsets(ChecklistText[page],
				 LineOffsets,
				 MAXLINES);

  _stprintf(buffer, gettext(TEXT("Checklist")));

  if (ChecklistTitle[page] &&
      (_tcslen(ChecklistTitle[page])>0)
      && (_tcslen(ChecklistTitle[page])<60)) {
    _tcscat(buffer, TEXT(": "));
    _tcscat(buffer, ChecklistTitle[page]);
  }
  wf->SetCaption(buffer);

  wDetails->ResetList();
  wDetails->Redraw();

}


static void OnPaintDetailsListItem(WindowControl * Sender, HDC hDC){
  (void)Sender;
  if (DrawListIndex < nTextLines){
    TCHAR* text = ChecklistText[page];
    if (!text) return;
    int nstart = LineOffsets[DrawListIndex];
    int nlen;
    if (DrawListIndex<nTextLines-1) {
      nlen = LineOffsets[DrawListIndex+1]-LineOffsets[DrawListIndex]-1;
      nlen--;
    } else {
      nlen = _tcslen(text+nstart);
    }
    while (_tcscmp(text+nstart+nlen-1,TEXT("\r"))==0) {
      nlen--;
    }
    while (_tcscmp(text+nstart+nlen-1,TEXT("\n"))==0) {
      nlen--;
    }
    if (nlen>0) {
      ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		 ETO_OPAQUE, NULL,
		 text+nstart,
		 nlen,
		 NULL);
    }
  }
}


static void OnDetailsListInfo(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  (void)Sender;
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = nTextLines-1;
  } else {
    DrawListIndex = ListInfo->DrawIndex+ListInfo->ScrollIndex;
  }
}



static void OnNextClicked(WindowControl * Sender){
  (void)Sender;
  NextPage(+1);
}

static void OnPrevClicked(WindowControl * Sender){
  (void)Sender;
  NextPage(-1);
}

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
	(void)lParam;
	(void)Sender;
  switch(wParam & 0xffff){
    case VK_LEFT:
    case '6':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdPrev")))->GetHandle());
      NextPage(-1);
      //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return(0);
    case VK_RIGHT:
    case '7':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdNext")))->GetHandle());
      NextPage(+1);
      //((WndButton *)wf->FindByName(TEXT("cmdNext")))->SetFocused(true, NULL);
    return(0);
  }
  return(1);
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnPaintDetailsListItem),
  DeclareCallBackEntry(OnDetailsListInfo),
  DeclareCallBackEntry(NULL)
};

void addChecklist(TCHAR* name, TCHAR* details) {
  if (nLists<MAXLISTS) {
    ChecklistTitle[nLists] = (TCHAR*)malloc((_tcslen(name)+1)*sizeof(TCHAR));
    ChecklistText[nLists] = (TCHAR*)malloc((_tcslen(details)+1)*sizeof(TCHAR));
    _tcscpy(ChecklistTitle[nLists], name);
    ChecklistTitle[MAXTITLE-1]= 0;
    _tcscpy(ChecklistText[nLists], details);
    ChecklistText[MAXDETAILS-1]= 0;
    nLists++;
  }
}

void LoadChecklist(void) {
  HANDLE hChecklist;
  nLists = 0;
  if (ChecklistText[0]) {
    free(ChecklistText[0]);
    ChecklistText[0]= NULL;
  }
  if (ChecklistTitle[0]) {
    free(ChecklistTitle[0]);
    ChecklistTitle[0]= NULL;
  }

  TCHAR filename[MAX_PATH];
  LocalPath(filename, TEXT(XCSCHKLIST));

  hChecklist = INVALID_HANDLE_VALUE;
  hChecklist = CreateFile(filename,
			  GENERIC_READ,0,NULL,
			  OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL,NULL);
  if( hChecklist == INVALID_HANDLE_VALUE)
    {
      return;
    }
  /////
  TCHAR TempString[MAXTITLE];
  TCHAR Details[MAXDETAILS];
  TCHAR Name[100];
  BOOL inDetails = FALSE;
  int i;
//  int k=0;

  Details[0]= 0;
  Name[0]= 0;
  TempString[0]=0;

  while(ReadString(hChecklist,MAXTITLE,TempString))
    {
      int len = _tcslen(TempString);
      if (len>0) {
	// JMW strip extra \r if it exists
	if (TempString[len-1]=='\r') {
	  TempString[len-1]= 0;
	}
      }

      if(TempString[0]=='[') { // Look for start

	if (inDetails) {
	  wcscat(Details,TEXT("\r\n"));
	  addChecklist(Name, Details);
	  Details[0]= 0;
	  Name[0]= 0;
	}

	// extract name
	for (i=1; i<MAXTITLE; i++) {
	  if (TempString[i]==']') {
	    break;
	  }
	  Name[i-1]= TempString[i];
	}
	Name[i-1]= 0;

	inDetails = TRUE;

      } else {
	// append text to details string
	wcsncat(Details,TempString,MAXDETAILS-2);
	wcscat(Details,TEXT("\r\n"));
	// TODO code: check the string is not too long
      }
    }

  if (inDetails) {
    wcscat(Details,TEXT("\r\n"));
    addChecklist(Name, Details);
  }

  /////
  CloseHandle(hChecklist);
  hChecklist = NULL;

}


void dlgChecklistShowModal(void){
  static bool first=true;
  if (first) {
    LoadChecklist();
    first=false;
  }

  //  WndProperty *wp;

  if (!InfoBoxLayout::landscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgChecklist_L.xml"));
    wf = dlgLoadFromXML(CallBackTable,
                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_CHECKLIST_L"));
  } else {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgChecklist.xml"));
    wf = dlgLoadFromXML(CallBackTable,
                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_CHECKLIST"));
  }

  nTextLines = 0;

  if (!wf) return;

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wDetails = (WndListFrame*)wf->FindByName(TEXT("frmDetails"));
  ASSERT(wDetails!=NULL);

  wDetailsEntry =
    (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmDetailsEntry"));
  ASSERT(wDetailsEntry!=NULL);
  wDetailsEntry->SetCanFocus(true);

  wDetails->SetBorderKind(BORDERLEFT);

  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  wf = NULL;

}

