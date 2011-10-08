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

#include "Dialogs/Weather.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Message.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Language/Language.hpp"
#include "Net/Features.hpp"

#ifdef HAVE_NET

#include "Dialogs/TextEntry.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Form/List.hpp"
#include "Form/Button.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Weather/NOAAGlue.hpp"
#include "Weather/NOAAStore.hpp"
#include "Weather/METAR.hpp"
#include "Util/StaticArray.hpp"
#include "MainWindow.hpp"
#include "Compiler.h"

#include <stdio.h>

static WndForm *wf;
static WndListFrame *station_list;

struct NOAAListItem
{
  StaticString<5> code;
  unsigned index;

  bool operator<(const NOAAListItem &i2) const {
    return _tcscmp(code, i2.code) == -1;
  }
};

static StaticArray<NOAAListItem, 20> list;

static void
UpdateList()
{
  list.clear();

  unsigned len = NOAAStore::Count();
  for (unsigned i = 0; i < len; ++i) {
    NOAAListItem item;
    item.code = NOAAStore::GetCodeT(i);
    item.index = i;
    list.push_back(item);
  }

  if (len > 1)
    std::sort(list.begin(), list.end());

  station_list->SetLength(len);
  station_list->invalidate();
}

static void
PaintListItem(Canvas &canvas, const PixelRect rc, unsigned index)
{
  assert(index < list.size());

  const Font &code_font = Fonts::MapBold;
  const Font &details_font = Fonts::MapLabel;

  canvas.select(code_font);

  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + Layout::FastScale(2), rc, list[index].code);

  canvas.select(details_font);

  METAR metar;
  const TCHAR *tmp;
  if (!NOAAStore::GetMETAR(list[index].index, metar))
    tmp = _("No METAR available");
  else
    tmp = metar.content.c_str();

  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + code_font.get_height() + Layout::FastScale(4),
                      rc, tmp);
}

static void
AddClicked(gcc_unused WndButton &button)
{
  if (NOAAStore::Full()) {
    MessageBoxX(_("The list of tracked stations is full. Please remove a station before adding a new one."),
                _("Error"), MB_OK);
    return;
  }

  TCHAR code[5] = _T("");
  if (!dlgTextEntryShowModal(*(SingleWindow *)button.get_root_owner(),
                             code, 5, _("Airport ICAO code")))
    return;

  if (_tcslen(code) != 4) {
    MessageBoxX(_("Please enter the FOUR letter code of the desired station."),
                _("Error"), MB_OK);
    return;
  }

  for (unsigned i = 0; i < 4; i++) {
    if (code[i] < _T('A') || code[i] > _T('Z')) {
      MessageBoxX(_("Please don't use special characters in the four letter code of the desired station."),
                  _("Error"), MB_OK);
      return;
    }
  }

  NOAAStore::AddStation(code);
  NOAAStore::SaveToProfile();

  DialogJobRunner runner(wf->GetMainWindow(), wf->GetLook(),
                         _("Download"), true);
  NOAAStore::UpdateStation(code, runner);

  UpdateList();
}

static void
UpdateClicked(gcc_unused WndButton &Sender)
{
  DialogJobRunner runner(wf->GetMainWindow(), wf->GetLook(),
                         _("Download"), true);
  NOAAStore::Update(runner);
  UpdateList();
}

static void
RemoveClicked(gcc_unused WndButton &Sender)
{
  unsigned index = station_list->GetCursorIndex();
  assert(index < list.size());

  TCHAR tmp[256];
  _stprintf(tmp, _("Do you want to remove station %s?"),
            list[index].code.c_str());

  if (MessageBoxX(tmp, _("Remove"), MB_YESNO) == IDNO)
    return;

  NOAAStore::RemoveStation(list[index].index);
  NOAAStore::SaveToProfile();

  UpdateList();
}

static void
CloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void
OpenDetails(unsigned index)
{
  assert(index < list.size());
  dlgNOAADetailsShowModal(*(SingleWindow *)wf->get_root_owner(),
                          list[index].index);
  UpdateList();
}

static void
DetailsClicked(gcc_unused WndButton &Sender)
{
  if (station_list->GetLength() > 0)
    OpenDetails(station_list->GetCursorIndex());
}

static void
ListItemSelected(unsigned index)
{
  OpenDetails(index);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(PaintListItem),
  DeclareCallBackEntry(ListItemSelected),
  DeclareCallBackEntry(AddClicked),
  DeclareCallBackEntry(UpdateClicked),
  DeclareCallBackEntry(RemoveClicked),
  DeclareCallBackEntry(CloseClicked),
  DeclareCallBackEntry(DetailsClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgNOAAListShowModal(SingleWindow &parent)
{
  wf = LoadDialog(CallBackTable, parent, Layout::landscape ?
                  _T("IDR_XML_NOAA_LIST_L") :
                  _T("IDR_XML_NOAA_LIST"));
  assert(wf != NULL);

  unsigned item_height = Fonts::MapBold.get_height() + Layout::Scale(6) +
                         Fonts::MapLabel.get_height();

  station_list = (WndListFrame *)wf->FindByName(_T("StationList"));
  assert(station_list != NULL);
  station_list->SetItemHeight(item_height);
  station_list->SetPaintItemCallback(PaintListItem);
  station_list->SetActivateCallback(ListItemSelected);
  UpdateList();

  wf->ShowModal();

  delete wf;
}

#else
void
dlgNOAAListShowModal(SingleWindow &parent)
{
  MessageBoxX(_("This function is not available on your platform yet."),
              _("Error"), MB_OK);
}
#endif
