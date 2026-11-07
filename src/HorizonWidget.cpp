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
#include "Math/Util.hpp"

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
    const auto next_readout = HorizonRenderer::Readout(basic);
    if (have_sample &&
        SameAhrs(attitude, next_attitude) &&
        SameTape(readout.speed, next_readout.speed) &&
        SameTape(readout.altitude, next_readout.altitude))
      return;

    attitude = next_attitude;
    readout = next_readout;
    have_sample = true;
    Invalidate();
  }

private:
  void StopDragging() noexcept {
    if (!dragging)
      return;

    dragging = false;
    ReleaseCapture();
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
  }

  return true;
}

bool
HorizonWindow::OnMouseUp([[maybe_unused]] PixelPoint p) noexcept
{
  if (dragging) {
    StopDragging();

    const char *gesture = gestures.Finish();
    if (gesture && InputEvents::processGesture(gesture))
      return true;
  }

  return false;
}

bool
HorizonWindow::OnMouseMove(PixelPoint p,
                           [[maybe_unused]] unsigned keys) noexcept
{
  if (dragging)
    gestures.Update(p);

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
