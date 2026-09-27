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
#include "Renderer/BoxShadowRenderer.hpp"
#include "Screen/Layout.hpp"
#include "util/StaticString.hxx"

#include <algorithm>

namespace {

/**
 * A dialog which stays smaller than the screen, so it floats, and
 * which measures its list again when the layout changes.
 */
class PlaneDetailsDialog final : public WidgetDialog {
  bool fitting = false;

  static unsigned PreferredClientWidth(const DialogLook &look) noexcept {
    const unsigned speed =
      look.list.font->TextSize("Max. Cruise Speed   000 km/h").width;
    const unsigned help = look.list.font->TextSize(
      "Seconds to empty the ballast tanks.").width;
    const unsigned text = std::max(speed, help);
    return text + 2 * Layout::VptScale(10) +
      2 * Layout::GetTextPadding();
  }

  static unsigned OuterLimit(unsigned parent, unsigned inset) noexcept {
    if (parent <= inset * 2)
      return parent > 1 ? parent - 1 : parent;

    return parent - 2 * inset;
  }

public:
  using WidgetDialog::WidgetDialog;

  void Fit(const PixelRect &parent_rc) noexcept {
    if (fitting)
      return;

    fitting = true;
    FitTo(parent_rc);
    fitting = false;
  }

  void ReinitialiseLayout(const PixelRect &parent_rc) noexcept override {
    Fit(parent_rc);
  }

  /* keep the strip under the list, including when the dialog is
     wider than it is tall */
  PixelRect LayoutButtons() noexcept override {
    return GetButtonPanel().BottomLayout();
  }

private:
  void FitTo(const PixelRect &parent_rc) noexcept;
};

void
PlaneDetailsDialog::FitTo(const PixelRect &parent_rc) noexcept
{
  auto &list = static_cast<GroupedListWidget &>(GetWidget());

  const unsigned parent_w = parent_rc.GetWidth();
  const unsigned parent_h = parent_rc.GetHeight();
  unsigned inset = BoxShadowExtent(BoxShadowStyle::DIALOG);
  if (inset < 1)
    inset = 1;

  const unsigned max_w = OuterLimit(parent_w, inset);
  const unsigned max_h = OuterLimit(parent_h, inset);
  const unsigned frame_w = ClientAreaToDialogSize({}).width;
  const unsigned frame_h = ClientAreaToDialogSize({}).height;

  unsigned client_w = PreferredClientWidth(GetLook());
  if (client_w + frame_w > max_w)
    client_w = max_w > frame_w ? max_w - frame_w : max_w;

  unsigned client_h = max_h > frame_h ? max_h - frame_h : 1;
  const unsigned min_client_h = Layout::GetMaximumControlHeight();

  for (unsigned pass = 0; pass < 2; ++pass) {
    const PixelSize outer = ClientAreaToDialogSize({client_w, client_h});
    if (GetSize() != outer)
      Resize(outer);

    const PixelRect widget_rc = LayoutButtons();
    list.Move(widget_rc);
    list.UpdateLayout();

    unsigned content = list.GetContentHeight();
    if (content == 0)
      content = list.GetMinimumSize().height;

    const unsigned widget_h = widget_rc.GetHeight();
    if (widget_h > content && client_h > content) {
      const unsigned spare = widget_h - content;
      client_h = client_h > spare ? client_h - spare : content;
    }

    if (client_h < min_client_h)
      client_h = min_client_h;

    if (client_h + frame_h > max_h)
      client_h = max_h > frame_h ? max_h - frame_h : client_h;
  }

  const PixelSize size = GetSize();
  int x = (int)parent_rc.left +
    ((int)parent_w - (int)size.width) / 2;
  int y = (int)parent_rc.top +
    ((int)parent_h - (int)size.height) / 2;
  if (x < (int)parent_rc.left)
    x = parent_rc.left;
  if (y < (int)parent_rc.top)
    y = parent_rc.top;

  Move(PixelPoint{x, y});
}

class PlaneDetails {
  using Callback = GroupedListWidget::Callback;

  GroupedListWidget *list = nullptr;
  PlaneDetailsDialog *dialog = nullptr;
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

  void SetDialog(PlaneDetailsDialog &_dialog) noexcept {
    dialog = &_dialog;
  }

  void Refresh() noexcept;
  void Refit() noexcept;

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

