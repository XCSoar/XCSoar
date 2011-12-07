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

#include <stdio.h>
#include "PagesConfigPanel.hpp"
#include "DataField/Enum.hpp"
#include "Form/Edit.hpp"
#include "Pages.hpp"
#include "Profile/PageProfile.hpp"
#include "Interface.hpp"
#include "Form/Form.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/dlgTools.h"
#include "Dialogs/XML.hpp"

using namespace Pages;


class PagesConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
};

void
PagesConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
PagesConfigPanel::Hide()
{
  XMLWidget::Hide();
}

static void
UpdateComboBox(DataFieldEnum* dfe, unsigned page)
{
  TCHAR buffer[128];
  const PageSettings::PageLayout *pl;
  const PageSettings::PageLayout *currentPL =
    &CommonInterface::SetUISettings().pages.pages[page];
  assert(currentPL != NULL);
  currentPL->MakeTitle(buffer);
  // dont offer "None" for first page
  unsigned i = (page == 0) ? 1 : 0;
  while (NULL != (pl = PossiblePageLayout(i))) {
    pl->MakeTitle(buffer);
    dfe->addEnumText(buffer, i);
    if (*pl == *currentPL)
      dfe->Set(i);
    i++;
  }
}

void
PagesConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  LoadWindow(NULL, parent,
             Layout::landscape ? _T("IDR_XML_PAGESCONFIGPANEL") :
                               _T("IDR_XML_PAGESCONFIGPANEL_L"));

  StaticString<64> prpName;
  for (unsigned i = 0; i < PageSettings::MAX_PAGES; i++) {
    prpName.Format(_T("prpPageLayout%u"), i);
    WndProperty* wp = (WndProperty*)form.FindByName(prpName);
    if (wp) {
      DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
      UpdateComboBox(dfe, i);
      wp->RefreshDisplay();
    }
  }
}

bool
PagesConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;
  StaticString<64> prpName;

  PageSettings &settings = CommonInterface::SetUISettings().pages;
  for (unsigned int i = 0; i < PageSettings::MAX_PAGES; i++) {
    prpName.Format(_T("prpPageLayout%u"), i);
    WndProperty* wp = (WndProperty*)form.FindByName(prpName);
    if (wp) {
      PageSettings::PageLayout *currentPL = &settings.pages[i];
      const PageSettings::PageLayout *setPL =
        PossiblePageLayout(wp->GetDataField()->GetAsInteger());
      if (! (*currentPL == *setPL)) {
        SetLayout(i, *setPL);

        *currentPL = *setPL;
        Profile::Save(*currentPL, i);

        changed = true;
      }
    }
  }
  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreatePagesConfigPanel()
{
  return new PagesConfigPanel();
}
