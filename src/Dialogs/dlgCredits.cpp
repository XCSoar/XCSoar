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

  extern const uint8_t THIRD_PARTY_NOTICES_txt_gz[];
  extern const size_t THIRD_PARTY_NOTICES_txt_gz_size;
}

/**
 * Build the logo/about page as markdown text.
 * @param dark_title use light/white title art on dark dialog backgrounds
 */
static const char *
GetLogoText(bool dark_title) noexcept
{
  static StaticString<512> text;

  const char *const title_res =
    dark_title ? "IDB_TITLE_HD_WHITE" : "IDB_TITLE_HD";

#ifdef GIT_COMMIT_ID
  text.Format(
    "![XCSoar Logo](resource:IDB_LOGO_HD)\n\n"
    "![XCSoar](resource:%s)\n\n"
    "**Version %s**\n\n"
    "git: %s\n\n"
    "Visit us at:\n"
    "[https://xcsoar.org](https://xcsoar.org)",
    title_res,
    XCSoar_VersionString,
    GIT_COMMIT_ID);
#else
  text.Format(
    "![XCSoar Logo](resource:IDB_LOGO_HD)\n\n"
    "![XCSoar](resource:%s)\n\n"
    "**Version %s**\n\n"
    "Visit us at:\n"
    "[https://xcsoar.org](https://xcsoar.org)",
    title_res,
    XCSoar_VersionString);
#endif

  return text.c_str();
}

void
dlgCreditsShowModal([[maybe_unused]] UI::SingleWindow &parent)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  const auto authors = InflateToString(AUTHORS_gz, AUTHORS_gz_size);
  const auto news = InflateToString(NEWS_txt_gz, NEWS_txt_gz_size);
  const auto third_party = InflateToString(THIRD_PARTY_NOTICES_txt_gz,
                                           THIRD_PARTY_NOTICES_txt_gz_size);
  const auto license = InflateToString(COPYING_gz, COPYING_gz_size);

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Credits"));

  auto pager = std::make_unique<ArrowPagerWidget>(look.button,
                                                  dialog.MakeModalResultCallback(mrOK));
  ArrowPagerWidget *const pager_ptr = pager.get();

  auto add_scroll_page = [&](std::unique_ptr<RichTextWidget> &&content) {
    pager->Add(std::make_unique<VScrollWidget>(std::move(content), look, true));
  };

  add_scroll_page(std::make_unique<RichTextWidget>(
    look, GetLogoText(look.dark_mode)));
  add_scroll_page(std::make_unique<RichTextWidget>(look, authors.c_str()));
  add_scroll_page(std::make_unique<RichTextWidget>(look, news.c_str(), false));
  add_scroll_page(std::make_unique<RichTextWidget>(look, third_party.c_str()));
  add_scroll_page(std::make_unique<RichTextWidget>(look, license.c_str(), false));

  /* Caption update on page flip (order matches add_scroll_page calls). */
  static constexpr const char *const titles[] = {
    N_("About"),
    N_("Authors"),
    N_("News"),
    N_("License"),
  };
  static constexpr unsigned third_party_page_index = 3;
  static constexpr unsigned license_page_index = 4;
  const unsigned total_pages = pager->GetSize();

  auto update_caption = [&dialog, pager_ptr, total_pages]() {
    const unsigned current = pager_ptr->GetCurrentIndex();
    StaticString<128> caption;
    const char *page_title = nullptr;
    if (current < third_party_page_index)
      page_title = gettext(titles[current]);
    else if (current == third_party_page_index)
      page_title = "Third-party";
    else if (current == license_page_index)
      page_title = gettext(titles[3]);

    if (page_title != nullptr)
      caption.Format("%s (%u/%u)", page_title, current + 1, total_pages);
    else
      caption = _("Credits");
    dialog.SetCaption(caption);
  };

  pager->SetPageFlippedCallback(update_caption);
  update_caption();

  dialog.FinishPreliminary(std::move(pager));
  dialog.ShowModal();
}
