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
#include "util/StringSplit.hxx"
#include "util/StringCompare.hxx"
#include "util/tstring.hpp"
#include "io/DataFile.hpp"
#include "io/LineReader.hpp"
#include "Language/Language.hpp"

#include <string>
#include <vector>

#define XCSCHKLIST  "xcsoar-checklist.txt"

struct ChecklistPage {
  tstring title, text;

  bool empty() const noexcept {
    return title.empty() && text.empty();
  }
};

using Checklist = std::vector<ChecklistPage>;

static void
UpdateCaption(WndForm &form, const Checklist &checklist, std::size_t page)
{
  StaticString<80> buffer{_("Checklist")};

  const auto &p = checklist[page];

  if (!p.title.empty()) {
    buffer.append(_T(": "));
    buffer.append(p.title);
  }

  form.SetCaption(buffer);
}

static Checklist
LoadChecklist() noexcept
try {
  Checklist c;

  auto reader = OpenDataTextFile(_T(XCSCHKLIST));

  ChecklistPage page;

  TCHAR *TempString;
  while ((TempString = reader->ReadLine()) != NULL) {
    const tstring_view line{TempString};

    // Look for start
    if (TempString[0] == '[') {
      if (!page.empty()) {
        c.emplace_back(std::move(page));
        page = {};
      }

      // extract name
      page.title = tstring{Split(line.substr(1), _T(']')).first};
    } else if (!line.empty() || !page.text.empty()) {
      // append text to details string
      page.text.append(line);
      page.text.push_back(_T('\n'));
    }
  }

  if (!page.empty())
    c.emplace_back(std::move(page));

  return c;
} catch (...) {
  return {};
}

void
dlgChecklistShowModal()
{
  static std::size_t current_page = 0;

  auto checklist = LoadChecklist();
  if (checklist.empty())
    checklist.emplace_back(ChecklistPage{
        _("No checklist loaded"),
        _("Create xcsoar-checklist.txt"),
      });

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Checklist"));

  auto pager = std::make_unique<ArrowPagerWidget>(look.button,
                                                   dialog.MakeModalResultCallback(mrOK));
  for (const auto &i : checklist)
    pager->Add(std::make_unique<LargeTextWidget>(look, i.text.c_str()));

  if (current_page < checklist.size())
    pager->SetCurrent(current_page);

  pager->SetPageFlippedCallback([&checklist, &dialog, &pager=*pager](){
    UpdateCaption(dialog, checklist, pager.GetCurrentIndex());
  });

  UpdateCaption(dialog, checklist, pager->GetCurrentIndex());

  dialog.FinishPreliminary(std::move(pager));
  dialog.ShowModal();

  current_page = ((ArrowPagerWidget &)dialog.GetWidget()).GetCurrentIndex();
}
