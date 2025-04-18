// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Airspace/AirspaceSorter.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/RowFormWidget.hpp"
#include "ui/control/List.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Prefix.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Renderer/AirspaceListRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Look/MapLook.hpp"
#include "Look/DialogLook.hpp"
#include "util/Compiler.h"
#include "util/Macros.hpp"
#include "Units/Units.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Language/Language.hpp"
#include "util/StringCompare.hxx"

#include <cassert>
#include <stdio.h>

enum Controls {
  NAME,
  DISTANCE,
  DIRECTION,
  CLASS_AND_TYPE,
};

class AirspaceFilterWidget;

class AirspaceListWidget final
  : public ListWidget, public DataFieldListener,
    NullBlackboardListener {
  AirspaceFilterWidget &filter_widget;

  AirspaceSelectInfoVector items;

  TwoTextRowsRenderer row_renderer;

public:
  AirspaceListWidget(AirspaceFilterWidget &_filter_widget)
    :filter_widget(_filter_widget) {}

  void UpdateList();
  void FilterMode(bool direction);
  void OnAirspaceListEnter(unsigned index);

  void ShowDetails() noexcept {
    OnAirspaceListEnter(GetList().GetCursorIndex());
  }

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

  void Show(const PixelRect &rc) noexcept override {
    ListWidget::Show(rc);
    UpdateList();
    CommonInterface::GetLiveBlackboard().AddListener(*this);
  }

  void Hide() noexcept override {
    CommonInterface::GetLiveBlackboard().RemoveListener(*this);

    ListWidget::Hide();
  }

  /* virtual methods from ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from ListCursorHandler */
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override;

  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;

private:
  /* virtual methods from BlackboardListener */
  virtual void OnGPSUpdate(const MoreData &basic) override;
};

class AirspaceFilterWidget final : public RowFormWidget {
  DataFieldListener *listener;

public:
  AirspaceFilterWidget(const DialogLook &look)
    :RowFormWidget(look, true) {}

  void SetListener(DataFieldListener *_listener) {
    listener = _listener;
  }

  void Update();

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

class AirspaceListButtons final : public RowFormWidget {
  WndForm &dialog;
  AirspaceListWidget *list;

public:
  AirspaceListButtons(const DialogLook &look, WndForm &_dialog) noexcept
    :RowFormWidget(look), dialog(_dialog) {}

  void SetList(AirspaceListWidget *_list) {
    list = _list;
  }

  void Prepare([[maybe_unused]] ContainerWindow &parent,
               [[maybe_unused]] const PixelRect &rc) noexcept override {
    AddButton(_("Details"), [this](){
      list->ShowDetails();
    });

    AddButton(_("Close"), dialog.MakeModalResultCallback(mrCancel));
  }
};

/**
 * Special enum integer value for "filter disabled".
 */
static constexpr unsigned WILDCARD = 0x7fff;

static const Airspaces *airspaces;
static ProtectedAirspaceWarningManager *airspace_warnings;

static GeoPoint location;

static Angle last_heading;

static constexpr StaticEnumChoice type_filter_list[] = {
  { WILDCARD, _T("*") },
  { OTHER, _T("Unknown") },
  { RESTRICTED, _T("Restricted areas") },
  { PROHIBITED, _T("Prohibited areas") },
  { DANGER, _T("Danger areas") },
  { CLASSA, _T("Class A") },
  { CLASSB, _T("Class B") },
  { CLASSC, _T("Class C") },
  { CLASSD, _T("Class D") },
  { NOGLIDER, _T("No gliders") },
  { CTR, _T("CTR") },
  { WAVE, _T("Wave") },
  { AATASK, _T("Task Area") },
  { CLASSE, _T("Class E") },
  { CLASSF, _T("Class F") },
  { TMZ, _T("TMZ") },
  { CLASSG, _T("Class G") },
  { MATZ, _T("MATZ") },
  { RMZ, _T("RMZ") },
  { UNCLASSIFIED, _T("UNCLASSIFIED") },
  { TMA, _T("TMA") },
  { TRA, _T("TRA") },
  { TSA, _T("TSA") },
  { FIR, _T("FIR") },
  { UIR, _T("UIR") },
  { ADIZ, _T("ADIZ") },
  { ATZ, _T("ATZ") },
  { AWY, _T("AWY") },
  { MTR, _T("MTR") },
  { ALERT, _T("ALERT") },
  { WARNING, _T("WARNING") },
  { PROTECTED, _T("PROTECTED") },
  { HTZ, _T("HTZ") },
  { GLIDING_SECTOR, _T("Gliding Sector") },
  { TRP, _T("TRP") },
  { TIZ, _T("TIZ") },
  { TIA, _T("TIA") },
  { MTA, _T("MTA") },
  { CTA, _T("CTA") },
  { ACC_SECTOR, _T("ACC Sector") },
  { AERIAL_SPORTING_RECREATIONAL, _T("Aerial Sporting Recreational") },
  { OVERFLIGHT_RESTRICTION, _T("Overflight Restriction") },
  { MRT, _T("MRT") },
  { TFR, _T("TFR") },
  { VFR_SECTOR, _T("VFR Sector") },
  { FIS_SECTOR, _T("FIS Sector") },
  { LTA, _T("Lower Traffic Area") },
  { UTA, _T("Upper Traffic Area") },
   nullptr
};

/* Remove two from type_filter list, as WILDCARD and nullptr are not
AirSpaceClasses */
static_assert(
    ARRAY_SIZE(type_filter_list) - 2 ==
        (size_t)AirspaceClass::AIRSPACECLASSCOUNT,
    "number of airspace class filter entries, does not match number of "
    "airspace classes");

struct AirspaceListWidgetState
{
  double distance;
  unsigned direction;
  unsigned class_and_type;

