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
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"

#ifdef GNAV

#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Button.hpp"
#include "Units/Units.hpp"
#include "Form/DataField/Base.hpp"
#include "Form/DataField/Boolean.hpp"
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

static AltairBacklightRateLimiter *rate_limiter;
static bool EnableAutoBrightness = true;
static unsigned BrightnessValue = 0;

static void
UpdateValues()
{
  rate_limiter->SetBacklight(EnableAutoBrightness
                             ? AltairControl::BACKLIGHT_AUTO
                             : int(BrightnessValue));
}

static void
OnAutoData(DataField *Sender, DataField::DataAccessMode Mode)
{
  DataFieldBoolean &df = *(DataFieldBoolean *)Sender;

  switch (Mode) {
  case DataField::daChange:
    EnableAutoBrightness = df.GetAsBoolean();
    UpdateValues();
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
OnBrightnessData(DataField *Sender,
                 DataField::DataAccessMode Mode)
{
  switch (Mode) {
  case DataField::daChange:
    BrightnessValue = Sender->GetAsInteger();
    UpdateValues();
    break;

  case DataField::daSpecial:
    return;
  }
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnAutoData),
  DeclareCallBackEntry(OnBrightnessData),
  DeclareCallBackEntry(NULL)
};

void
dlgBrightnessShowModal()
{
  WndForm *wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                           _T("IDR_XML_BRIGHTNESS"));
  if (wf == NULL)
    return;

  LoadFormProperty(*wf, _T("prpBrightness"), BrightnessValue);
  LoadFormProperty(*wf, _T("prpAuto"), EnableAutoBrightness);

  AltairBacklightRateLimiter _rate_limiter;
  rate_limiter = &_rate_limiter;

  wf->ShowModal();

  delete wf;
}

#else /* !GNAV */

void
dlgBrightnessShowModal()
{
  /* only available on Altair */
}

#endif /* !GNAV */
