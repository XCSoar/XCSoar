/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "VegaDialogs.hpp"
#include "Schemes.hpp"
#include "HardwareParameters.hpp"
#include "CalibrationParameters.hpp"
#include "AudioModeParameters.hpp"
#include "AudioDeadbandParameters.hpp"
#include "AudioParameters.hpp"
#include "LoggerParameters.hpp"
#include "MixerParameters.hpp"
#include "FlarmAlertParameters.hpp"
#include "FlarmIdentificationParameters.hpp"
#include "FlarmRepeatParameters.hpp"
#include "AlertParameters.hpp"
#include "LimitParameters.hpp"
#include "DisplayParameters.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Screen/Layout.hpp"
#include "Device/Driver/Vega/Internal.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Operation/MessageOperationEnvironment.hpp"

static const TCHAR *const captions[] = {
  _T(" 1 Hardware"),
  _T(" 2 Calibration"),
  _T(" 3 Audio Modes"),
  _T(" 4 Deadband"),
  _T(" 5 Tones: Cruise Faster"),
  _T(" 6 Tones: Cruise Slower"),
  _T(" 7 Tones: Cruise in Lift"),
  _T(" 8 Tones: Circling, climbing fast"),
  _T(" 9 Tones: Circling, climbing slow"),
  _T("10 Tones: Circling, descending"),
  _T("11 Vario flight logger"),
  _T("12 Audio mixer"),
  _T("13 FLARM Alerts"),
  _T("14 FLARM Identification"),
  _T("15 FLARM Repeats"),
  _T("16 Alerts"),
  _T("17 Airframe Limits"),
  _T("18 Audio Schemes"),
  _T("19 Display"),
};

static const char *const audio_pages[] = {
  "CruiseFaster",
  "CruiseSlower",
  "CruiseLift",
  "CirclingClimbingHi",
  "CirclingClimbingLow",
  "CirclingDescending",
  NULL
};

static VegaDevice *device;
static bool changed, dirty;

class VegaConfigurationExtraButtons final
  : public NullWidget {
  struct Layout {
    PixelRect demo, save;

    Layout(const PixelRect &rc):demo(rc), save(rc) {
      const unsigned height = rc.GetHeight();
      const unsigned max_v_height = 2 * ::Layout::GetMaximumControlHeight();

      if (height >= max_v_height) {
        demo.top = rc.bottom - max_v_height;
        demo.bottom = save.top = unsigned(demo.top + rc.bottom) / 2;
      } else
        demo.right = save.left = unsigned(rc.left + rc.right) / 2;
    }
  };

  WidgetDialog &dialog;

  Button demo_button, save_button;

public:
  VegaConfigurationExtraButtons(WidgetDialog &_dialog)
    :dialog(_dialog) {}

protected:
  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    Layout layout(rc);

    WindowStyle style;
    style.Hide();
    style.TabStop();

    const auto &button_look = dialog.GetLook().button;
    demo_button.Create(parent, button_look, _("Demo"),
                       layout.demo, style,
                       [this](){ OnDemo(); });
    save_button.Create(parent, button_look, _("Save"),
                       layout.save, style,
                       [this](){ OnSave(); });
  }

  void Show(const PixelRect &rc) noexcept override {
    Layout layout(rc);
    demo_button.MoveAndShow(layout.demo);
    save_button.MoveAndShow(layout.save);
  }

  void Hide() noexcept override {
    demo_button.FastHide();
    save_button.FastHide();
  }

  void Move(const PixelRect &rc) noexcept override {
    Layout layout(rc);
    demo_button.Move(layout.demo);
    save_button.Move(layout.save);
  }

private:
  void OnDemo();
  void OnSave();
};

