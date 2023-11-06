// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "io/Reader.hxx"
#include "io/BufferedReader.hxx"
#include "io/StringConverter.hpp"
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

  auto file_reader = OpenDataFile(_T(XCSCHKLIST));
  BufferedReader reader{*file_reader};
  StringConverter string_converter{Charset::UTF8};

  ChecklistPage page;

  char *TempString;
  while ((TempString = reader.ReadLine()) != nullptr) {
    const std::string_view line{TempString};

    // Look for start
    if (TempString[0] == '[') {
      if (!page.empty()) {
        c.emplace_back(std::move(page));
        page = {};
      }

      // extract name
      page.title.assign(string_converter.Convert(Split(line.substr(1), ']').first));
    } else if (!line.empty() || !page.text.empty()) {
      // append text to details string
      page.text.append(string_converter.Convert(line));
      page.text.push_back('\n');
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
