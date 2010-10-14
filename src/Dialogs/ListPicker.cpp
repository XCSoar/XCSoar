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

#include "Dialogs/ListPicker.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Screen/Layout.hpp"

#include <assert.h>

static WndForm *wf;
static ListHelpCallback_t help_callback;

static WndListFrame *list_control;

static void
OnHelpClicked(WndButton &button)
{
  (void)button;

  assert(help_callback != NULL);

  unsigned i = list_control->GetCursorIndex();
  if (i < list_control->GetLength())
    help_callback(i);
}

static void
OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;

  wf->SetModalResult(mrOK);
}

static void
OnComboPopupListEnter(unsigned i)
{
  wf->SetModalResult(mrOK);
}

static void
OnCancelClicked(WindowControl * Sender)
{
  (void)Sender;

  wf->SetModalResult(mrCancel);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnCancelClicked),
  DeclareCallBackEntry(NULL)
};

int
ListPicker(SingleWindow &parent, const TCHAR *caption,
           unsigned num_items, unsigned initial_value, unsigned item_height,
           WndListFrame::PaintItemCallback_t paint_callback,
           ListHelpCallback_t _help_callback)
{
  assert(num_items <= 0x7fffffff);
  assert((num_items == 0 && initial_value == 0) || initial_value < num_items);
  assert(item_height > 0);
  assert(paint_callback != NULL);

  wf = LoadDialog(CallBackTable, parent, !Layout::landscape
                  ? _T("IDR_XML_COMBOPICKER_L")
                  : _T("IDR_XML_COMBOPICKER"));
  assert(wf != NULL);

  if (caption != NULL)
    wf->SetCaption(caption);

  list_control = (WndListFrame *)wf->FindByName(_T("frmComboPopupList"));
  assert(list_control != NULL);
  list_control->SetItemHeight(item_height);
  list_control->SetLength(num_items);
  list_control->SetCursorIndex(initial_value);
  list_control->SetActivateCallback(OnComboPopupListEnter);
  list_control->SetPaintItemCallback(paint_callback);

  help_callback = _help_callback;

  WndButton *help_button = (WndButton *)wf->FindByName(_T("cmdHelp"));
  assert(help_button != NULL);
  if (help_callback != NULL)
    help_button->SetOnClickNotify(OnHelpClicked);
  else
    help_button->hide();

  int result = wf->ShowModal() == mrOK
    ? (int)list_control->GetCursorIndex()
    : -1;
  delete wf;

  return result;
}
