/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Dialogs/TextEntry.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/Keyboard.hpp"
#include "Form/Edit.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "MainWindow.hpp"
#include "Compatibility/string.h"
#include "StringUtil.hpp"

#include <algorithm>
#include <assert.h>

using std::min;

static WndForm *wf = NULL;
static KeyboardControl *kb = NULL;

static AllowedCharactersCallback_t AllowedCharactersCallback;

#define MAX_TEXTENTRY 40
static unsigned int cursor = 0;
static unsigned int max_width = MAX_TEXTENTRY;
static TCHAR edittext[MAX_TEXTENTRY];

static void
UpdateAllowedCharacters()
{
  if (AllowedCharactersCallback != NULL)
    kb->SetAllowedCharacters(AllowedCharactersCallback(edittext));
}

static void
UpdateTextboxProp(void)
{
  WndProperty *wp = (WndProperty*)wf->FindByName(_T("prpText"));
  assert(wp != NULL);
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
FormKeyDown(WndForm &Sender, unsigned key_code)
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
OnBackspace(WndButton &Sender)
{
  DoBackspace();
}

static void
OnOk(WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void
OnCancel(WndButton &Sender)
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
OnClear(WndButton &Sender)
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
dlgTextEntryKeyboardShowModal(SingleWindow &parent, TCHAR *text,
                              int width, const TCHAR* caption,
                              AllowedCharactersCallback_t accb)
{
  if (width == 0)
    width = MAX_TEXTENTRY;

  max_width = min(MAX_TEXTENTRY, width);

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ? _T("IDR_XML_TEXTENTRY_KEYBOARD_L") :
                                      _T("IDR_XML_TEXTENTRY_KEYBOARD"));
  assert(wf != NULL);

  if (caption)
    wf->SetCaption(caption);

  AllowedCharactersCallback = accb;

  kb = (KeyboardControl*)wf->FindByName(_T("Keyboard"));
  assert(kb != NULL);

  cursor = 0;
  ClearText();

  if (!string_is_empty(text)) {
    _tcsupr(text);
    CopyString(edittext, text, max_width);
    cursor = _tcslen(text);
  }

  UpdateTextboxProp();
  wf->SetKeyDownNotify(FormKeyDown);
  bool result = (wf->ShowModal() == mrOK);

  if (result) {
    CopyString(text, edittext, max_width);
  }

  delete wf;
  return result;
}
