// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PlaneDialogs.hpp"
#include "WeGlideTypePicker.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/PickList.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Plane/Plane.hpp"
#include "Computer/Settings.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "net/client/WeGlide/AircraftList.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"

namespace {

class PlaneDetails {
  using BadgeStyle = GroupedListWidget::BadgeStyle;
  using ValueState = GroupedListWidget::ValueState;

  GroupedListWidget *list = nullptr;
  WidgetDialog *dialog = nullptr;
  Plane plane;

public:
  explicit PlaneDetails(const Plane &_plane) noexcept
    :plane(_plane) {}

  const Plane &GetValue() const noexcept {
    return plane;
  }

  void SetList(GroupedListWidget &_list) noexcept {
    list = &_list;
  }

  void SetDialog(WidgetDialog &_dialog) noexcept {
    dialog = &_dialog;
  }

  void Refresh() noexcept;

  void EditRegistration() noexcept;
  void EditCompetitionId() noexcept;
  void EditPolar() noexcept;
  void EditType() noexcept;
  void EditHandicap() noexcept;
  void EditWingArea() noexcept;
  void EditEmptyMass() noexcept;
  void EditMaxBallast() noexcept;
  void EditDumpTime() noexcept;
  void EditMaxSpeed() noexcept;
  void EditWeGlideType() noexcept;

private:
  void UpdateCaption() noexcept;
  void Build() noexcept;