  AirspaceListWidgetState()
    :distance(-1), direction(WILDCARD), class_and_type(WILDCARD) {}
};

static AirspaceListWidgetState dialog_state;

void
AirspaceListWidget::OnAirspaceListEnter(unsigned i)
{
  if (items.empty()) {
    assert(i == 0);
    return;
  }

  assert(i < items.size());

  dlgAirspaceDetails(items[i].GetAirspacePtr(), airspace_warnings);
}

void
AirspaceListWidget::OnActivateItem(unsigned index) noexcept
{
  OnAirspaceListEnter(index);
}

void
AirspaceListWidget::UpdateList()
{
  AirspaceFilterData data;

  if (dialog_state.class_and_type != WILDCARD)
    data.cls = (AirspaceClass)dialog_state.class_and_type;

  const TCHAR *name_filter = filter_widget.GetValueString(NAME);
  if (!StringIsEmpty(name_filter))
    data.name_prefix = name_filter;

  if (dialog_state.direction != WILDCARD) {
    data.direction = dialog_state.direction == 0
      ? CommonInterface::Basic().attitude.heading
      : Angle::Degrees(dialog_state.direction);
  }

  if (dialog_state.distance > 0)
    data.distance = dialog_state.distance;

  items = FilterAirspaces(*airspaces,
                          CommonInterface::Basic().location,
                          data);

  GetList().SetLength(std::max((size_t)1, items.size()));
  GetList().Invalidate();
}

void
AirspaceListWidget::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font,
                                          look.small_font));
  UpdateList();
}

inline void
AirspaceListWidget::FilterMode(bool direction)
{
  if (direction) {
    dialog_state.distance = -1;
    dialog_state.direction = WILDCARD;

    filter_widget.LoadValueEnum(DISTANCE, WILDCARD);
    filter_widget.LoadValueEnum(DIRECTION, WILDCARD);
  } else {
    filter_widget.LoadValue(NAME, _T(""));
  }
}

void
AirspaceListWidget::OnModified(DataField &df) noexcept
{
  if (filter_widget.IsDataField(DISTANCE, df)) {
    DataFieldEnum &dfe = (DataFieldEnum &)df;
    dialog_state.distance = dfe.GetValue() != WILDCARD
      ? Units::ToSysDistance(dfe.GetValue())
      : -1.;

  } else if (filter_widget.IsDataField(DIRECTION, df)) {
    DataFieldEnum &dfe = (DataFieldEnum &)df;
    dialog_state.direction = dfe.GetValue();

  } else if (filter_widget.IsDataField(CLASS_AND_TYPE, df)) {
    DataFieldEnum &dfe = (DataFieldEnum &)df;
    dialog_state.class_and_type = dfe.GetValue();
  }

  FilterMode(filter_widget.IsDataField(NAME, df));
  UpdateList();
}

