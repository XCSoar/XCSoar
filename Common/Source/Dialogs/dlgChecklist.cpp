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
#include "Screen/Layout.hpp"
#include "LocalPath.hpp"
#include "UtilsText.hpp"
#include "MainWindow.hpp"
#include "Defines.h"
#include "StringUtil.hpp"

#include <assert.h>

#define MAXTITLE 200
#define MAXDETAILS 5000

static int page=0;
static WndForm *wf=NULL;
static WndListFrame *wDetails=NULL;

#define MAXLINES 100
#define MAXLISTS 20
static int LineOffsets[MAXLINES];
static int DrawListIndex=0;
static int nTextLines=0;
static int nLists=0;
static TCHAR *ChecklistText[MAXDETAILS];
static TCHAR *ChecklistTitle[MAXTITLE];

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

  _stprintf(buffer, gettext(_T("Checklist")));

  if (ChecklistTitle[page] && !string_is_empty(ChecklistTitle[page])
      && (_tcslen(ChecklistTitle[page])<60)) {
    _tcscat(buffer, _T(": "));
    _tcscat(buffer, ChecklistTitle[page]);
  }
  wf->SetCaption(buffer);

  wDetails->ResetList();
  wDetails->invalidate();
}


static void
OnPaintDetailsListItem(WindowControl *Sender, Canvas &canvas)
{
  (void)Sender;
  if (DrawListIndex >= nTextLines)
    return;

  TCHAR* text = ChecklistText[page];
  if (text == NULL)
    return;

  int nstart = LineOffsets[DrawListIndex];
  int nlen;
  if (DrawListIndex < nTextLines - 1) {
    nlen = LineOffsets[DrawListIndex + 1] - LineOffsets[DrawListIndex] - 1;
    nlen--;
  } else {
    nlen = _tcslen(text + nstart);
  }

  while (_tcscmp(text + nstart + nlen - 1, _T("\r")) == 0)
    nlen--;

  while (_tcscmp(text + nstart + nlen - 1, _T("\n")) == 0)
    nlen--;

  if (nlen > 0)
    canvas.text_opaque(Layout::FastScale(2), Layout::FastScale(2),
                       text + nstart, nlen);
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

static bool
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
	(void)Sender;

  switch (key_code) {
    case VK_LEFT:
    case '6':
      ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
      NextPage(-1);
      //((WndButton *)wf->FindByName(_T("cmdPrev")))->SetFocused(true, NULL);
    return true;

    case VK_RIGHT:
    case '7':
      ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
      NextPage(+1);
      //((WndButton *)wf->FindByName(_T("cmdNext")))->SetFocused(true, NULL);
    return true;

  default:
    return false;
  }
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnPaintDetailsListItem),
  DeclareCallBackEntry(OnDetailsListInfo),
  DeclareCallBackEntry(NULL)
};

static void
addChecklist(TCHAR *name, TCHAR *details)
{
  if (nLists >= MAXLISTS)
    return;

  ChecklistTitle[nLists] = (TCHAR*)malloc((_tcslen(name)+1)*sizeof(TCHAR));
  ChecklistText[nLists] = (TCHAR*)malloc((_tcslen(details)+1)*sizeof(TCHAR));
  _tcscpy(ChecklistTitle[nLists], name);
  ChecklistTitle[MAXTITLE-1]= 0;
  _tcscpy(ChecklistText[nLists], details);
  ChecklistText[MAXDETAILS-1]= 0;
  nLists++;
}

void LoadChecklist(void) {
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
  LocalPath(filename, _T(XCSCHKLIST));

  FILE *file = _tfopen(filename, _T("rt"));
  if (file == NULL)
    {
      return;
    }

  TCHAR TempString[MAXTITLE];
  TCHAR Details[MAXDETAILS];
  TCHAR Name[100];
  BOOL inDetails = FALSE;
  int i;
//  int k=0;

  Details[0]= 0;
  Name[0]= 0;
  TempString[0]=0;

  while (ReadStringX(file, MAXTITLE, TempString))
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
	  _tcscat(Details,_T("\r\n"));
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
	_tcsncat(Details,TempString,MAXDETAILS-2);
	_tcscat(Details,_T("\r\n"));
	// TODO code: check the string is not too long
      }
    }

  if (inDetails) {
    _tcscat(Details,_T("\r\n"));
    addChecklist(Name, Details);
  }

  fclose(file);
}


void dlgChecklistShowModal(void){
  static bool first=true;
  if (first) {
    LoadChecklist();
    first=false;
  }

  //  WndProperty *wp;

  if (!Layout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgChecklist_L.xml"),
                        XCSoarInterface::main_window,
                        _T("IDR_XML_CHECKLIST_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgChecklist.xml"),
                        XCSoarInterface::main_window,
                        _T("IDR_XML_CHECKLIST"));
  }

  nTextLines = 0;

  if (!wf) return;

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wDetails = (WndListFrame*)wf->FindByName(_T("frmDetails"));
  assert(wDetails!=NULL);

  wDetails->SetBorderKind(BORDERLEFT);

  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  wf = NULL;

}

