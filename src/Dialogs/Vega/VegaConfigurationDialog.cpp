/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Tabbed.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Device/device.hpp"
#include "Device/Descriptor.hpp"
#include "Device/Driver/Vega/Internal.hpp"
#include "Profile/Profile.hpp"
#include "Form/DataField/Enum.hpp"
#include "UIGlobals.hpp"
#include "Simulator.hpp"
#include "Compiler.h"
#include "OS/Sleep.h"
#include "Util/Macros.hpp"
#include "Operation/MessageOperationEnvironment.hpp"

#include <assert.h>
#include <string.h>

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
static WndForm *wf = NULL;
static TabbedControl *tabbed;

static void
SetParametersScheme(int schemetype)
{
  if(ShowMessageBox(_("Set new audio scheme?  Old values will be lost."),
                 _T("Vega"),
                 MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  const VEGA_SCHEME &scheme = VegaSchemes[schemetype];

  tabbed->PreparePage(2);
  LoadAudioModeScheme((VegaParametersWidget &)tabbed->GetPage(2), scheme);

  for (unsigned i = 0; audio_pages[i] != NULL; ++i) {
    tabbed->PreparePage(4 + i);
    ((VegaAudioParametersWidget &)tabbed->GetPage(4 + i)).LoadScheme(scheme.audio[i]);
  }
}

static void
UpdateCaption()
{
  wf->SetCaption(captions[tabbed->GetCurrentPage()]);
}

static void
PageSwitched()
{
  UpdateCaption();
}

static void
OnNextClicked(gcc_unused WndButton &Sender)
{
  tabbed->NextPage();
  PageSwitched();
}

static void
OnPrevClicked(gcc_unused WndButton &Sender)
{
  tabbed->PreviousPage();
  PageSwitched();
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  bool require_restart = false;
  if (!tabbed->Save(changed, require_restart))
    return;

  // make sure changes are sent to device
  wf->SetModalResult(mrOK);
}

static void
OnSaveClicked(gcc_unused WndButton &Sender)
{
  bool _changed = false, require_restart = false;
  if (!tabbed->Save(_changed, require_restart))
    return;

  changed |= _changed;
  dirty |= changed;

  // make sure changes are sent to device
  MessageOperationEnvironment env;
  if (dirty && device->SendSetting("StoreToEeprom", 2, env))
    dirty = false;
}

static void
OnDemoClicked(gcc_unused WndButton &Sender)
{
  // retrieve changes from form
  bool require_restart = false;
  if (!tabbed->Save(changed, require_restart))
    return;

  dlgVegaDemoShowModal();
}

static bool
FormKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
  switch (key_code) {
  case KEY_LEFT:
#ifdef GNAV
  case '6':
#endif
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->SetFocus();
    tabbed->PreviousPage();
    PageSwitched();
    //((WndButton *)wf->FindByName(_T("cmdPrev")))->SetFocused(true, NULL);
    return true;

  case KEY_RIGHT:
#ifdef GNAV
  case '7':
#endif
    ((WndButton *)wf->FindByName(_T("cmdNext")))->SetFocus();
    tabbed->NextPage();
    PageSwitched();
    //((WndButton *)wf->FindByName(_T("cmdNext")))->SetFocused(true, NULL);
    return true;

  default:
    return false;
  }
}

class VegaSchemeButtonsPage : public RowFormWidget, ActionListener {
public:
  VegaSchemeButtonsPage(const DialogLook &look)
    :RowFormWidget(look) {}

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) {
    RowFormWidget::Prepare(parent, rc);

    AddButton(_T("Vega"), this, 0);
    AddButton(_T("Borgelt"), this, 1);
    AddButton(_T("Cambridge"), this, 2);
    AddButton(_T("Zander"), this, 3);
  }

  /* methods from ActionListener */
  virtual void OnAction(int id) {
    SetParametersScheme(id);
  }
};

static Window *
OnCreatePager(ContainerWindow &parent, PixelRect rc,
              WindowStyle style)
{
  style.ControlParent();

  tabbed = new TabbedControl(parent, rc, style);

  const DialogLook &look = UIGlobals::GetDialogLook();

  tabbed->AddPage(new VegaParametersWidget(look, *device, hardware_parameters));
  tabbed->AddPage(new VegaParametersWidget(look, *device,
                                           calibration_parameters));
  tabbed->AddPage(new VegaParametersWidget(look, *device,
                                           audio_mode_parameters));
  tabbed->AddPage(new VegaParametersWidget(look, *device,
                                           audio_deadband_parameters));
  tabbed->AddPage(new VegaAudioParametersWidget(look, *device,
                                                "CruiseFaster"));
  tabbed->AddPage(new VegaAudioParametersWidget(look, *device,
                                                "CruiseSlower"));
  tabbed->AddPage(new VegaAudioParametersWidget(look, *device,
                                                "CruiseLift"));
  tabbed->AddPage(new VegaAudioParametersWidget(look, *device,
                                                "CirclingClimbingHi"));
  tabbed->AddPage(new VegaAudioParametersWidget(look, *device,
                                                "CirclingClimbingLow"));
  tabbed->AddPage(new VegaAudioParametersWidget(look, *device,
                                                "CirclingDescending"));
  tabbed->AddPage(new VegaParametersWidget(look, *device,
                                           logger_parameters));
  tabbed->AddPage(new VegaParametersWidget(look, *device,
                                           mixer_parameters));
  tabbed->AddPage(new VegaParametersWidget(look, *device,
                                           flarm_alert_parameters));
  tabbed->AddPage(new VegaParametersWidget(look, *device,
                                           flarm_id_parameters));
  tabbed->AddPage(new VegaParametersWidget(look, *device,
                                           flarm_repeat_parameters));
  tabbed->AddPage(new VegaParametersWidget(look, *device, alert_parameters));
  tabbed->AddPage(new VegaParametersWidget(look, *device, limit_parameters));

  tabbed->AddPage(new VegaSchemeButtonsPage(look));

  tabbed->AddPage(new VegaParametersWidget(look, *device, display_parameters));

  return tabbed;
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(OnDemoClicked),
  DeclareCallBackEntry(OnSaveClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnCreatePager),
  DeclareCallBackEntry(NULL)
};

bool
dlgConfigurationVarioShowModal(Device &_device)
{
  device = (VegaDevice *)&_device;
  changed = dirty = false;

  wf = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  Layout::landscape
                  ? _T("IDR_XML_VARIO_L") : _T("IDR_XML_VARIO_L"));
  if (!wf)
    return false;

  wf->SetKeyDownNotify(FormKeyDown);

  UpdateCaption();

  wf->ShowModal();

  delete wf;
  wf = NULL;

  return changed;
}
