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
#include "Dialogs/dlgTools.h"
#include "InfoBoxLayout.h"
#include "MainWindow.hpp"
#include "Compatibility/string.h"

static WndForm *wf=NULL;
static WndOwnerDrawFrame *wGrid=NULL;

#define MAX_TEXTENTRY 40
static unsigned int cursor = 0;
static unsigned int max_width = MAX_TEXTENTRY;
static TCHAR edittext[MAX_TEXTENTRY];
#define MAXENTRYLETTERS (sizeof(EntryLetters)/sizeof(EntryLetters[0])-1)

static void UpdateTextboxProp(void)
{
  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(TEXT("prpText"));
  if (wp) {
    wp->SetText(edittext);
  }
}


static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam) {
  switch(wParam & 0xffff){
    case VK_LEFT:
      if (cursor<1)
        return(0); // min width
      cursor--;
      edittext[cursor] = 0;
      UpdateTextboxProp();
      return(0);
      /* JMW this prevents cursor buttons from being used to enter
    case VK_RETURN:
      wf->SetModalResult(mrOK);
      return(0);
      */
  }
  return(1);
}

static void OnKey(WindowControl * Sender)
{
  TCHAR *Caption = Sender->GetCaption();
  if (cursor < max_width-1)
    {
      edittext[cursor++] = Caption[0];
    }
  UpdateTextboxProp();
}

static void OnOk(WindowControl * Sender)
{
  wf->SetModalResult(mrOK);
}

static void ClearText(void)
{
  cursor = 0;
  memset(edittext, 0, sizeof(TCHAR)*MAX_TEXTENTRY);
  UpdateTextboxProp();
}


static void OnClear(WindowControl * Sender)
{
  ClearText();
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnKey),
  DeclareCallBackEntry(OnClear),
  DeclareCallBackEntry(OnOk),
  DeclareCallBackEntry(NULL)
};

void dlgTextEntryKeyboardShowModal(TCHAR *text, int width)
{
  wf = NULL;
  wGrid = NULL;
  if (width==0) {
    width = MAX_TEXTENTRY;
  }
  max_width = min(MAX_TEXTENTRY, width);

  if (InfoBoxLayout::landscape)
  {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("frmTextEntry_Keyboard_L.xml"),
			main_window,
			TEXT("IDR_XML_TEXTENTRY_KEYBOARD_L"));
    if (!wf) return;
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("frmTextEntry_Keyboard.xml"),
			main_window,
			TEXT("IDR_XML_TEXTENTRY_KEYBOARD"));
    if (!wf) return;
  }

  wGrid = (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmGrid"));

  cursor = 0;
  ClearText();

  /*edittext[0]= 0;
    edittext[1]= 0;*/

  if (_tcslen(text)>0) {
    _tcsupr(text);
    _tcsncpy(edittext, text, max_width-1);
    edittext[max_width-1]= 0;
  }

  UpdateTextboxProp();
  wf->SetKeyDownNotify(FormKeyDown);
  wf->ShowModal();
  _tcsncpy(text, edittext, max_width);
  text[max_width-1]=0;
  delete wf;
}


