// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/RichTextWidget.hpp"
#include "Widget/VScrollWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Version.hpp"
#include "Inflate.hpp"
#include "util/ConvertString.hpp"
#include "util/AllocatedString.hxx"
#include "util/StaticString.hxx"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"

extern "C"
{
  extern const uint8_t COPYING_gz[];
  extern const size_t COPYING_gz_size;

  extern const uint8_t NEWS_txt_gz[];
  extern const size_t NEWS_txt_gz_size;

  extern const uint8_t AUTHORS_gz[];
  extern const size_t AUTHORS_gz_size;
}

/**
 * Build the logo/about page as markdown text.
 */
static const char *
GetLogoText() noexcept
{
  static StaticString<512> text;

#ifdef GIT_COMMIT_ID
  text.Format(
    "![XCSoar Logo](resource:IDB_LOGO_HD)\n\n"
    "![XCSoar](resource:IDB_TITLE_HD)\n\n"
    "**Version %s**\n\n"
    "git: %s\n\n"
    "Visit us at:\n"
    "[https://xcsoar.org](https://xcsoar.org)",
    XCSoar_VersionString,
    GIT_COMMIT_ID);
#else
  text.Format(
    "![XCSoar Logo](resource:IDB_LOGO_HD)\n\n"
    "![XCSoar](resource:IDB_TITLE_HD)\n\n"
    "**Version %s**\n\n"
    "Visit us at:\n"
    "[https://xcsoar.org](https://xcsoar.org)",
    XCSoar_VersionString);
#endif

  return text.c_str();
}

void
dlgCreditsShowModal([[maybe_unused]] UI::SingleWindow &parent)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  const auto authors = InflateToString(AUTHORS_gz, AUTHORS_gz_size);
  const UTF8ToWideConverter authors2(authors.c_str());

  const auto news = InflateToString(NEWS_txt_gz, NEWS_txt_gz_size);
  const UTF8ToWideConverter news2(news.c_str());

  const auto license = InflateToString(COPYING_gz, COPYING_gz_size);
  const UTF8ToWideConverter license2(license.c_str());

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Credits"));

  auto pager = std::make_unique<ArrowPagerWidget>(look.button,
                                                  dialog.MakeModalResultCallback(mrOK));
  ArrowPagerWidget *pager_ptr = pager.get();

  pager->Add(std::make_unique<VScrollWidget>(
    std::make_unique<RichTextWidget>(look, GetLogoText()), look, true));
  pager->Add(std::make_unique<VScrollWidget>(
    std::make_unique<RichTextWidget>(look, authors2), look, true));
  pager->Add(std::make_unique<VScrollWidget>(
    std::make_unique<RichTextWidget>(look, news2, false), look, true));
  pager->Add(std::make_unique<VScrollWidget>(
    std::make_unique<RichTextWidget>(look, license2, false), look, true));

  /* Caption update on page flip */
  static constexpr const char *const titles[] = {
    N_("About"),
    N_("Authors"),
    N_("News"),
    N_("License"),
  };
  const unsigned total_pages = pager->GetSize();

  auto update_caption = [&dialog, pager_ptr, total_pages]() {
    const unsigned current = pager_ptr->GetCurrentIndex();
    StaticString<128> caption;
    if (current < std::size(titles))
      caption.Format("%s (%u/%u)",
                     gettext(titles[current]),
                     current + 1, total_pages);
    else
      caption = _("Credits");
    dialog.SetCaption(caption);
  };

  pager->SetPageFlippedCallback(update_caption);
  update_caption();

  dialog.FinishPreliminary(std::move(pager));
  dialog.ShowModal();
}
