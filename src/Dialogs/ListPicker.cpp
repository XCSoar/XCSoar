/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Dialogs/ListPicker.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"
#include "Screen/Layout.hpp"

#include <assert.h>

static WndForm *wf;
static ListHelpCallback_t help_callback;

static WndFrame *wItemHelp;
static ListControl *list_control;
static ItemHelpCallback_t itemhelp_callback;

static void
OnHelpClicked(gcc_unused WndButton &button)
{
  assert(help_callback != NULL);

  unsigned i = list_control->GetCursorIndex();
  if (i < list_control->GetLength())
    help_callback(i);
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void
OnComboPopupListEnter(gcc_unused unsigned i)
{
  wf->SetModalResult(mrOK);
}

static void
OnCancelClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrCancel);
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  list_control->Invalidate();
}

static void
OnPointCursorCallback(unsigned i)
{
  assert(wItemHelp);
  assert(itemhelp_callback);
  const TCHAR* itemhelp = itemhelp_callback(i);
  wItemHelp->SetText(itemhelp);
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnCancelClicked),
  DeclareCallBackEntry(OnHelpClicked),
  DeclareCallBackEntry(NULL)
};

int
ListPicker(SingleWindow &parent, const TCHAR *caption,
           unsigned num_items, unsigned initial_value,
           UPixelScalar item_height,
           ListControl::PaintItemCallback paint_callback, bool update,
           ListHelpCallback_t _help_callback,
           ItemHelpCallback_t _itemhelp_callback)
{
  assert(num_items <= 0x7fffffff);
  assert((num_items == 0 && initial_value == 0) || initial_value < num_items);
  assert(item_height > 0);
  assert(paint_callback != NULL);

  wf = LoadDialog(CallBackTable, parent, Layout::landscape
                  ? _T("IDR_XML_COMBOPICKER_L")
                  : _T("IDR_XML_COMBOPICKER"));
  assert(wf != NULL);

  if (caption != NULL)
    wf->SetCaption(caption);

  list_control = (ListControl *)wf->FindByName(_T("frmComboPopupList"));
  assert(list_control != NULL);
  list_control->SetItemHeight(item_height);
  list_control->SetLength(num_items);
  list_control->SetActivateCallback(OnComboPopupListEnter);
  list_control->SetPaintItemCallback(paint_callback);

  help_callback = _help_callback;
  itemhelp_callback = _itemhelp_callback;

  if (itemhelp_callback != NULL) {
    wItemHelp = (WndFrame *)wf->FindByName(_T("lblItemHelp"));
    assert(wItemHelp);
    wItemHelp->Show();
    const UPixelScalar help_height = wItemHelp->GetHeight();
    PixelRect rc = list_control->GetPosition();
    assert(rc.bottom - rc.top - help_height > 0);
    rc.bottom -= help_height;
    list_control->Move(rc);
    list_control->SetCursorCallback(OnPointCursorCallback);
    OnPointCursorCallback(initial_value);
  }
  else
    wItemHelp = NULL;

  list_control->SetCursorIndex(initial_value);

  WndButton *help_button = (WndButton *)wf->FindByName(_T("cmdHelp"));
  assert(help_button != NULL);
  if (help_callback == NULL)
    help_button->Hide();

  if (num_items == 0) {
    WndButton *select_button = (WndButton *)wf->FindByName(_T("SelectButton"));
    assert(select_button != NULL);
    select_button->SetEnabled(false);
  }

  if (update)
    wf->SetTimerNotify(OnTimerNotify);

  int result = wf->ShowModal() == mrOK
    ? (int)list_control->GetCursorIndex()
    : -1;
  delete wf;

  return result;
}
