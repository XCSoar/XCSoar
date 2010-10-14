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
#include "Form/Keyboard.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "Compatibility/string.h"
#include "StringUtil.hpp"

#include <algorithm>

using std::min;

static WndForm *wf = NULL;
static KeyboardControl *kb = NULL;

static AllowedCharactersCallback_t AllowedCharactersCallback;

#define MAX_TEXTENTRY 40
static unsigned int cursor = 0;
static unsigned int max_width = MAX_TEXTENTRY;
static TCHAR edittext[MAX_TEXTENTRY];
#define MAXENTRYLETTERS (sizeof(EntryLetters)/sizeof(EntryLetters[0])-1)

static void
UpdateAllowedCharacters()
{
  if (AllowedCharactersCallback != NULL)
    kb->SetAllowedCharacters(AllowedCharactersCallback(edittext));
}

static void
UpdateTextboxProp(void)
{
  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(_T("prpText"));
  if (!wp)
    return;

  wp->SetText(edittext);

  UpdateAllowedCharacters();
}

static bool
DoBackspace()
{
  if (cursor < 1)
    return false;

  cursor--;
  edittext[cursor] = 0;
  UpdateTextboxProp();
  return true;
}

static bool
DoCharacter(TCHAR character)
{
  if (cursor >= max_width - 1)
    return false;

  edittext[cursor++] = character;
  edittext[cursor] = 0;
  UpdateTextboxProp();
  return true;
}

static bool
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
  switch (key_code) {
  case VK_RIGHT:
    return true;
  case VK_LEFT:
  case VK_BACK:
    DoBackspace();
    return true;
  }

  if (!has_keyboard())
    return false;

  if ((key_code >= 'A' && key_code <= 'Z') ||
      (key_code >= '0' && key_code <= '9')) {
    DoCharacter(key_code);
    return true;
  }

  if (key_code == VK_SPACE) {
    DoCharacter(_T(' '));
    return true;
  }

  return false;
}

static void
OnBackspace(WindowControl * Sender)
{
  DoBackspace();
}

static void
OnOk(WindowControl * Sender)
{
  wf->SetModalResult(mrOK);
}

static void
OnCancel(WindowControl * Sender)
{
  wf->SetModalResult(mrCancel);
}

static void
ClearText(void)
{
  cursor = 0;
  edittext[0] = 0;
  UpdateTextboxProp();
}

static void
OnClear(WindowControl * Sender)
{
  ClearText();
}

static void
OnCharacter(TCHAR character)
{
  DoCharacter(character);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCharacter),
  DeclareCallBackEntry(OnBackspace),
  DeclareCallBackEntry(OnClear),
  DeclareCallBackEntry(OnCancel),
  DeclareCallBackEntry(OnOk),
  DeclareCallBackEntry(NULL)
};

bool
dlgTextEntryKeyboardShowModal(TCHAR *text, int width,
                              AllowedCharactersCallback_t accb)
{
  if (width == 0)
    width = MAX_TEXTENTRY;

  max_width = min(MAX_TEXTENTRY, width);

  if (Layout::landscape)
    wf = LoadDialog(CallBackTable,
			XCSoarInterface::main_window, _T("IDR_XML_TEXTENTRY_KEYBOARD_L"));
  else
    wf = LoadDialog(CallBackTable,
			XCSoarInterface::main_window, _T("IDR_XML_TEXTENTRY_KEYBOARD"));

  if (!wf)
    return false;

  AllowedCharactersCallback = accb;

  kb = (KeyboardControl*)wf->FindByName(_T("Keyboard"));
  if (!kb)
    return false;


  cursor = 0;
  ClearText();

  if (!string_is_empty(text)) {
    _tcsupr(text);
    _tcsncpy(edittext, text, max_width - 1);
    edittext[max_width - 1] = 0;
    cursor = _tcslen(text);
  }

  UpdateTextboxProp();
  wf->SetKeyDownNotify(FormKeyDown);
  bool result = (wf->ShowModal() == mrOK);

  if (result) {
    _tcsncpy(text, edittext, max_width);
    text[max_width - 1] = 0;
  }

  delete wf;
  return result;
}
