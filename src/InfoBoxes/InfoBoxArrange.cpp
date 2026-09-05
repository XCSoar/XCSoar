// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxArrange.hpp"
#include "InfoBoxArrangeWindow.hpp"
#include "InfoBoxLayout.hpp"
#include "InfoBoxManager.hpp"
#include "InfoBoxWindow.hpp"
#include "Asset.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "Look/Look.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "UIState.hpp"
#include "ui/event/Timer.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "ui/window/SingleWindow.hpp"

#include <memory>

using namespace UI;

namespace {

/** the InfoBox configuration as it was when the mode was entered */
InfoBoxSettings::Panel saved_panel;
unsigned saved_panel_index;

/**
 * Full-screen overlay: cards plus a real Help/Close #ButtonPanel.
 * Hidden (not destroyed) when the mode ends, because that happens
 * from its own event handler.  The inactivity timeout is the same as
 * the main menu.
 */
class OverlayWindow final : public ContainerWindow {
  class ArrangeWindow final : public InfoBoxArrangeWindow {
    OverlayWindow &overlay;

  public:
    explicit ArrangeWindow(OverlayWindow &_overlay) noexcept
      :InfoBoxArrangeWindow(UIGlobals::GetLook().info_box,
                            UIGlobals::GetDialogLook(),
                            Style::MAP),
       overlay(_overlay) {}

  protected:
    void OnArrangeActivity() noexcept override {
      overlay.RestartTimeout();
    }

    void OnArrangeSuspend() noexcept override {
      overlay.timeout_timer.Cancel();
    }

    bool OnArrangeCancel() noexcept override {
      InfoBoxArrange::Cancel();
      return true;
    }
  };

  ArrangeWindow arrange;
  ButtonPanel buttons;
  UI::Timer timeout_timer{[]{ InfoBoxArrange::Save(); }};

  static void HideInfoBoxes() noexcept {
    for (unsigned i = 0; i < InfoBoxManager::layout.count; ++i)
      if (auto *window = InfoBoxManager::GetWindow(i))
        window->FastHide();
  }

  static void ShowInfoBoxes() noexcept {
    for (unsigned i = 0; i < InfoBoxManager::layout.count; ++i)
      if (auto *window = InfoBoxManager::GetWindow(i))
        window->Show();
  }

public:
  OverlayWindow() noexcept
    :arrange(*this),
     buttons(*this, UIGlobals::GetDialogLook().button) {}

  InfoBoxArrangeWindow &GetArrange() noexcept {
    return arrange;
  }

  void RestartTimeout() noexcept {
    timeout_timer.Schedule(CommonInterface::GetUISettings().menu_timeout);
  }

  void Create(SingleWindow &parent) noexcept {
    WindowStyle style;
    style.Hide();
    style.ControlParent();
    ContainerWindow::Create(parent, parent.GetClientRect(), style);

#ifndef USE_WINUSER
    /* the map below must still be painted */
    SetTransparent();
#endif

    arrange.Create(*this, GetClientRect());
    buttons.Add(_("Help"), [this]{ arrange.ShowHelp(); });
    buttons.Add(_("Close"), []{ InfoBoxArrange::Save(); });
  }

  void UpdateLayout() noexcept {
    arrange.Move(GetClientRect());

    /* Help/Close sit in the map remaining, above a bottom InfoBox
       row; left vs bottom follows the page, not the hole */
    const auto origin = GetPosition().GetTopLeft();
    PixelRect remaining = InfoBoxManager::layout.remaining;
    remaining.Offset(-origin.x, -origin.y);

    const PixelRect full = GetClientRect();
    PixelRect content = full.GetWidth() > full.GetHeight()
      ? buttons.LeftLayout(remaining)
      : buttons.BottomLayout(remaining);
    content.Offset(origin.x, origin.y);

    arrange.SetLayout(InfoBoxManager::layout, content);
    /* the card window fills the overlay; keep Help/Close above it */
    buttons.Raise();
  }

  /** Show the overlay and hide the InfoBox windows behind it. */
  void Enter() noexcept {
    auto &parent = UIGlobals::GetMainWindow();
    if (IsDefined())
      Move(parent.GetClientRect());
    else
      Create(parent);

    arrange.SetPanel(InfoBoxManager::GetPanel(saved_panel_index));
    UpdateLayout();
    /* Create() hides the cards, as the settings dialog does; that
       dialog shows them from Widget::Show(), which this overlay
       never has */
    arrange.Show();
    Show();
    BringToTop();

    HideInfoBoxes();
    /* after hiding the InfoBoxes, so that a hidden one cannot keep
       the keyboard focus */
    arrange.SetFocus();
    buttons.Raise();
  }

  void Leave() noexcept {
    arrange.Drop();
    FocusParent();
    Hide();
    timeout_timer.Cancel();

    ShowInfoBoxes();
    InfoBoxManager::Refresh();
    InfoBoxManager::ScheduleRedraw();
  }

protected:
  void OnPaint(Canvas &canvas) noexcept override {
    ContainerWindow::OnPaint(canvas);
    /* after the buttons, so the carried card covers Help/Close
       without hiding them when the dim comes forward */
    if (arrange.IsDefined() && arrange.IsVisible())
      arrange.PaintFloatingCard(canvas);
  }

  void OnResize(PixelSize new_size) noexcept override {
    ContainerWindow::OnResize(new_size);

    if (arrange.IsDefined())
      UpdateLayout();
  }
};

std::unique_ptr<OverlayWindow> overlay;

void
Enter() noexcept
{
  /* display mode can switch the current panel while the overlay is
     up; save and cancel must still talk to the panel we opened */
  saved_panel_index = CommonInterface::GetUIState().panel_index;
  saved_panel = InfoBoxManager::GetPanel(saved_panel_index);
  if (overlay == nullptr)
    overlay = std::make_unique<OverlayWindow>();
  overlay->Enter();
}

} // namespace

bool
InfoBoxArrange::IsActive() noexcept
{
  return overlay != nullptr && overlay->IsVisible();
}

void
InfoBoxArrange::Begin(unsigned id) noexcept
{
  if (InfoBoxManager::GetWindow(id) == nullptr)
    return;

  if (!IsActive())
    Enter();

  PlayHapticFeedback();

  overlay->GetArrange().SelectSlot(id);
  overlay->RestartTimeout();
}

void
InfoBoxArrange::Begin() noexcept
{
  if (IsActive() || InfoBoxManager::GetWindow(0) == nullptr)
    return;

  Enter();
  overlay->GetArrange().FocusSlot(0);
  overlay->RestartTimeout();
}

bool
InfoBoxArrange::SetFocus() noexcept
{
  if (!IsActive())
    return false;

  overlay->GetArrange().SetFocus();
  return true;
}

void
InfoBoxArrange::Save() noexcept
{
  if (!IsActive())
    return;

  overlay->Leave();
  InfoBoxManager::SavePanel(saved_panel_index);
}

void
InfoBoxArrange::Cancel() noexcept
{
  if (!IsActive())
    return;

  InfoBoxManager::GetPanel(saved_panel_index) = saved_panel;
  overlay->Leave();
}

void
InfoBoxArrange::Reset() noexcept
{
  overlay.reset();
}

bool
InfoBoxArrange::PastTouchSlop(PixelPoint a, PixelPoint b) noexcept
{
  const int slop = Layout::Scale(HasTouchScreen() ? 20 : 10);
  return (a - b).MagnitudeSquared() >= slop * slop;
}
