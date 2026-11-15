// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HorizonConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  SPEED,
  COURSE,
  ALTITUDE,
};

static constexpr StaticEnumChoice speed_list[] = {
  { HorizonSettings::Speed::INDICATED, N_("Indicated airspeed"),
    N_("The speed the wing feels, as on an airspeed indicator.") },
  { HorizonSettings::Speed::TRUE_AIRSPEED, N_("True airspeed"),
    N_("The speed through the air, corrected for the air density.") },
  { HorizonSettings::Speed::GROUND, N_("Ground speed"),
    N_("The speed over the ground from the GPS, which differs from "
       "the airspeed by the wind.") },
  nullptr
};

static constexpr StaticEnumChoice course_list[] = {
  { HorizonSettings::Course::HEADING, N_("Heading"),
    N_("Where the nose points: from a compass when one is connected, "
       "otherwise the track corrected for the wind, or the track "
       "itself while the wind is not known.") },
  { HorizonSettings::Course::TRACK, N_("Track"),
    N_("The direction of motion over the ground from the GPS, which "
       "differs from the heading by the drift angle.") },
  nullptr
};

static constexpr StaticEnumChoice altitude_list[] = {
  { HorizonSettings::Altitude::ABOVE_GROUND, N_("Height above ground"),
    N_("The height above the terrain below; needs a terrain file.") },
  { HorizonSettings::Altitude::BARO, N_("Barometric altitude"),
    N_("The pressure altitude from a connected instrument, corrected "
       "with the QNH.") },
  { HorizonSettings::Altitude::GPS, N_("GPS altitude"),
    N_("The altitude from the GPS.") },
  nullptr
};

class HorizonConfigPanel final : public RowFormWidget {
public:
  HorizonConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
HorizonConfigPanel::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  const HorizonSettings &settings = CommonInterface::GetUISettings().horizon;

  RowFormWidget::Prepare(parent, rc);

  AddEnum(_("Speed"),
          _("What the tape on the left of the full-screen horizon shows."),
          speed_list, (unsigned)settings.speed);
  AddEnum(_("Course"),
          _("What the tape along the top of the full-screen horizon shows."),
          course_list, (unsigned)settings.course);
  AddEnum(_("Altitude"),
          _("What the tape on the right of the full-screen horizon shows."),
          altitude_list, (unsigned)settings.altitude);
}

bool
HorizonConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  HorizonSettings &settings = CommonInterface::SetUISettings().horizon;
  changed |= SaveValueEnum(SPEED, ProfileKeys::HorizonSpeed,
                           settings.speed);
  changed |= SaveValueEnum(COURSE, ProfileKeys::HorizonCourse,
                           settings.course);
  changed |= SaveValueEnum(ALTITUDE, ProfileKeys::HorizonAltitude,
                           settings.altitude);

  _changed |= changed;
  return true;
}

void
ShowHorizonConfigDialog()
{
  auto *panel = new HorizonConfigPanel();
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(), _("Horizon"), panel);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  /* the dialog has saved the panel on OK and recorded whether
     anything changed */
  if (dialog.ShowModal() != mrOK || !dialog.GetChanged())
    return;

  Profile::Save();

  /* tell the open horizon, however the dialog was reached */
  ActionInterface::SendMapSettings(true);
}
