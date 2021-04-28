/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "util/StaticString.hxx"
#include "util/StringCompare.hxx"
#include "io/DataFile.hpp"
#include "io/LineReader.hpp"
#include "Language/Language.hpp"

#define XCSCHKLIST  "xcsoar-checklist.txt"

#define MAXTITLE 200
#define MAXDETAILS 5000

#define MAXLISTS 20
static int nLists = 0;
static TCHAR *ChecklistText[MAXTITLE];
static TCHAR *ChecklistTitle[MAXTITLE];

static void
UpdateCaption(WndForm &form, unsigned page)
{
  TCHAR buffer[80];
  _tcscpy(buffer, _("Checklist"));

  if (ChecklistTitle[page] &&
      !StringIsEmpty(ChecklistTitle[page]) &&
      _tcslen(ChecklistTitle[page]) < 60) {
    _tcscat(buffer, _T(": "));
    _tcscat(buffer, ChecklistTitle[page]);
  }

  form.SetCaption(buffer);
}

static void
addChecklist(const TCHAR *name, const TCHAR *details)
{
  if (nLists >= MAXLISTS)
    return;

  ChecklistTitle[nLists] = _tcsdup(name);
  ChecklistText[nLists] = _tcsdup(details);
  nLists++;
}

static void
LoadChecklist()
try {
  nLists = 0;

  free(ChecklistText[0]);
  ChecklistText[0] = NULL;

  free(ChecklistTitle[0]);
  ChecklistTitle[0] = NULL;

  auto reader = OpenDataTextFile(_T(XCSCHKLIST));

  StaticString<MAXDETAILS> Details;
  TCHAR Name[MAXTITLE];
  bool inDetails = false;
  int i;

  Details.clear();
  Name[0] = 0;

  TCHAR *TempString;
  while ((TempString = reader->ReadLine()) != NULL) {
    // Look for start
    if (TempString[0] == '[') {
      if (inDetails) {
        addChecklist(Name, Details);
        Details.clear();
        Name[0] = 0;
      }

      // extract name
      for (i = 1; i < MAXTITLE; i++) {
        if (TempString[i] == ']')
          break;

        Name[i - 1] = TempString[i];
      }
      Name[i - 1] = 0;

      inDetails = true;
    } else {
      // append text to details string
      Details.append(TempString);
      Details.push_back(_T('\n'));
    }
  }

  if (inDetails) {
    addChecklist(Name, Details);
  }
} catch (...) {
}

void
dlgChecklistShowModal()
{
  static unsigned int current_page = 0;
  static bool first = true;
  if (first) {
    LoadChecklist();
    if (nLists == 0)
      addChecklist(_("No checklist loaded"), _("Create xcsoar-checklist.txt"));
    first = false;
  }

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Checklist"));

  auto pager = std::make_unique<ArrowPagerWidget>(look.button,
                                                   dialog.MakeModalResultCallback(mrOK));
  for (int i = 0; i < nLists; ++i)
    pager->Add(std::make_unique<LargeTextWidget>(look, ChecklistText[i]));
  pager->SetCurrent(current_page);
  pager->SetPageFlippedCallback([&dialog, &pager=*pager](){
    UpdateCaption(dialog, pager.GetCurrentIndex());
  });

  UpdateCaption(dialog, pager->GetCurrentIndex());

  dialog.FinishPreliminary(std::move(pager));
  dialog.ShowModal();

  current_page = ((ArrowPagerWidget &)dialog.GetWidget()).GetCurrentIndex();
}
