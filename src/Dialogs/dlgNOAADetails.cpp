/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Dialogs/JobDialog.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Language/Language.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_NOAA

#include "Weather/NOAAGlue.hpp"
#include "Weather/NOAAStore.hpp"
#include "Weather/NOAAUpdater.hpp"
#include "Weather/METAR.hpp"
#include "Weather/ParsedMETAR.hpp"
#include "Weather/TAF.hpp"
#include "Weather/NOAAFormatter.hpp"
#include "Formatter/Units.hpp"
#include "Screen/EditWindow.hpp"
#include "Screen/Layout.hpp"

#include <stdio.h>

static WndForm *wf = NULL;
static NOAAStore::iterator station_iterator;

static void
Update()
{
  tstring metar_taf = _T("");

  NOAAFormatter::Format(*station_iterator, metar_taf);

  ((EditWindow *)wf->FindByName(_T("DetailsText")))->SetText(metar_taf.c_str());

  StaticString<100> caption;
  caption.Format(_T("%s: "), _("METAR and TAF"));

  if (!station_iterator->parsed_metar_available ||
      !station_iterator->parsed_metar.name_available)
    caption += station_iterator->GetCodeT();
  else
    caption.AppendFormat(_T("%s (%s)"),
                         station_iterator->parsed_metar.name.c_str(),
                         station_iterator->GetCodeT());

  wf->SetCaption(caption);
}

static void
UpdateClicked(gcc_unused WndButton &Sender)
{
  DialogJobRunner runner(wf->GetMainWindow(), wf->GetLook(),
                         _("Download"), true);
  NOAAUpdater::Update(*station_iterator, runner);
  Update();
}

static void
RemoveClicked(gcc_unused WndButton &Sender)
{
  StaticString<256> tmp;
  tmp.Format(_("Do you want to remove station %s?"),
             station_iterator->GetCodeT());

  if (ShowMessageBox(tmp, _("Remove"), MB_YESNO) == IDNO)
    return;

  noaa_store->erase(station_iterator);
  noaa_store->SaveToProfile();

  wf->SetModalResult(mrOK);
}

static void
CloseClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(UpdateClicked),
  DeclareCallBackEntry(RemoveClicked),
  DeclareCallBackEntry(CloseClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgNOAADetailsShowModal(SingleWindow &parent, NOAAStore::iterator iterator)
{
  station_iterator = iterator;

  wf = LoadDialog(CallBackTable, parent, Layout::landscape ?
                  _T("IDR_XML_NOAA_DETAILS_L") : _T("IDR_XML_NOAA_DETAILS"));
  assert(wf != NULL);

  Update();

  wf->ShowModal();

  delete wf;
}

#else

#include "Dialogs/Message.hpp"

void
dlgNOAADetailsShowModal(SingleWindow &parent, unsigned station_index)
{
  ShowMessageBox(_("This function is not available on your platform yet."),
              _("Error"), MB_OK);
}
#endif
