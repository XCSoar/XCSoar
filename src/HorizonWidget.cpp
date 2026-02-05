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

/**
 * A Window which renders a terrain and airspace cross-section
 */
class HorizonWindow : public AntiFlickerWindow {
  const HorizonLook &look;
  const bool& inverse;

  AttitudeState attitude;

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

  void ReadBlackboard(const AttitudeState _attitude) noexcept {
    attitude = _attitude;
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

    if (!attitude.bank_angle_available && !attitude.pitch_angle_available)
      // TODO: paint "no data" hint
      return;

    HorizonRenderer::Draw(canvas, canvas.GetRect(), look, attitude);
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
  w.ReadBlackboard(basic.attitude);
  w.Invalidate();
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
  CommonInterface::GetLiveBlackboard().AddListener(*this);

  WindowWidget::Show(rc);
}

void
HorizonWidget::Hide() noexcept
{
  WindowWidget::Hide();

  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
}

void
HorizonWidget::OnGPSUpdate(const MoreData &basic) noexcept
{
  Update(basic);
}