void
AirspaceListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                unsigned i) noexcept
{
  if (items.empty()) {
    assert(i == 0);

    row_renderer.DrawFirstRow(canvas, rc, _("No Match!"));
    return;
  }

  assert(i < items.size());

  const AbstractAirspace &airspace = items[i].GetAirspace();

  AirspaceListRenderer::Draw(
      canvas, rc, airspace,
      items[i].GetVector(location, airspaces->GetProjection()),
      row_renderer, UIGlobals::GetMapLook().airspace,
      CommonInterface::GetMapSettings().airspace);
}

[[gnu::pure]]
static const TCHAR *
GetHeadingString(TCHAR *buffer)
{
  TCHAR heading[32];
  FormatBearing(heading, ARRAY_SIZE(heading),
                CommonInterface::Basic().attitude.heading);

  StringFormatUnsafe(buffer, _T("%s (%s)"), _("Heading"), heading);
  return buffer;
}

void
AirspaceListWidget::OnGPSUpdate(const MoreData &basic)
{
  if (dialog_state.direction == 0 && !CommonInterface::Calculated().circling) {
    const Angle heading = basic.attitude.heading;
    Angle a = last_heading - heading;
    if (a.AsDelta().Absolute() >= Angle::Degrees(10)) {
      last_heading = heading;

      UpdateList();
      filter_widget.Update();
    }
  }
}

inline void
AirspaceFilterWidget::Update()
{
  WndProperty &direction_control = GetControl(DIRECTION);
  DataFieldEnum &direction_df = *(DataFieldEnum *)
    direction_control.GetDataField();

  TCHAR buffer[64];
  direction_df.replaceEnumText(0, GetHeadingString(buffer));
  direction_control.RefreshDisplay();
}

static void
FillDistanceEnum(DataFieldEnum &df)
{
  df.AddChoice(0, _T("*"));

  static constexpr unsigned distances[] = {
    25, 50, 75, 100, 150, 250, 500, 1000
  };

  TCHAR buffer[64];
  const TCHAR *unit = Units::GetDistanceName();
  for (unsigned i = 0; i < ARRAY_SIZE(distances); ++i) {
    StringFormatUnsafe(buffer, _T("%u %s"), distances[i], unit);
    df.AddChoice(distances[i], buffer);
  }

  df.SetValue(0u);
}

static void
FillDirectionEnum(DataFieldEnum &df)
{
  TCHAR buffer[64];

  df.AddChoice(WILDCARD, _T("*"));
  df.AddChoice(0, GetHeadingString(buffer));

  static constexpr unsigned directions[] = {
    360, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330
  };

  for (unsigned i = 0; i < ARRAY_SIZE(directions); ++i)
    df.AddChoice(directions[i], FormatBearing(directions[i]).c_str());

  df.SetValue(WILDCARD);
}

static DataField *
CreateNameDataField(DataFieldListener *listener)
{
  return new PrefixDataField(_T(""), listener);
}

static DataField *
CreateDistanceDataField(DataFieldListener *listener)
{
  DataFieldEnum *df = new DataFieldEnum(listener);
  FillDistanceEnum(*df);
  return df;
}

static DataField *
CreateDirectionDataField(DataFieldListener *listener)
{
  DataFieldEnum *df = new DataFieldEnum(listener);
  FillDirectionEnum(*df);
  return df;
}

void
AirspaceFilterWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                              [[maybe_unused]] const PixelRect &rc) noexcept
{
  Add(_("Name"), nullptr, CreateNameDataField(listener));
  Add(_("Distance"), nullptr, CreateDistanceDataField(listener));
  Add(_("Direction"), nullptr, CreateDirectionDataField(listener));
  AddEnum(_("Class/Type"), nullptr, type_filter_list, WILDCARD, listener);
}

void
ShowAirspaceListDialog(const Airspaces &_airspaces,
                       ProtectedAirspaceWarningManager *_airspace_warnings)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  airspace_warnings = _airspace_warnings;
  airspaces = &_airspaces;
  location = CommonInterface::Basic().location;

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Select Airspace"));

  auto filter_widget = std::make_unique<AirspaceFilterWidget>(look);

  auto list_widget = std::make_unique<AirspaceListWidget>(*filter_widget);

  auto buttons_widget = std::make_unique<AirspaceListButtons>(look, dialog);

  filter_widget->SetListener(list_widget.get());
  buttons_widget->SetList(list_widget.get());

  auto left_widget = std::make_unique<TwoWidgets>(std::move(filter_widget),
                                                  std::move(buttons_widget),
                                                  true);

  dialog.FinishPreliminary(new TwoWidgets(std::move(left_widget),
                                          std::move(list_widget), false));
  dialog.ShowModal();
}
