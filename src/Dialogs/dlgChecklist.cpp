// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/VScrollWidget.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/RichTextWidget.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "util/StaticString.hxx"
#include "util/StringSplit.hxx"
#include "util/tstring.hpp"
#include "io/FileReader.hxx"
#include "io/Reader.hxx"
#include "io/BufferedReader.hxx"
#include "io/StringConverter.hpp"
#include "LocalPath.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "system/Path.hpp"
#include "Language/Language.hpp"

#include <string>
#include <vector>

struct ChecklistPage {
  tstring title, text;

  bool empty() const noexcept {
    return title.empty() && text.empty();
  }
};

using Checklist = std::vector<ChecklistPage>;

/** Maximum checklist pages; must not exceed PagerWidget's child limit (32). */
static constexpr std::size_t MAX_CHECKLIST_PAGES = 32;

static void
UpdateCaption(WndForm &form, const Checklist &checklist,
              std::size_t page, std::size_t total)
{
  if (page >= checklist.size())
    return;

  const auto &p = checklist[page];
  StaticString<128> caption;

  if (!p.title.empty())
    caption.Format("%s (%u/%u)",
                   p.title.c_str(),
                   (unsigned)(page + 1), (unsigned)total);
  else
    caption.Format("%s (%u/%u)",
                   (const char *)_("Checklist"),
                   (unsigned)(page + 1), (unsigned)total);

  form.SetCaption(caption);
}

static Checklist
LoadChecklist(Path path) noexcept
try {
  Checklist c;

  if (path == nullptr || path.empty())
    return c;

  FileReader file_reader(path);
  BufferedReader reader{file_reader};
  StringConverter string_converter{Charset::UTF8};

  ChecklistPage page;

  char *TempString;
  while ((TempString = reader.ReadLine()) != nullptr) {
    const std::string_view line{TempString};

    // Look for start of new page
    if (TempString[0] == '[') {
      if (!page.empty()) {
        if (c.size() < MAX_CHECKLIST_PAGES) {
          c.emplace_back(std::move(page));
        } else if (!c.empty()) {
          c.back().text.append(page.title);
          c.back().text.append("\n");
          c.back().text.append(page.text);
        }
        page = {};
      }

      if (c.size() < MAX_CHECKLIST_PAGES) {
        page.title.assign(string_converter.Convert(Split(line.substr(1), ']').first));
      } else if (!c.empty()) {
        // Already at page limit; append this line to last page instead
        c.back().text.append(string_converter.Convert(line));
        c.back().text.push_back('\n');
      }
    } else if (!line.empty() || !page.text.empty()) {
      // append text to details string
      if (c.size() < MAX_CHECKLIST_PAGES) {
        page.text.append(string_converter.Convert(line));
        page.text.push_back('\n');
      } else if (!c.empty()) {
        c.back().text.append(string_converter.Convert(line));
        c.back().text.push_back('\n');
      }
    }
  }

  if (!page.empty()) {
    if (c.size() < MAX_CHECKLIST_PAGES) {
      c.emplace_back(std::move(page));
    } else if (!c.empty()) {
      // At page limit; append final page content to last page
      if (!page.title.empty()) {
        c.back().text.append(page.title);
        c.back().text.append("\n");
      }
      c.back().text.append(page.text);
    }
  }

  return c;
} catch (...) {
  return {};
}

void
dlgChecklistShowModal()
{
  static std::size_t current_page = 0;

  auto path = Profile::GetPath(ProfileKeys::ChecklistFile);
  if (path == nullptr || path.empty())
    path = LocalPath("xcsoar-checklist.txt");
  auto checklist = LoadChecklist(path);
  if (checklist.empty())
    {
      /* Build the getting-started page by assembling translated
         display text with untranslated Markdown syntax so that
         translators never need to touch formatting characters. */
      StaticString<1024> body;
      body.Format(
        "# %s\n\n"
        "%s\n\n"
        "- [%s](https://xcsoar.org/download/data/xcsoar-checklist.txt)\n"
        "- [%s](https://xcsoar.org/develop/checklist.html)\n\n"
        "%s **%s**.\n\n"
        "## %s\n\n"
        "- [ ] %s\n"
        "- [ ] %s\n"
        "- [ ] %s\n"
        "- [ ] %s\n"
        "- [ ] %s",
        _("Getting Started"),
        _("Download the example checklist or create your own:"),
        _("Download Example"),
        _("View Documentation"),
        _("Then select it in"),
        _("Site Files > Checklist"),
        _("Features"),
        _("Interactive checkboxes"),
        N_("**Bold** and # Headings"),
        _("Clickable links"),
        _("Phone: tel: / SMS: sms: / Email: mailto:"),
        _("Maps: geo:47.5,8.5"));
      checklist.emplace_back(ChecklistPage{
          _("No checklist loaded"),
          body.c_str(),
        });
    }

  if (current_page >= checklist.size())
    current_page = 0;

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Checklist"));

  auto pager = std::make_unique<ArrowPagerWidget>(look.button,
                                                   dialog.MakeModalResultCallback(mrOK));
  for (const auto &i : checklist)
    pager->Add(std::make_unique<VScrollWidget>(
      std::make_unique<RichTextWidget>(look, i.text.c_str()), look, true));

  pager->SetCurrent(current_page);

  const std::size_t total_pages = checklist.size();

  pager->SetPageFlippedCallback(
    [&checklist, &dialog, &pager=*pager, total_pages](){
      UpdateCaption(dialog, checklist,
                    pager.GetCurrentIndex(), total_pages);
    });

  UpdateCaption(dialog, checklist,
                pager->GetCurrentIndex(), total_pages);

  dialog.FinishPreliminary(std::move(pager));
  dialog.ShowModal();

  current_page = ((ArrowPagerWidget &)dialog.GetWidget()).GetCurrentIndex();
}
