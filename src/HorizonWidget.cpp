// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HorizonWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Interface.hpp"
#include "Input/InputEvents.hpp"
#include "UIUtil/GestureManager.hpp"
#include "Screen/Layout.hpp"
#include "ui/window/AntiFlickerWindow.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Renderer/HorizonRenderer.hpp"
#include "Look/IconLook.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/ButtonRenderer.hpp"
#include "Dialogs/Settings/Panels/HorizonConfigPanel.hpp"

static bool
SameAhrs(const AttitudeState &a, const AttitudeState &b) noexcept
{
  const bool bank = a.bank_angle_available;
  const bool pitch = a.pitch_angle_available;
  if (bank != bool(b.bank_angle_available) ||
      pitch != bool(b.pitch_angle_available))
    return false;
  if (bank && a.bank_angle != b.bank_angle)
    return false;
  if (pitch && a.pitch_angle != b.pitch_angle)
    return false;
  return true;
}

static bool
SameTape(const std::optional<double> &a,
         const std::optional<double> &b) noexcept
{
  if (a.has_value() != b.has_value())
    return false;
  return !a || iround(*a) == iround(*b);
}

/**
 * A Window which renders a terrain and airspace cross-section
 */
class HorizonWindow : public AntiFlickerWindow {
  const HorizonLook &look;
  const bool& inverse;

  AttitudeState attitude;
  HorizonRenderer::HorizonReadout readout;
  bool have_sample = false;

  GestureManager gestures;
  bool dragging = false;

  /**
   * The button that opens the settings of the tapes, in the upper
   * right corner of the dial; empty while the dial has no room for it.
   */
  PixelRect settings_button{};

  /** The gear is held: it is drawn pressed until the finger lifts. */
  bool settings_pressed = false;

public:
  /**
   * Constructor. Initializes most class members.
   */
  HorizonWindow(const HorizonLook &_look, const bool &_inverse) noexcept
    :look(_look),inverse(_inverse)
  {
    attitude.Reset();
  }

  void ReadBlackboard(const MoreData &basic) noexcept {
    const AttitudeState next_attitude = basic.attitude;
    auto next_readout =
      HorizonRenderer::Readout(basic, CommonInterface::Calculated(),
                               CommonInterface::GetUISettings().horizon);
    if (have_sample &&
        readout.speed_title == next_readout.speed_title &&
        readout.altitude_title == next_readout.altitude_title &&
        readout.course_title == next_readout.course_title &&
        SameAhrs(attitude, next_attitude) &&
        SameTape(readout.speed, next_readout.speed) &&
        SameTape(readout.altitude, next_readout.altitude) &&
        SameTape(readout.course, next_readout.course))
      return;

    attitude = next_attitude;
    readout = next_readout;
    have_sample = true;
    Invalidate();
  }

  /**
   * Reads the readout again although no new sample has arrived,
   * because the choice of the tapes has changed.
   */
  void Refresh() noexcept {
    have_sample = false;
    ReadBlackboard(CommonInterface::Basic());
  }

private:
  void StopDragging() noexcept {
    if (!dragging)
      return;

    dragging = false;
    ReleaseCapture();
    SetSettingsPressed(false);
  }

  void SetSettingsPressed(bool pressed) noexcept {
    if (pressed == settings_pressed)
      return;

    settings_pressed = pressed;
    Invalidate();
  }

protected:
  /* virtual methods from AntiFlickerWindow */
  void OnPaintBuffer(Canvas &canvas) noexcept override {
    if (inverse)
      canvas.Clear(COLOR_BLACK);
    else
      canvas.ClearWhite();

    HorizonRenderer::Draw(canvas, canvas.GetRect(), look, attitude,
                          readout);
    DrawSettingsButton(canvas);
  }

