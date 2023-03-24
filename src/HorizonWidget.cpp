// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HorizonWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Interface.hpp"
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
};

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
  style.Disable();

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
