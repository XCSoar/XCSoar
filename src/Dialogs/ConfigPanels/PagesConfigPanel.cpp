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

#include "PagesConfigPanel.hpp"
#include "DataField/Enum.hpp"
#include "Pages.hpp"
#include "Language/Language.hpp"
#include "Profile/PageProfile.hpp"
#include "Interface.hpp"
#include "Form/RowFormWidget.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"

#include <stdio.h>

using namespace Pages;


class PagesConfigPanel : public RowFormWidget {
public:
  PagesConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook(), Layout::Scale(50)) {}

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
};

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
  static const StaticEnumChoice  empty_list[] = { { 0 }  };

  RowFormWidget::Prepare(parent, rc);

  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; i++) {
    StaticString<32> caption;
    caption.Format(_T("%s %u"), _("Page"), i+1);
    WndProperty* wp = AddEnum(caption,
                             _("Determines the information displayed on different screen pages."),
                             empty_list, 0);
    if (wp) {
      DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
      UpdateComboBox(dfe, i);
// TODO     if (i>2 && !Expert) {
//        wp->set_visible(false);
//      }
      wp->RefreshDisplay();
    }
  }
}

bool
PagesConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;

  PageSettings &settings = CommonInterface::SetUISettings().pages;
  for (unsigned int i = 0; i < PageSettings::MAX_PAGES; i++) {
    unsigned int tmp_page_selection;
    if (SaveValueEnum(i, tmp_page_selection)) {
      PageSettings::PageLayout *currentPL = &settings.pages[i];
      const PageSettings::PageLayout *setPL =
        PossiblePageLayout(tmp_page_selection);
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