  /**
   * A gear in the upper right corner of the page shows that the tapes
   * can be chosen, and opens their settings when tapped.  It has the
   * size, the place and the look of the menu buttons over the map,
   * which are not shown on this page, so that it is recognised as a
   * button.
   */
  void DrawSettingsButton(Canvas &canvas) noexcept {
    const PixelRect rc = canvas.GetRect();
    const int size = int(Layout::GetMaximumControlHeight());
    const int margin = int(Layout::GetTextPadding());
    if (int(rc.GetWidth()) < size * 4 || int(rc.GetHeight()) < size * 4) {
      settings_button = {};
      return;
    }

    settings_button = {rc.right - margin - size, rc.top + margin,
                       rc.right - margin, rc.top + margin + size};

    const ButtonLook &button_look = UIGlobals::GetDialogLook().button;
    ButtonFrameRenderer(button_look).DrawButton(canvas, settings_button,
                                                settings_pressed
                                                ? ButtonState::PRESSED
                                                : ButtonState::ENABLED);
    canvas.SetTextColor(button_look.standard.foreground_color);
    UIGlobals::GetIconLook().hBmpTabSettings.Draw(
      canvas, settings_button.GetCenter(),
      settings_button.GetHeight() * 2 / 3);
  }

  /* virtual methods from Window */
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseDouble(PixelPoint p) noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  void OnCancelMode() noexcept override;
};

bool
HorizonWindow::OnMouseDouble([[maybe_unused]] PixelPoint p) noexcept
{
  StopDragging();
  InputEvents::ShowMenu();
  return true;
}

bool
HorizonWindow::OnMouseDown(PixelPoint p) noexcept
{
  if (!dragging) {
    dragging = true;
    SetCapture();
    gestures.Start(p, Layout::Scale(20));
    SetSettingsPressed(settings_button.Contains(p));
  }

  return true;
}

bool
HorizonWindow::OnMouseUp(PixelPoint p) noexcept
{
  if (dragging) {
    const bool on_settings = settings_pressed;
    StopDragging();

    const char *gesture = gestures.Finish();
    if (gesture && InputEvents::processGesture(gesture))
      return true;

    if (gesture == nullptr && on_settings && settings_button.Contains(p)) {
      /* the dialog announces the new choice, see OnUISettingsUpdate() */
      ShowHorizonConfigDialog();
      return true;
    }
  }

  return false;
}

bool
HorizonWindow::OnMouseMove(PixelPoint p,
                           [[maybe_unused]] unsigned keys) noexcept
{
  if (dragging) {
    gestures.Update(p);
    if (settings_pressed && !settings_button.Contains(p))
      SetSettingsPressed(false);
  }

  return true;
}

void
HorizonWindow::OnCancelMode() noexcept
{
  AntiFlickerWindow::OnCancelMode();
  StopDragging();
}

bool
HorizonWindow::OnKeyDown(unsigned key_code) noexcept
{
  return InputEvents::processKey(key_code);
}

void
HorizonWidget::Update(const MoreData &basic) noexcept
{
  HorizonWindow &w = (HorizonWindow &)GetWindow();
  w.ReadBlackboard(basic);
}

void
HorizonWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  const Look &look = UIGlobals::GetLook();

  WindowStyle style;
  style.Hide();

  auto w = std::make_unique<HorizonWindow>(look.horizon, look.info_box.inverse);
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

void
HorizonWidget::Show(const PixelRect &rc) noexcept
{
  Update(CommonInterface::Basic());
  CommonInterface::GetLiveBlackboard().AddListener(rate_limiter);

  WindowWidget::Show(rc);
}

void
HorizonWidget::Move(const PixelRect &rc) noexcept
{
  WindowWidget::Move(rc);

  /* A layout pass with the same pixel size (text size, DPI) does not
     resize the window, so it would not repaint.  The draw refits the
     tapes and chevrons from the rect and the layout scale. */
  GetWindow().Invalidate();
}

void
HorizonWidget::Hide() noexcept
{
  WindowWidget::Hide();

  CommonInterface::GetLiveBlackboard().RemoveListener(rate_limiter);
  rate_limiter.Cancel();
}

void
HorizonWidget::OnGPSUpdate(const MoreData &basic) noexcept
{
  Update(basic);
}

void
HorizonWidget::OnCalculatedUpdate(const MoreData &basic,
                                  [[maybe_unused]] const DerivedInfo &calculated) noexcept
{
  /* the height above ground is calculated after the fix, so the tape
     follows the calculation as well, not only the next fix */
  Update(basic);
}

void
HorizonWidget::OnUISettingsUpdate([[maybe_unused]] const UISettings &settings) noexcept
{
  /* the choice of the tapes has changed, from the gear button or from
     the input event "Setup Horizon": show it at once, not with the
     next fix */
  HorizonWindow &w = (HorizonWindow &)GetWindow();
  w.Refresh();
}
