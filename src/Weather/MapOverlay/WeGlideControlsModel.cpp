// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeGlideControlsModel.hpp"

#include "CursorBarLabels.hpp"
#include "Interface.hpp"
#include "PageSettings.hpp"
#include "UIState.hpp"
#include "Language/Language.hpp"
#include "Weather/WeGlide/PageOverlay.hpp"
#include "net/client/WeGlide/Tiles.hpp"
#include "time/BrokenDateTime.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace WeatherMapOverlay {

namespace {

PageLayout *
MutablePage() noexcept
{
  auto &pages = CommonInterface::SetUISettings().pages;
  const unsigned index = CommonInterface::GetUIState().pages.current_index;
  if (index >= pages.n_pages)
    return nullptr;
  return &pages.pages[index];
}

const PageLayout *
CurrentPage() noexcept
{
  const auto &pages = CommonInterface::GetUISettings().pages;
  const unsigned index = CommonInterface::GetUIState().pages.current_index;
  if (index >= pages.n_pages)
    return nullptr;
  return &pages.pages[index];
}

uint32_t
NowSeconds() noexcept
{
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  if (seconds <= 0)
    return 0;
  if (uint64_t(seconds) > 0xffffffffu)
    return 0xffffffffu;
  return uint32_t(seconds);
}

uint32_t
ShownFrame(const WeGlide::LayerTick &tick, int64_t stored,
           uint32_t now) noexcept
{
  if (stored <= PageLayout::WEGLIDE_TIME_AUTO ||
      uint64_t(stored) > 0xffffffffu)
    return WeGlide::CurrentTickTime(tick, now);
  return WeGlide::AlignTickTime(tick, uint32_t(stored), now);
}

} // namespace

void
WeGlideControlsModel::FormatPrimaryLabel(StaticString<64> &text) const noexcept
{
  const auto *tick = WeGlide::SelectedTick();
  if (tick == nullptr) {
    text = C_("Weather control", "Time");
    return;
  }

  const auto *page = CurrentPage();
  const int64_t stored = page != nullptr
    ? page->weglide_time : PageLayout::WEGLIDE_TIME_AUTO;
  const bool automatic = stored == PageLayout::WEGLIDE_TIME_AUTO;
  const uint32_t now = NowSeconds();
  const uint32_t requested = ShownFrame(*tick, stored, now);
  const uint32_t drawn = WeGlide::DrawnFrameTime();
  const uint32_t frame = drawn != 0 ? drawn : requested;
  const auto local = BrokenDateTime::FromUnixTime(frame).ToLocal();

  char offset[16];
  const int offset_min = int((int64_t(frame) - int64_t(now)) / 60);
  FormatSignedMinuteOffset(offset, sizeof(offset), offset_min);
  FormatAutoLocalTimeLabel(text, automatic, local.IsPlausible(),
                           local.hour, local.minute, offset);
}

void
WeGlideControlsModel::FormatSecondaryLabel(StaticString<64> &text)
  const noexcept
{
  const auto id = std::string{WeGlide::SelectedLayer()};
  for (const auto &layer : WeGlide::Catalog()) {
    if (layer.id == id) {
      text = layer.name.c_str();
      return;
    }
  }
  text = id.empty() ? "thermal" : id.c_str();
}

bool
WeGlideControlsModel::HasPrimaryData() const noexcept
{
  return WeGlide::SelectedTick() != nullptr;
}

bool
WeGlideControlsModel::IsPrimaryEnabled() const noexcept
{
  return WeGlide::SelectedTick() != nullptr;
}

bool
WeGlideControlsModel::HasSecondaryData() const noexcept
{
  return true;
}

bool
WeGlideControlsModel::StepPrimary(int delta) noexcept
{
  const auto *tick = WeGlide::SelectedTick();
  auto *page = MutablePage();
  if (tick == nullptr || page == nullptr || delta == 0)
    return false;

  const uint32_t now = NowSeconds();
  const uint32_t current = ShownFrame(*tick, page->weglide_time, now);
  const uint32_t next = WeGlide::StepTickTime(*tick, current, delta, now);
  const uint32_t live = WeGlide::CurrentTickTime(*tick, now);
  const int64_t stored = next >= live
    ? PageLayout::WEGLIDE_TIME_AUTO : int64_t(next);
  if (stored == page->weglide_time)
    return false;

  page->weglide_time = stored;
  WeGlide::SetFrameTime(stored);
  return true;
}

bool
WeGlideControlsModel::StepSecondary(int delta) noexcept
{
  std::vector<std::string> ids;
  for (const auto &layer : WeGlide::Catalog())
    ids.emplace_back(layer.id);
  if (ids.empty()) {
    ids.emplace_back("thermal");
    ids.emplace_back("vario");
  }

  const auto current = std::string{WeGlide::SelectedLayer()};
  std::size_t index = 0;
  for (std::size_t i = 0; i < ids.size(); ++i)
    if (ids[i] == current)
      index = i;

  int next = int(index) + delta;
  const int count = int(ids.size());
  next %= count;
  if (next < 0)
    next += count;

  const auto &id = ids[std::size_t(next)];
  auto &pages = CommonInterface::SetUISettings().pages;
  const unsigned page = CommonInterface::GetUIState().pages.current_index;
  if (page < pages.n_pages)
    pages.pages[page].weglide_layer = id.c_str();

  WeGlide::SelectLayer(id);
  return true;
}

bool
WeGlideControlsModel::GetPrimaryAutoAdvance() const noexcept
{
  const auto *page = CurrentPage();
  return page == nullptr ||
    page->weglide_time == PageLayout::WEGLIDE_TIME_AUTO;
}

void
WeGlideControlsModel::SetPrimaryAutoAdvance(bool auto_advance) noexcept
{
  auto *page = MutablePage();
  if (page == nullptr)
    return;

  if (auto_advance) {
    page->weglide_time = PageLayout::WEGLIDE_TIME_AUTO;
    WeGlide::SetFrameTime(PageLayout::WEGLIDE_TIME_AUTO);
    return;
  }

  if (page->weglide_time != PageLayout::WEGLIDE_TIME_AUTO)
    return;

  const auto *tick = WeGlide::SelectedTick();
  if (tick == nullptr)
    return;

  page->weglide_time = WeGlide::CurrentTickTime(*tick, NowSeconds());
  WeGlide::SetFrameTime(page->weglide_time);
}

void
WeGlideControlsModel::ApplyPrimaryAutoAdvance() noexcept
{
  const auto *page = CurrentPage();
  WeGlide::SetFrameTime(page != nullptr
                        ? page->weglide_time
                        : PageLayout::WEGLIDE_TIME_AUTO);
}

void
WeGlideControlsModel::ResumePrimaryAuto() noexcept
{
  EnablePrimaryAutoAndRefresh();
}

void
WeGlideControlsModel::OnGPSUpdate(const MoreData &) noexcept
{
  if (!GetPrimaryAutoAdvance())
    return;

  const auto *tick = WeGlide::SelectedTick();
  if (tick == nullptr)
    return;

  const uint32_t minute =
    WeGlide::CurrentTickTime(*tick, NowSeconds()) / 60;
  if (minute == shown_minute)
    return;

  shown_minute = minute;
  Notify(ControlsUpdate::LABELS);
}

void
WeGlideControlsModel::RefreshOverlay() noexcept
{
  WeGlide::SelectLayer(WeGlide::SelectedLayer());
}

} // namespace WeatherMapOverlay
