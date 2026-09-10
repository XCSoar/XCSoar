// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxArrange.hpp"
#include "InfoBoxArrangeWindow.hpp"
#include "InfoBoxLayout.hpp"
#include "InfoBoxManager.hpp"
#include "InfoBoxWindow.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Look/Look.hpp"
#include "UIGlobals.hpp"
#include "UISettings.hpp"
#include "ui/event/Timer.hpp"
#include "ui/window/SingleWindow.hpp"

#include <memory>
#include <optional>

namespace {

/** is the arrange mode active? */
bool active;

/** the InfoBox configuration as it was when the mode was entered */
InfoBoxSettings::Panel saved_panel;

/** leaves the arrange mode after the menu timeout */
std::optional<UI::Timer> timeout_timer;

/**
 * Restart the inactivity timeout; the arrange mode closes itself
 * after the same time as the main menu.
 */
void
RestartTimeout() noexcept
{
  if (!timeout_timer)
    timeout_timer.emplace([]{ InfoBoxArrange::Save(); });

  timeout_timer->Schedule(CommonInterface::GetUISettings().menu_timeout);
}

/** Stop the timeout while a dialog covers the arrange mode. */
void
CancelTimeout() noexcept
{
  if (timeout_timer)
    timeout_timer->Cancel();
}

/**
 * The arrange mode on the map: a window which covers the whole screen
 * while the InfoBox windows are hidden, so that dragging a card across
 * the map cannot pan or zoom it.
 */
class OverlayWindow final : public InfoBoxArrangeWindow {
public:
  explicit OverlayWindow(const InfoBoxLook &_look) noexcept
    :InfoBoxArrangeWindow(_look, UIGlobals::GetDialogLook(), Style::MAP) {
    AddButton(_("Help"), [this]{ ShowHelp(); });
    AddButton(_("Close"), []{ InfoBoxArrange::Save(); });
  }

protected:
  /* virtual methods from class InfoBoxArrangeWindow */
  void OnArrangeActivity() noexcept override {
    RestartTimeout();
  }

  void OnArrangeSuspend() noexcept override {
    CancelTimeout();
  }

  bool OnArrangeCancel() noexcept override {
    InfoBoxArrange::Cancel();
    return true;
  }
};

/**
 * The overlay window.  It is only hidden (and not destroyed) when the
 * mode ends, because that happens from its own event handler.
 */
std::unique_ptr<OverlayWindow> overlay;

/** Show the overlay and hide the InfoBox windows behind it. */
void
ShowControls() noexcept
{
  auto &parent = UIGlobals::GetMainWindow();

  if (overlay == nullptr)
    overlay = std::make_unique<OverlayWindow>(UIGlobals::GetLook().info_box);

  overlay->SetPanel(InfoBoxManager::GetCurrentPanel());
  overlay->SetLayout(InfoBoxManager::layout,
                     InfoBoxManager::layout.remaining);

  if (overlay->IsDefined())
    overlay->Move(parent.GetClientRect());
  else
    overlay->Create(parent, parent.GetClientRect());

  overlay->Show();
  overlay->BringToTop();

  /* the overlay paints the InfoBoxes itself */
  for (unsigned i = 0; i < InfoBoxManager::layout.count; ++i)
    if (auto *window = InfoBoxManager::GetWindow(i))
      window->FastHide();

  /* after hiding the InfoBoxes, so that a hidden one cannot keep the
     keyboard focus */
  overlay->SetFocus();
}

/** Common part of InfoBoxArrange::Save() and InfoBoxArrange::Cancel(). */
void
Leave() noexcept
{
  active = false;

  if (overlay != nullptr) {
    /* the finger may still rest on a card */
    overlay->Drop();
    overlay->FocusParent();
    overlay->Hide();
  }

  CancelTimeout();

  for (unsigned i = 0; i < InfoBoxManager::layout.count; ++i)
    if (auto *window = InfoBoxManager::GetWindow(i))
      window->Show();

  InfoBoxManager::Refresh();
  InfoBoxManager::ScheduleRedraw();
}

} // namespace

bool
InfoBoxArrange::IsActive() noexcept
{
  return active;
}

void
InfoBoxArrange::Begin(unsigned id, PixelPoint pointer) noexcept
{
  if (InfoBoxManager::GetWindow(id) == nullptr)
    return;

  if (!active) {
    active = true;
    saved_panel = InfoBoxManager::GetCurrentPanel();
    ShowControls();
  }

  /* the long press has already picked the InfoBox up, so it follows
     the finger right away */
  overlay->BeginDrag(id, pointer, true);
}

void
InfoBoxArrange::Begin() noexcept
{
  if (active || InfoBoxManager::GetWindow(0) == nullptr)
    return;

  active = true;
  saved_panel = InfoBoxManager::GetCurrentPanel();
  ShowControls();

  overlay->FocusSlot(0);
  RestartTimeout();
}

bool
InfoBoxArrange::SetFocus() noexcept
{
  if (!active || overlay == nullptr)
    return false;

  overlay->SetFocus();
  return true;
}

void
InfoBoxArrange::Save() noexcept
{
  if (!active)
    return;

  Leave();
  InfoBoxManager::SaveCurrentPanel();
}

void
InfoBoxArrange::Cancel() noexcept
{
  if (!active)
    return;

  InfoBoxManager::GetCurrentPanel() = saved_panel;
  Leave();
}

void
InfoBoxArrange::Reset() noexcept
{
  active = false;
  timeout_timer.reset();
  overlay.reset();
}
