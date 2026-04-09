// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigurationDialog.hpp"
#include "Device/Driver/GDL90/Driver.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"
#include "UIGlobals.hpp"
#include "Units/Units.hpp"
#include "Widget/RowFormWidget.hpp"

#include <algorithm>
#include <cmath>

class GDL90ConfigurationWidget final : public RowFormWidget {
  enum Controls {
    USE_SYSTEM_UTC_DATE,
    HRANGE,
    VRANGE,
  };

  GDL90Device::GDL90Settings settings;

  /* Stored in profile as metres; limits match previous integer dialog. */
  static constexpr double HRANGE_MIN_M = 4000;
  static constexpr double HRANGE_MAX_M = 40000;
  static constexpr double HRANGE_STEP_M = 1000;
  static constexpr double VRANGE_MIN_M = 1000;
  static constexpr double VRANGE_MAX_M = 10000;
  static constexpr double VRANGE_STEP_M = 1000;

public:
  explicit GDL90ConfigurationWidget(const DialogLook &look)
    :RowFormWidget(look) {}

  void Prepare([[maybe_unused]] ContainerWindow &parent,
               [[maybe_unused]] const PixelRect &rc) noexcept override
  {
    LoadFromProfile(settings);

    const Unit distance_u = Units::GetUserDistanceUnit();
    const Unit altitude_u = Units::GetUserAltitudeUnit();

    AddBoolean(_("Use system date for UTC"),
               _("The GDL90 protocol sends time-of-day only, not calendar date. "
                 "When this is on, XCSoar sets the date from this device's "
                 "clock after a valid heartbeat. Turn it off if the system "
                 "date is unreliable (e.g. no RTC); use NMEA or set the clock "
                 "manually instead."),
               settings.use_system_utc_date);

    AddFloat(_("Horizontal Range"), nullptr,
             "%.0f %s", "%.0f",
             Units::ToUserUnit(HRANGE_MIN_M, distance_u),
             Units::ToUserUnit(HRANGE_MAX_M, distance_u),
             Units::ToUserUnit(HRANGE_STEP_M, distance_u),
             false,
             UnitGroup::DISTANCE,
             double(settings.hrange));

    AddFloat(_("Vertical Range"), nullptr,
             "%.0f %s", "%.0f",
             Units::ToUserUnit(VRANGE_MIN_M, altitude_u),
             Units::ToUserUnit(VRANGE_MAX_M, altitude_u),
             Units::ToUserUnit(VRANGE_STEP_M, altitude_u),
             false,
             UnitGroup::ALTITUDE,
             double(settings.vrange));
  }

  bool Save(bool &_changed) noexcept override
  {
    bool changed = false;

    if (SaveValue(USE_SYSTEM_UTC_DATE, settings.use_system_utc_date))
      changed = true;

    double h_m = double(settings.hrange);
    if (SaveValue(HRANGE, UnitGroup::DISTANCE, h_m)) {
      settings.hrange = static_cast<uint16_t>(std::clamp(
        std::lround(h_m), long(HRANGE_MIN_M), long(HRANGE_MAX_M)));
      changed = true;
    }

    double v_m = double(settings.vrange);
    if (SaveValue(VRANGE, UnitGroup::ALTITUDE, v_m)) {
      settings.vrange = static_cast<uint16_t>(std::clamp(
        std::lround(v_m), long(VRANGE_MIN_M), long(VRANGE_MAX_M)));
      changed = true;
    }

    SaveToProfile(settings);
    Profile::Save();

    _changed |= changed;
    if (_changed)
      ShowMessageBox(_("Changes to configuration saved. Restart XCSoar to apply changes."),
                     "", MB_OK);

    return true;
  }
};

void
ManageGDL90Dialog() noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      look, _("GDL90 Setup"),
                      new GDL90ConfigurationWidget(look));

  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}

