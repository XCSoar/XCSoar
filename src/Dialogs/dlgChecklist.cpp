// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataFilePath.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/VScrollWidget.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/RichTextWidget.hpp"
#include "ui/control/RichTextWindow.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "util/StaticString.hxx"
#include "util/StringSplit.hxx"
#include "io/FileReader.hxx"
#include "io/Reader.hxx"
#include "io/BufferedReader.hxx"
#include "io/StringConverter.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "Repository/FileType.hpp"
#include "system/Path.hpp"
#include "Language/Language.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

struct ChecklistPage {
  std::string title, text;

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

/**
 * Commit @p page as a new page, or fold it into the last page when
 * the pager child limit is reached.
 */
static void
FlushPage(Checklist &c, ChecklistPage &page) noexcept
{
  if (page.empty())
    return;

  if (c.size() < MAX_CHECKLIST_PAGES) {
    c.push_back(std::move(page));
  } else if (!c.empty()) {
    if (!page.title.empty()) {
      c.back().text.append(page.title);
      c.back().text.push_back('\n');
    }
    c.back().text.append(page.text);
  }

  page = {};
}

static Checklist
LoadChecklist(Path path) noexcept
try {
  Checklist c;

  if (path == nullptr || path.empty())
    return c;

  FileReader file_reader(path);
  BufferedReader reader{file_reader};
  StringConverter conv{Charset::UTF8};

  ChecklistPage page;
  while (char *raw = reader.ReadLine()) {
    const std::string_view line{raw};

    if (raw[0] == '[') {
      FlushPage(c, page);

      if (c.size() < MAX_CHECKLIST_PAGES)
        page.title.assign(
          conv.Convert(Split(line.substr(1), ']').first));
      else if (!c.empty()) {
        /* Extra [section] headers stay visible on the last page. */
        c.back().text.append(conv.Convert(line));
        c.back().text.push_back('\n');
      }
    } else if (!line.empty() || !page.text.empty()) {
      std::string &dest = c.size() < MAX_CHECKLIST_PAGES
        ? page.text
        : c.back().text;
      dest.append(conv.Convert(line));
      dest.push_back('\n');
    }
  }

  FlushPage(c, page);
  return c;
} catch (...) {
  return {};
}

/**
 * Survives dialog close so the last page and checkbox ticks come back
 * on the next open.  Cleared when the Site Files checklist path changes.
 */
static struct {
  std::size_t page = 0;
  AllocatedPath path;
  std::vector<std::string> texts;
  std::vector<std::vector<uint8_t>> checks;
} session;

static RichTextWindow &
GetPageWindow(Widget &page) noexcept
{
  auto &widget = static_cast<RichTextWidget &>(
    static_cast<VScrollWidget &>(page).GetWidget());
  return static_cast<RichTextWindow &>(widget.GetWindow());
}

static void
RestoreSessionChecks(ArrowPagerWidget &pager, Path path,
                     const Checklist &checklist) noexcept
{
  if (path != session.path)
    return;

  const unsigned n = std::min(pager.GetSize(),
                              (unsigned)checklist.size());
  for (unsigned i = 0; i < n; ++i) {
    if (i >= session.texts.size() || i >= session.checks.size())
      break;
    if (session.texts[i] != checklist[i].text)
      continue;

    GetPageWindow(pager.GetWidget(i))
      .SetCheckboxCheckedStates(session.checks[i]);
  }
}

static void
SaveSession(ArrowPagerWidget &pager, Path path,
            const Checklist &checklist) noexcept
{
  session.page = pager.GetCurrentIndex();
  session.path = path;
  session.texts.clear();
  session.checks.clear();

  const unsigned n = std::min(pager.GetSize(),
                              (unsigned)checklist.size());
  session.texts.reserve(n);
  session.checks.reserve(n);
  for (unsigned i = 0; i < n; ++i) {
    session.texts.push_back(checklist[i].text);
    session.checks.push_back(
      GetPageWindow(pager.GetWidget(i)).GetCheckboxCheckedStates());
  }
}

void
dlgChecklistNotifySiteFileChanged() noexcept
{
  /* Profile::GetPath(ChecklistFile) is already updated; drop page and
     ticks so a different file does not keep stale state. */
  session = {};
}

void
dlgChecklistShowModal()
{

  auto path = Profile::GetPath(ProfileKeys::ChecklistFile);
  if (path == nullptr || path.empty())
    path = ResolveTypedDataFilePath(FileType::CHECKLIST,
                                    "xcsoar-checklist.txt");
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
        "- [%s](https://xcsoar.readthedocs.io/en/latest/checklist.html)\n\n"
        "%s **%s**.\n\n"
        "## %s\n\n"
        "- [ ] %s\n"
        "- [ ] %s\n"
        "- [ ] %s\n"
        "- [ ] %s\n"
        "- [ ] %s\n"
        "- [ ] %s [%s](vhf:122.800#standby)",
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
        _("Maps: geo:47.5,8.5"),
        _("Set standby from a link"),
        C_("Menu", "Information"));
      checklist.emplace_back(ChecklistPage{
          _("No checklist loaded"),
          body.c_str(),
        });
    }

  if (session.page >= checklist.size())
    session.page = 0;

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Checklist"));

  auto pager = std::make_unique<ArrowPagerWidget>(look.button,
                                                   dialog.MakeModalResultCallback(mrOK));
  ArrowPagerWidget *const pager_ptr = pager.get();

  for (const auto &page : checklist) {
    pager->Add(std::make_unique<VScrollWidget>(
      std::make_unique<RichTextWidget>(look, page.text.c_str()), look, true));
  }

  pager->SetCurrent(session.page);

  const std::size_t total_pages = checklist.size();

  pager->SetPageFlippedCallback(
    [&checklist, &dialog, pager_ptr, total_pages]() {
      UpdateCaption(dialog, checklist,
                    pager_ptr->GetCurrentIndex(), total_pages);
    });

  UpdateCaption(dialog, checklist,
                pager->GetCurrentIndex(), total_pages);

  dialog.FinishPreliminary(std::move(pager));
  /* Full-screen dialogs do not Prepare() until Show(); do it here so
     the page windows exist before ticks are restored. */
  dialog.PrepareWidget();

  auto &pager_widget = static_cast<ArrowPagerWidget &>(dialog.GetWidget());
  RestoreSessionChecks(pager_widget, path, checklist);

  dialog.ShowModal();

  SaveSession(pager_widget, path, checklist);
}
