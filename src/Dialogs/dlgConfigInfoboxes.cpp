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

#include "Dialogs/dlgConfigInfoboxes.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Dialogs.h"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Profile/InfoBoxConfig.hpp"
#include "Screen/Layout.hpp"
#include "DataField/Enum.hpp"
#include "Compiler.h"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/Content/Factory.hpp"
#include "Compiler.h"

#include <assert.h>
#include <cstdio>
#include <algorithm>

static InfoBoxPanelConfig data;
static WndForm *wf = NULL;
static InfoBoxPanelConfig clipboard;
static unsigned clipboard_size;
static WndButton *buttonPaste;

static void
RefreshPasteButton()
{
  buttonPaste->set_enabled(clipboard_size > 0);
}

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

/**
 * @return true if the #InfoBoxPanelConfig has been modified
 */
static bool
FormToPanelConfig(InfoBoxPanelConfig &config)
{
  bool changed = false;

  for (unsigned item = 0; item < InfoBoxManager::layout.count; item++) {
    WndProperty *wp = FindInfoBoxField(item);
    if (wp == NULL)
      continue;

    unsigned new_value = wp->GetDataField()->GetAsInteger();
    if (new_value == config.infoBoxID[item])
      continue;

    config.infoBoxID[item] = new_value;
    changed = true;
  }

  return changed;
}

static void
OnCopy(gcc_unused WndButton &button)
{
  FormToPanelConfig(clipboard);
  clipboard_size = InfoBoxManager::layout.count;
  RefreshPasteButton();
}

static void
OnPaste(gcc_unused WndButton &button)
{
  if (clipboard_size == 0)
    return;

  if(MessageBoxX(_("Overwrite?"), _("InfoBox paste"),
                 MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  unsigned count = std::min(InfoBoxManager::layout.count, clipboard_size);
  for (unsigned item = 0; item < count; item++) {
    unsigned content = clipboard.infoBoxID[item];
    if (content >= InfoBoxFactory::NUM_TYPES)
      continue;

    WndProperty *wp = FindInfoBoxField(item);
    if (wp != NULL) {
      DataFieldEnum *dfe = (DataFieldEnum *)wp->GetDataField();
      dfe->Set(content);
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

  const TCHAR *text = InfoBoxFactory::GetDescription(type);
  if (text == NULL)
    text = _("No help available on this item");
  else
    text = gettext(text);

  dlgHelpShowModal(wf->GetMainWindow(), caption, text);
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

  dfe->Set(data.infoBoxID[item]);
  wp->RefreshDisplay();
}

bool
dlgConfigInfoboxesShowModal(SingleWindow &parent, const TCHAR *panel_name,
                            InfoBoxPanelConfig &data_r)
{
  data = data_r;

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ? _T("IDR_XML_CONFIG_INFOBOXES_L") :
                                      _T("IDR_XML_CONFIG_INFOBOXES"));
  if (wf == NULL)
    return false;

  TCHAR caption[100];
  _tcscpy(caption, wf->GetCaption());
  _tcscat(caption, _T(": "));
  _tcscat(caption, panel_name);
  wf->SetCaption(caption);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  WndButton *buttonCopy = ((WndButton *)wf->FindByName(_T("cmdCopy")));
  if (buttonCopy)
    buttonCopy->SetOnClickNotify(OnCopy);

  buttonPaste = ((WndButton *)wf->FindByName(_T("cmdPaste")));
  if (buttonPaste)
    buttonPaste->SetOnClickNotify(OnPaste);

  RefreshPasteButton();

  for (unsigned j = 0; j < InfoBoxManager::layout.count; j++)
    SetInfoBoxSelector(j);

  int result = wf->ShowModal();
  if (result != mrOK || !FormToPanelConfig(data)) {
    delete wf;
    return false;
  }

  delete wf;

  data.modified = true;
  data_r = data;
  return true;
}
