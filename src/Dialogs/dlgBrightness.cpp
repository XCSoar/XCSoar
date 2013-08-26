/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Dialogs/Dialogs.h"

#ifdef GNAV

#include "Dialogs/WidgetDialog.hpp"
#include "Form/DataField/Listener.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "RateLimiter.hpp"
#include "Components.hpp"
#include "Hardware/AltairControl.hpp"
#include "UIGlobals.hpp"

/**
 * This class limits the rate at which we forward user input to the
 * backlight.
 */
class AltairBacklightRateLimiter final
  : private RateLimiter {
  int current_value, new_value;

public:
  AltairBacklightRateLimiter()
    :RateLimiter(200, 50), current_value(-1), new_value(-1) {}

  ~AltairBacklightRateLimiter() {
    Apply();
  }

  void SetBacklight(int _value) {
    if (_value == new_value)
      return;

    new_value = _value;
    if (new_value != current_value)
      RateLimiter::Trigger();
    else
      RateLimiter::Cancel();
  }

private:
  void Apply() {
    if (new_value != current_value && altair_control.SetBacklight(new_value))
      current_value = new_value;
  }

  virtual void Run() override {
    Apply();
  }
};

class AltairBacklightWidget final
  : public RowFormWidget, private DataFieldListener {
  enum Controls {
    AUTO, BRIGHTNESS,
  };

  AltairBacklightRateLimiter rate_limiter;

public:
  AltairBacklightWidget(const DialogLook &_look)
    :RowFormWidget(_look) {}

  void Apply();

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;
};

void
AltairBacklightWidget::Apply()
{
  const bool auto_brightness = GetValueBoolean(AUTO);
  SetRowEnabled(BRIGHTNESS, !auto_brightness);

  rate_limiter.SetBacklight(auto_brightness
                            ? AltairControl::BACKLIGHT_AUTO
                            : GetValueInteger(BRIGHTNESS));
}

void
AltairBacklightWidget::OnModified(DataField &df)
{
  Apply();
}

void
AltairBacklightWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  int value;
  if (!altair_control.GetBacklight(value))
    value = 100;

  const bool auto_brightness = value == AltairControl::BACKLIGHT_AUTO;

  AddBoolean(_("Auto"),
             _("Enables automatic backlight, responsive to light sensor."),
             auto_brightness, this);

  AddInteger(_("Brightness"),
             _("Adjusts backlight. When automatic backlight is enabled, this biases the backlight algorithm. When the automatic backlight is disabled, this controls the backlight directly."),
             _T("%u %%"), _T("%u"), 0, 100, 5,
             value < 0 ? 100 : value, this);

  SetRowEnabled(BRIGHTNESS, !auto_brightness);
}

void
dlgBrightnessShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  AltairBacklightWidget widget(look);
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _("Screen Brightness"),
                    &widget);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  dialog.StealWidget();
}

#else /* !GNAV */

void
dlgBrightnessShowModal()
{
  /* only available on Altair */
}

#endif /* !GNAV */
