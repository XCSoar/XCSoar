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

#include "Dialogs/Internal.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Profile/InfoBoxConfig.hpp"
#include "Screen/Layout.hpp"
#include "MainWindow.hpp"
#include "DataField/Enum.hpp"
#include "Compiler.h"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/Content/Factory.hpp"
#include "LogFile.hpp"

#include <assert.h>
#include <cstdio>

static bool changed = false;
static WndForm *wf = NULL;
static WndButton *buttonCopy = NULL;
static WndButton *buttonPaste = NULL;
static int cpyInfoBox[10];
static unsigned panel;

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static WndProperty *
FindInfoBoxField(int item)
{
  TCHAR name[80];
  _stprintf(name, _T("prpInfoBox%1d"), item);
  return (WndProperty*)wf->FindByName(name);
}

static void
OnCopy(gcc_unused WndButton &button)
{
  for (unsigned item = 0; item < InfoBoxManager::layout.count; item++) {
    WndProperty *wp = FindInfoBoxField(item);
    if (wp)
      cpyInfoBox[item] = wp->GetDataField()->GetAsInteger();
  }
}

static void
OnPaste(gcc_unused WndButton &button)
{
  if(MessageBoxX(_("Overwrite?"), _("InfoBox paste"),
                 MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  for (unsigned item = 0; item < InfoBoxManager::layout.count; item++) {
    WndProperty *wp = FindInfoBoxField(item);
    if (wp &&
        cpyInfoBox[item] >= 0 &&
        (unsigned)cpyInfoBox[item] < InfoBoxFactory::NUM_TYPES) {
      DataFieldEnum *dfe = (DataFieldEnum *)wp->GetDataField();
      dfe->Set(cpyInfoBox[item]);
      wp->RefreshDisplay();
    }
  }
}

static void
OnInfoBoxHelp(WindowControl * Sender)
{
  WndProperty *wp = (WndProperty*)Sender;
  unsigned type = wp->GetDataField()->GetAsInteger();
  if (type >= InfoBoxFactory::NUM_TYPES)
    return;

  const TCHAR *name = InfoBoxFactory::GetName(type);
  if (name == NULL)
    return;

  TCHAR caption[100];
  _stprintf(caption, _T("%s: %s"), _("InfoBox"), gettext(name));

  const TCHAR* text = InfoBoxFactory::GetDescription(type);
  if (text)
    dlgHelpShowModal(XCSoarInterface::main_window, caption, gettext(text));
  else
    dlgHelpShowModal(XCSoarInterface::main_window, caption,
                     _("No help available on this item"));
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnInfoBoxHelp),
  DeclareCallBackEntry(NULL)
};

static void
SetInfoBoxSelector(unsigned item)
{
  WndProperty *wp = FindInfoBoxField(item);
  if (wp == NULL)
    return;

  DataFieldEnum* dfe;
  dfe = (DataFieldEnum*)wp->GetDataField();
  for (unsigned i = 0; i < InfoBoxFactory::NUM_TYPES; i++)
    dfe->addEnumText(gettext(InfoBoxFactory::GetName(i)));

  dfe->Sort(0);

  dfe->Set(InfoBoxManager::GetType(item, panel));
  wp->RefreshDisplay();
}

static void
GetInfoBoxSelector(unsigned item)
{
  WndProperty *wp = FindInfoBoxField(item);
  if (wp == NULL)
    return;

  int itnew = wp->GetDataField()->GetAsInteger();
  int it = InfoBoxManager::GetType(item, panel);

  if (it == itnew)
    return;

  changed = true;
  InfoBoxManager::SetType(item, itnew, panel);
  Profile::SetInfoBoxManagerConfig(infoBoxManagerConfig);
}

void dlgConfigInfoboxesShowModal(unsigned _panel)
{
  panel = _panel;

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                  Layout::landscape ? _T("IDR_XML_CONFIG_INFOBOXES_L") :
                                      _T("IDR_XML_CONFIG_INFOBOXES"));
  if (wf == NULL)
    return;

  TCHAR caption[100];
  _tcscpy(caption, wf->GetCaption());
  _tcscat(caption, _T(": "));
  _tcscat(caption, InfoBoxManager::GetPanelName(panel));
  wf->SetCaption(caption);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  static bool first = true;
  if (first) {
    first = false;

    for (int item = 0; item < 10; item++)
      cpyInfoBox[item] = -1;
  }

  buttonCopy = ((WndButton *)wf->FindByName(_T("cmdCopy")));
  if (buttonCopy)
    buttonCopy->SetOnClickNotify(OnCopy);

  buttonPaste = ((WndButton *)wf->FindByName(_T("cmdPaste")));
  if (buttonPaste)
    buttonPaste->SetOnClickNotify(OnPaste);

  for (unsigned j = 0; j < InfoBoxManager::layout.count; j++)
    SetInfoBoxSelector(j);

  changed = false;

  wf->ShowModal();

  for (unsigned j = 0; j < InfoBoxManager::layout.count; ++j)
    GetInfoBoxSelector(j);

  if (changed) {
    Profile::Save();
    LogDebug(_T("InfoBox configuration: Changes saved"));
  }

  delete wf;
}
