// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeatherOverlayDraft.hpp"

#include "Dialogs/InternalLink.hpp"
#include "Dialogs/Settings/Panels/PagesConfigPanel.hpp"
#include "PageActions.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/Edit.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Current.hpp"
#include "Profile/PageProfile.hpp"
#include "UIState.hpp"
#include "Weather/Features.hpp"
#include "Weather/MapOverlay/ControlsWidget.hpp"
#include "Weather/MapOverlay/PagePlacement.hpp"

namespace WeatherOverlayDraft {

void
OpenPagesConfig() noexcept
{
  ShowConfigPanel(_("Pages"), CreatePagesConfigPanel);
}

static bool
PageHasOverlay(unsigned page_index,
               PageLayout::Overlay overlay) noexcept
{
  const auto &settings = CommonInterface::GetUISettings().pages;
  if (page_index >= settings.n_pages)
    return false;

  const auto &page = settings.pages[page_index];
  return page.IsDefined() && page.IsMapMain() && page.overlay == overlay;
}

void
State::Load(PageLayout::Overlay overlay) noexcept
{
  const auto &settings = CommonInterface::GetUISettings().pages;
  const unsigned current =
    CommonInterface::GetUIState().pages.current_index;

  if (PageHasOverlay(current, overlay)) {
    draft = settings.pages[current];
    draft.Normalise();
    baseline = draft;
    return;
  }

  draft = PageLayout::Default();
  WeatherMapOverlay::ApplyWeatherOverlayToLayout(draft, overlay, -1);
  baseline = PageLayout::Default();
  baseline.overlay = PageLayout::Overlay::NONE;
}

bool
State::IsDirty() const noexcept
{
  if (draft.overlay != baseline.overlay)
    return true;

  switch (draft.overlay) {
  case PageLayout::Overlay::RASP:
    return draft.rasp_field != baseline.rasp_field ||
      draft.rasp_time != baseline.rasp_time;

  case PageLayout::Overlay::EDL:
    return draft.edl_time != baseline.edl_time ||
      draft.edl_isobar != baseline.edl_isobar;

  case PageLayout::Overlay::XCTHERM:
    return draft.xctherm_layer != baseline.xctherm_layer ||
      draft.xctherm_time != baseline.xctherm_time;

  case PageLayout::Overlay::NONE:
  case PageLayout::Overlay::MAX:
    return false;
  }

  return false;
}

void
State::SyncButtons(Button *apply_button, Button *add_button) const noexcept
{
  if (apply_button != nullptr)
    apply_button->SetEnabled(IsDirty());

  if (add_button != nullptr) {
    const auto &settings = CommonInterface::GetUISettings().pages;
    add_button->SetEnabled(settings.n_pages < PageSettings::MAX_PAGES);
  }
}

static bool
CommitDraft(const PageLayout &draft) noexcept
{
  if (draft.overlay == PageLayout::Overlay::NONE ||
      draft.overlay == PageLayout::Overlay::MAX)
    return false;

  auto &settings = CommonInterface::SetUISettings().pages;
  const unsigned page_index =
    CommonInterface::GetUIState().pages.current_index;
  const int rasp_field = draft.overlay == PageLayout::Overlay::RASP
    ? draft.rasp_field
    : -1;
  if (!WeatherMapOverlay::EnsureWeatherOverlayOnPage(settings, page_index,
                                                     draft.overlay,
                                                     rasp_field))
    return false;

  WeatherMapOverlay::CopyWeatherOverlayCursors(settings.pages[page_index],
                                               draft);
  Profile::Save(Profile::map, settings);
  PageActions::Update();
  return true;
}

bool
State::ApplyIfDirty() noexcept
{
  if (!IsDirty())
    return false;

  if (!CommitDraft(draft))
    return false;

  Load(draft.overlay);
  return true;
}

void
State::AddPage(Button *apply_button, Button *add_button) noexcept
{
  if (draft.overlay == PageLayout::Overlay::NONE ||
      draft.overlay == PageLayout::Overlay::MAX)
    return;

  auto &settings = CommonInterface::SetUISettings().pages;
  unsigned new_page_index = 0;
  const auto result = WeatherMapOverlay::AddWeatherOverlayPageFromDraft(
    settings, draft, new_page_index);
  if (result != WeatherMapOverlay::AddPageResult::SUCCESS)
    return;

  Profile::Save(Profile::map, settings);
  SyncButtons(apply_button, add_button);
}

void
SetAxisLabel(WndProperty &control, const char *label,
             bool enabled) noexcept
{
  auto &df = (DataFieldEnum &)*control.GetDataField();
  df.ClearChoices();
  df.AddChoice(0, label != nullptr ? label : "");
  df.SetValue(0U);
  control.SetEnabled(enabled);
  control.RefreshDisplay();
}

} // namespace WeatherOverlayDraft