static void
SetParametersScheme(PagerWidget &pager, int schemetype)
{
  if(ShowMessageBox(_("Set new audio scheme?  Old values will be lost."),
                 _T("Vega"),
                 MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  const VEGA_SCHEME &scheme = VegaSchemes[schemetype];

  pager.PrepareWidget(2);
  LoadAudioModeScheme((VegaParametersWidget &)pager.GetWidget(2), scheme);

  for (unsigned i = 0; audio_pages[i] != NULL; ++i) {
    pager.PrepareWidget(4 + i);
    ((VegaAudioParametersWidget &)pager.GetWidget(4 + i)).LoadScheme(scheme.audio[i]);
  }
}

static auto
MakeSetParametersScheme(PagerWidget &pager, int schemetype) noexcept
{
  return [&pager, schemetype](){ SetParametersScheme(pager, schemetype); };
}

static void
UpdateCaption(WndForm &form, unsigned page)
{
  form.SetCaption(captions[page]);
}

inline void
VegaConfigurationExtraButtons::OnSave()
{
  bool _changed = false;
  if (!dialog.GetWidget().Save(_changed))
    return;

  changed |= _changed;
  dirty |= changed;

  // make sure changes are sent to device
  MessageOperationEnvironment env;
  if (dirty) {
    try {
      device->SendSetting("StoreToEeprom", 2, env);
      dirty = false;
    } catch (OperationCancelled) {
    } catch (...) {
      env.SetError(std::current_exception());
    }
  }
}

inline void
VegaConfigurationExtraButtons::OnDemo()
{
  // retrieve changes from form
  if (!dialog.GetWidget().Save(changed))
    return;

  dlgVegaDemoShowModal();
}

class VegaSchemeButtonsPage : public RowFormWidget {
  PagerWidget &pager;

public:
  VegaSchemeButtonsPage(PagerWidget &_pager, const DialogLook &look)
    :RowFormWidget(look), pager(_pager) {}

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    RowFormWidget::Prepare(parent, rc);

    AddButton(_T("Vega"), MakeSetParametersScheme(pager, 0));
    AddButton(_T("Borgelt"), MakeSetParametersScheme(pager, 1));
    AddButton(_T("Cambridge"), MakeSetParametersScheme(pager, 2));
    AddButton(_T("Zander"), MakeSetParametersScheme(pager, 3));
  }
};

static void
FillPager(PagerWidget &pager)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  pager.Add(std::make_unique<VegaParametersWidget>(look, *device, hardware_parameters));
  pager.Add(std::make_unique<VegaParametersWidget>(look, *device, calibration_parameters));
  pager.Add(std::make_unique<VegaParametersWidget>(look, *device, audio_mode_parameters));
  pager.Add(std::make_unique<VegaParametersWidget>(look, *device,
                                                   audio_deadband_parameters));
  pager.Add(std::make_unique<VegaAudioParametersWidget>(look, *device, "CruiseFaster"));
  pager.Add(std::make_unique<VegaAudioParametersWidget>(look, *device, "CruiseSlower"));
  pager.Add(std::make_unique<VegaAudioParametersWidget>(look, *device, "CruiseLift"));
  pager.Add(std::make_unique<VegaAudioParametersWidget>(look, *device,
                                                        "CirclingClimbingHi"));
  pager.Add(std::make_unique<VegaAudioParametersWidget>(look, *device,
                                                        "CirclingClimbingLow"));
  pager.Add(std::make_unique<VegaAudioParametersWidget>(look, *device,
                                                        "CirclingDescending"));
  pager.Add(std::make_unique<VegaParametersWidget>(look, *device, logger_parameters));
  pager.Add(std::make_unique<VegaParametersWidget>(look, *device, mixer_parameters));
  pager.Add(std::make_unique<VegaParametersWidget>(look, *device, flarm_alert_parameters));
  pager.Add(std::make_unique<VegaParametersWidget>(look, *device, flarm_id_parameters));
  pager.Add(std::make_unique<VegaParametersWidget>(look, *device, flarm_repeat_parameters));
  pager.Add(std::make_unique<VegaParametersWidget>(look, *device, alert_parameters));
  pager.Add(std::make_unique<VegaParametersWidget>(look, *device, limit_parameters));

  pager.Add(std::make_unique<VegaSchemeButtonsPage>(pager, look));

  pager.Add(std::make_unique<VegaParametersWidget>(look, *device, display_parameters));
}

bool
dlgConfigurationVarioShowModal(Device &_device)
{
  device = (VegaDevice *)&_device;
  changed = dirty = false;

  const DialogLook &look = UIGlobals::GetDialogLook();

  TWidgetDialog<ArrowPagerWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look, _("Vario Configuration"));
  dialog.SetWidget(look.button,
                   dialog.MakeModalResultCallback(mrOK),
                   std::make_unique<VegaConfigurationExtraButtons>(dialog));
  FillPager(dialog.GetWidget());

  dialog.GetWidget().SetPageFlippedCallback([&dialog](){
    UpdateCaption(dialog, dialog.GetWidget().GetCurrentIndex());
  });
  UpdateCaption(dialog, dialog.GetWidget().GetCurrentIndex());

  dialog.ShowModal();

  return changed || dialog.GetChanged();
}
