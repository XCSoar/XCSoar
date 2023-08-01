// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VegaDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Device/MultipleDevices.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Float.hpp"
#include "Form/DataField/Boolean.hpp"
#include "time/PeriodClock.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "Math/Util.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

static double VegaDemoW = 0;
static double VegaDemoV = 0;
static bool VegaDemoAudioClimb = true;

static void
VegaWriteDemo()
{
  static PeriodClock last_time;
  if (!last_time.CheckUpdate(std::chrono::milliseconds(250)))
    return;

  TCHAR dbuf[100];
  _stprintf(dbuf, _T("PDVDD,%d,%d"),
            iround(VegaDemoW * 10),
            iround(VegaDemoV * 10));

  PopupOperationEnvironment env;
  backend_components->devices->VegaWriteNMEA(dbuf, env);
}

class VegaDemoWidget final
  : public RowFormWidget, private DataFieldListener {
  enum Controls {
    VARIO,
    AIRSPEED,
    CIRCLING,
  };

public:
  VegaDemoWidget(const DialogLook &_look)
    :RowFormWidget(_look) {}

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
VegaDemoWidget::OnModified(DataField &df) noexcept
{
  if (IsDataField(VARIO, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    VegaDemoW = Units::ToSysVSpeed(dff.GetValue());
  } else if (IsDataField(AIRSPEED, df)) {
    const DataFieldFloat &dff = (const DataFieldFloat &)df;
    VegaDemoV = Units::ToSysSpeed(dff.GetValue());
  } else if (IsDataField(CIRCLING, df)) {
    const DataFieldBoolean &dfb = (const DataFieldBoolean &)df;
    VegaDemoAudioClimb = dfb.GetValue();
  }

  VegaWriteDemo();
}

void
VegaDemoWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  AddFloat(_("TE vario"),
           _("This produces a fake TE vario gross vertical velocity.  It can be used when in circling mode to demonstrate the lift tones.  When not in circling mode, set this to a realistic negative value so speed command tones are produced."),
           _T("%.1f %s"), _T("%.1f"),
           Units::ToUserVSpeed(-20), Units::ToUserVSpeed(20),
           GetUserVerticalSpeedStep(),
           false, UnitGroup::VERTICAL_SPEED, VegaDemoW, this);

  AddFloat(_("Airspeed"),
           _("This produces a fake airspeed.  It can be used when not in circling mode to demonstrate the speed command tones."),
           _T("%.0f %s"), _T("%.0f"), 0, 200, 2,
           false, UnitGroup::HORIZONTAL_SPEED, VegaDemoV, this);

  AddBoolean(_("Circling"),
             _("This forces the variometer into circling or cruise mode"),
             VegaDemoAudioClimb, this);
}

void
dlgVegaDemoShowModal()
{
  PopupOperationEnvironment env;
  backend_components->devices->VegaWriteNMEA(_T("PDVSC,S,DemoMode,0"), env);
  backend_components->devices->VegaWriteNMEA(_T("PDVSC,S,DemoMode,3"), env);

  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<VegaDemoWidget>
    dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
           look, _("Vario Demo"));
  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget(look);
  dialog.ShowModal();

  // deactivate demo.
  backend_components->devices->VegaWriteNMEA(_T("PDVSC,S,DemoMode,0"), env);
}
