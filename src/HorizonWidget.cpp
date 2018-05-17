/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "HorizonWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Interface.hpp"
#include "Screen/AntiFlickerWindow.hpp"
#include "Screen/Canvas.hpp"
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
  HorizonWindow(const HorizonLook &_look, const bool &_inverse):look(_look),inverse(_inverse) {
    attitude.Reset();
  }

  void ReadBlackboard(const AttitudeState _attitude) {
    attitude = _attitude;
    Invalidate();
  }

protected:
  /* virtual methods from AntiFlickerWindow */
  void OnPaintBuffer(Canvas &canvas) override {
    if (inverse)
      canvas.Clear(COLOR_BLACK);
    else
      canvas.ClearWhite();

    if (!attitude.IsBankAngleUseable() && !attitude.IsPitchAngleUseable())
      // TODO: paint "no data" hint
      return;

    HorizonRenderer::Draw(canvas, canvas.GetRect(), look, attitude);
  }
};

void
HorizonWidget::Update(const MoreData &basic)
{
  HorizonWindow &w = (HorizonWindow &)GetWindow();
  w.ReadBlackboard(basic.attitude);
  w.Invalidate();
}

void
HorizonWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const Look &look = UIGlobals::GetLook();

  WindowStyle style;
  style.Hide();
  style.Disable();

  HorizonWindow *w = new HorizonWindow(look.horizon, look.info_box.inverse);
  w->Create(parent, rc, style);
  SetWindow(w);
}

void
HorizonWidget::Unprepare()
{
  DeleteWindow();
}

void
HorizonWidget::Show(const PixelRect &rc)
{
  Update(CommonInterface::Basic());
  CommonInterface::GetLiveBlackboard().AddListener(*this);

  WindowWidget::Show(rc);
}

void
HorizonWidget::Hide()
{
  WindowWidget::Hide();

  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
}

void
HorizonWidget::OnGPSUpdate(const MoreData &basic)
{
  Update(basic);
}