  void AddRow(const char *caption, const char *help,
              const char *text, Callback edit,
              const char *badge = nullptr,
              GroupedListWidget::BadgeStyle badge_style =
                GroupedListWidget::BadgeStyle::PRIMARY) noexcept;

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

void
PlaneDetails::AddRow(const char *caption, const char *help,
                     const char *text, Callback edit,
                     const char *badge,
                     GroupedListWidget::BadgeStyle badge_style) noexcept
{
  /* a tap on the label selects the row and draws an arrow.  A tap
     on the value opens it.  Without an action the row stays grey
     and the cursor may rest on it so its help can be read.  A badge
     on that row says why it cannot be used. */
  GroupedListWidget::ItemOptions options;
  options.help = help;
  options.value = text;
  options.badge_style = badge_style;
  options.chevron = static_cast<bool>(edit);
  options.label_selects = static_cast<bool>(edit);

  if (edit) {
    options.badge = badge;
    list->AddItem(caption, std::move(edit), options);
  } else {
    options.disabled = true;
    options.selectable_when_disabled = true;
    options.disabled_badge_label = badge;
    list->AddItem(caption, options);
  }
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
PlaneDetails::Refresh() noexcept
{
  list->Clear();

  list->AddGroup(nullptr);

  StaticString<64> text;

  AddRow(_("Registration"), nullptr, plane.registration.c_str(),
         [this]{ EditRegistration(); });
  AddRow(_("Comp. ID"), nullptr, plane.competition_id.c_str(),
         [this]{ EditCompetitionId(); });
  AddRow(_("Polar"), nullptr, plane.polar_name.c_str(),
         [this]{ EditPolar(); });
  AddRow(_("Type"), nullptr, plane.type.c_str(),
         [this]{ EditType(); });

  text.Format("%u %%", plane.handicap);
  AddRow(_("Handicap"), nullptr, text.c_str(),
         [this]{ EditHandicap(); });

  if (plane.wing_area > 0) {
    text.Format("%.1f m²", plane.wing_area);
    AddRow(_("Wing Area"), nullptr, text.c_str(),
           [this]{ EditWingArea(); });
  } else
    AddRow(_("Wing Area"), nullptr, nullptr,
           [this]{ EditWingArea(); },
           _("No wing area (m²)"),
           GroupedListWidget::BadgeStyle::DANGER);

  if (plane.empty_mass > 0) {
    FormatUserMass(plane.empty_mass, text.data(), true);
    AddRow(_("Empty Mass"), _("Net mass of the rigged plane."),
           text.c_str(), [this]{ EditEmptyMass(); });
  } else
    AddRow(_("Empty Mass"), _("Net mass of the rigged plane."),
           nullptr, [this]{ EditEmptyMass(); },
           _("No empty mass"),
           GroupedListWidget::BadgeStyle::DANGER);

  if (plane.max_ballast > 0) {
    text.Format("%.0f l", plane.max_ballast);
    AddRow(_("Max. Ballast"), nullptr, text.c_str(),
           [this]{ EditMaxBallast(); });
  } else
    AddRow(_("Max. Ballast"), nullptr, nullptr,
           [this]{ EditMaxBallast(); },
           _("No max ballast"),
           GroupedListWidget::BadgeStyle::WARNING);

  if (plane.dump_time > 0) {
    text.Format("%u s", plane.dump_time);
    AddRow(_("Dump Time"),
           _("Seconds to empty the ballast tanks. Set to 0 for no "
             "dump time."),
           text.c_str(), [this]{ EditDumpTime(); });
  } else
    AddRow(_("Dump Time"),
           _("Seconds to empty the ballast tanks. Set to 0 for no "
             "dump time."),
           nullptr, [this]{ EditDumpTime(); },
           _("No dump time"),
           GroupedListWidget::BadgeStyle::WARNING);

  if (plane.max_speed > 0) {
    FormatUserSpeed(plane.max_speed, text.data(), true);
    AddRow(_("Max. Cruise Speed"),
           _("Upper limit for MacCready speed-to-fly, including final "
             "glide. Prevents the glide computer from commanding "
             "unrealistically high cruise speeds. A typical choice is "
             "the rough-air / green-arc limit from the flight manual."),
           text.c_str(), [this]{ EditMaxSpeed(); });
  } else
    AddRow(_("Max. Cruise Speed"),
           _("Upper limit for MacCready speed-to-fly, including final "
             "glide. Prevents the glide computer from commanding "
             "unrealistically high cruise speeds. A typical choice is "
             "the rough-air / green-arc limit from the flight manual."),
           nullptr, [this]{ EditMaxSpeed(); },
           _("No max cruise speed"),
           GroupedListWidget::BadgeStyle::WARNING);

  if (plane.weglide_glider_type == 0)
    AddRow(_("WeGlide Aircraft"), nullptr, nullptr,
           [this]{ EditWeGlideType(); },
           _("No WeGlide type"),
           GroupedListWidget::BadgeStyle::WARNING);
  else {
    StaticString<128> aircraft;
    StaticString<96> name;
    if (WeGlide::LookupAircraftTypeName(plane.weglide_glider_type,
                                        name))
      aircraft.Format("%s (%u)", name.c_str(),
                      plane.weglide_glider_type);
    else
      aircraft.Format("%s (%u)", _("Unknown"),
                      plane.weglide_glider_type);
    AddRow(_("WeGlide Aircraft"), nullptr, aircraft.c_str(),
           [this]{ EditWeGlideType(); });
  }

  UpdateCaption();
  list->UpdateLayout();
  Refit();
}

void
PlaneDetails::Refit() noexcept
{
  if (dialog != nullptr)
    dialog->Fit(dialog->GetParentClientRect());
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

  const PixelRect rc{Layout::Scale(PixelSize{220u, 220u})};
  PlaneDetailsDialog dialog(UIGlobals::GetMainWindow(), look, rc,
                            caption, list);
  details.SetDialog(dialog);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.EnableCursorSelection();
  dialog.ResyncButtonPanelSelection();
  list->SetActionBar(dialog.GetButtonPanel());
  list->SetCursorCallback([&details](int){
    details.Refit();
  });

  dialog.PrepareWidget();
  details.Refresh();

  const int result = dialog.ShowModal();
  if (result != mrOK)
    return false;

  _plane = details.GetValue();
  return true;
}