  template<size_t N>
  void EditText(StaticString<N> &field, const char *caption) noexcept;
};

void
PlaneDetails::UpdateCaption() noexcept
{
  if (dialog == nullptr)
    return;

  StaticString<128> tmp;
  tmp.Format("%s: %s", _("Plane Details"), plane.registration.c_str());
  dialog->SetCaption(tmp);
}

template<size_t N>
void
PlaneDetails::EditText(StaticString<N> &field,
                       const char *caption) noexcept
{
  auto copy = field;
  if (!TextEntryDialog(copy, caption))
    return;

  field = copy;
  Refresh();
}

void
PlaneDetails::EditRegistration() noexcept
{
  EditText(plane.registration, _("Registration"));
}

void
PlaneDetails::EditCompetitionId() noexcept
{
  EditText(plane.competition_id, _("Comp. ID"));
}

void
PlaneDetails::EditType() noexcept
{
  EditText(plane.type, _("Type"));
}

void
PlaneDetails::EditPolar() noexcept
{
  dlgPlanePolarShowModal(plane);
  if (plane.polar_name != "Custom")
    plane.type = plane.polar_name;

  Refresh();
}

void
PlaneDetails::EditHandicap() noexcept
{
  double value = plane.handicap;
  if (!PickList(_("Handicap"), value, 50, 150, 1,
                [](double v, StaticString<64> &text) {
                  text.Format("%u %%", (unsigned)v);
                }))
    return;

  plane.handicap = (unsigned)value;
  Refresh();
}

void
PlaneDetails::EditWingArea() noexcept
{
  double value = plane.wing_area;
  if (!PickList(_("Wing Area"), value, 0, 40, 0.1,
                [](double v, StaticString<64> &text) {
                  text.Format("%.1f m²", v);
                }))
    return;

  plane.wing_area = value;
  Refresh();
}

void
PlaneDetails::EditEmptyMass() noexcept
{
  double value = Units::ToUserMass(plane.empty_mass);
  if (!PickList(_("Empty Mass"), value, 0, 1000, 5,
                [](double v, StaticString<64> &text) {
                  FormatUserMass(Units::ToSysMass(v), text.data(), true);
                }))
    return;

  plane.empty_mass = Units::ToSysMass(value);
  Refresh();
}

void
PlaneDetails::EditMaxBallast() noexcept
{
  double value = plane.max_ballast;
  if (!PickList(_("Max. Ballast"), value, 0, 500, 5,
                [](double v, StaticString<64> &text) {
                  text.Format("%.0f l", v);
                }))
    return;

  plane.max_ballast = value;
  Refresh();
}

void
PlaneDetails::EditDumpTime() noexcept
{
  double value = plane.dump_time;
  if (!PickList(_("Dump Time"), value, 0, 300, 5,
                [](double v, StaticString<64> &text) {
                  text.Format("%u s", (unsigned)v);
                }))
    return;

  plane.dump_time = (unsigned)value;
  Refresh();
}

void
PlaneDetails::EditMaxSpeed() noexcept
{
  double value = Units::ToUserSpeed(plane.max_speed);
  if (!PickList(_("Max. Cruise Speed"), value, 0, 300, 5,
                [](double v, StaticString<64> &text) {
                  FormatUserSpeed(Units::ToSysSpeed(v), text.data(), true);
                }))
    return;

  plane.max_speed = Units::ToSysSpeed(value);
  Refresh();
}

void
PlaneDetails::EditWeGlideType() noexcept
{
  unsigned id = plane.weglide_glider_type;
  if (!SelectWeGlideAircraftType(id,
                                 CommonInterface::GetComputerSettings()
                                   .weglide))
    return;

  plane.weglide_glider_type = id;
  Refresh();
}

void
PlaneDetails::Build() noexcept
{
  list->AddGroup(nullptr);

  list->AddValue(_("Registration"), nullptr,
                 [this](ValueState &state) {
                   state.text = plane.registration.c_str();
                 },
                 [this]{ EditRegistration(); });
  list->AddValue(_("Comp. ID"), nullptr,
                 [this](ValueState &state) {
                   state.text = plane.competition_id.c_str();
                 },
                 [this]{ EditCompetitionId(); });
  list->AddValue(_("Polar"), nullptr,
                 [this](ValueState &state) {
                   state.text = plane.polar_name.c_str();
                 },
                 [this]{ EditPolar(); });
  list->AddValue(_("Type"), nullptr,
                 [this](ValueState &state) {
                   state.text = plane.type.c_str();
                 },
                 [this]{ EditType(); });
  list->AddValue(_("Handicap"), nullptr,
                 [this](ValueState &state) {
                   StaticString<64> text;
                   text.Format("%u %%", plane.handicap);
                   state.text = text.c_str();
                 },
                 [this]{ EditHandicap(); });
  list->AddValue(_("Wing Area"), nullptr,
                 [this](ValueState &state) {
                   if (plane.wing_area > 0) {
                     StaticString<64> text;
                     text.Format("%.1f m²", plane.wing_area);
                     state.text = text.c_str();
                   } else {
                     state.badge = _("No wing area (m²)");
                     state.badge_style = BadgeStyle::DANGER;
                   }
                 },
                 [this]{ EditWingArea(); });
  list->AddValue(_("Empty Mass"),
                 _("Net mass of the rigged plane."),
                 [this](ValueState &state) {
                   if (plane.empty_mass > 0) {
                     StaticString<64> text;
                     FormatUserMass(plane.empty_mass, text.data(), true);
                     state.text = text.c_str();
                   } else {
                     state.badge = _("No empty mass");
                     state.badge_style = BadgeStyle::DANGER;
                   }
                 },
                 [this]{ EditEmptyMass(); });
  list->AddValue(_("Max. Ballast"), nullptr,
                 [this](ValueState &state) {
                   if (plane.max_ballast > 0) {
                     StaticString<64> text;
                     text.Format("%.0f l", plane.max_ballast);
                     state.text = text.c_str();
                   } else {
                     state.badge = _("No max ballast");
                     state.badge_style = BadgeStyle::WARNING;
                   }
                 },
                 [this]{ EditMaxBallast(); });
  list->AddValue(_("Dump Time"),
                 _("Seconds to empty the ballast tanks. Set to 0 for no "
                   "dump time."),
                 [this](ValueState &state) {
                   if (plane.dump_time > 0) {
                     StaticString<64> text;
                     text.Format("%u s", plane.dump_time);
                     state.text = text.c_str();
                   } else {
                     state.badge = _("No dump time");
                     state.badge_style = BadgeStyle::WARNING;
                   }
                 },
                 [this]{ EditDumpTime(); });
  list->AddValue(_("Max. Cruise Speed"),
                 _("Upper limit for MacCready speed-to-fly, including final "
                   "glide. Prevents the glide computer from commanding "
                   "unrealistically high cruise speeds. A typical choice is "
                   "the rough-air / green-arc limit from the flight manual."),
                 [this](ValueState &state) {
                   if (plane.max_speed > 0) {
                     StaticString<64> text;
                     FormatUserSpeed(plane.max_speed, text.data(), true);
                     state.text = text.c_str();
                   } else {
                     state.badge = _("No max cruise speed");
                     state.badge_style = BadgeStyle::WARNING;
                   }
                 },
                 [this]{ EditMaxSpeed(); });
  list->AddValue(_("WeGlide Aircraft"), nullptr,
                 [this](ValueState &state) {
                   if (plane.weglide_glider_type == 0) {
                     state.badge = _("No WeGlide type");
                     state.badge_style = BadgeStyle::WARNING;
                     return;
                   }

                   StaticString<128> aircraft;
                   StaticString<96> name;
                   if (WeGlide::LookupAircraftTypeName(
                         plane.weglide_glider_type, name))
                     aircraft.Format("%s (%u)", name.c_str(),
                                     plane.weglide_glider_type);
                   else
                     aircraft.Format("%s (%u)", _("Unknown"),
                                     plane.weglide_glider_type);
                   state.text = aircraft.c_str();
                 },
                 [this]{ EditWeGlideType(); });
}

void
PlaneDetails::Refresh() noexcept
{
  if (list->GetItemCount() == 0)
    Build();

  UpdateCaption();
  if (list->UpdateValues() && dialog != nullptr)
    dialog->RefitList();
}

} // namespace

bool
dlgPlaneDetailsShowModal(Plane &_plane) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  auto *list = new GroupedListWidget(look);
  PlaneDetails details(_plane);
  details.SetList(*list);

  StaticString<128> caption;
  caption.Format("%s: %s", _("Plane Details"),
                 _plane.registration.c_str());

  WidgetDialog dialog(WidgetDialog::Floating{}, UIGlobals::GetMainWindow(),
                      look, caption, list);
  details.SetDialog(dialog);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.PrepareFloatingList();
  details.Refresh();

  const int result = dialog.ShowModal();
  if (result != mrOK)
    return false;

  _plane = details.GetValue();
  return true;
}
