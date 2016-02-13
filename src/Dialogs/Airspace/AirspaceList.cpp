/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Airspace.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Airspace/AirspaceSorter.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/List.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Prefix.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Renderer/AirspaceListRenderer.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "Look/MapLook.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Compiler.h"
#include "Util/Macros.hpp"
#include "Units/Units.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Language/Language.hpp"
#include "Util/StringCompare.hxx"

#include <assert.h>
#include <stdio.h>

enum Controls {
  NAME,
  DISTANCE,
  DIRECTION,
  TYPE,
};

enum Buttons {
  DETAILS,
};

class AirspaceFilterWidget;

class AirspaceListWidget final
  : public ListWidget, public DataFieldListener,
    public ActionListener, NullBlackboardListener {
  AirspaceFilterWidget &filter_widget;

  AirspaceSelectInfoVector items;

  TwoTextRowsRenderer row_renderer;

public:
  AirspaceListWidget(AirspaceFilterWidget &_filter_widget)
    :filter_widget(_filter_widget) {}

  void UpdateList();
  void FilterMode(bool direction);
  void OnAirspaceListEnter(unsigned index);

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;

  virtual void Unprepare() override {
    DeleteWindow();
  }

  virtual void Show(const PixelRect &rc) override {
    ListWidget::Show(rc);
    UpdateList();
    CommonInterface::GetLiveBlackboard().AddListener(*this);
  }

  virtual void Hide() override {
    CommonInterface::GetLiveBlackboard().RemoveListener(*this);

    ListWidget::Hide();
  }

  /* virtual methods from ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;

  /* virtual methods from ListCursorHandler */
  virtual bool CanActivateItem(unsigned index) const override {
    return true;
  }

  virtual void OnActivateItem(unsigned index) override;

  /* virtual methods from ActionListener */
  virtual void OnAction(int id) override;

  /* virtual methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;

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
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;
};

class AirspaceListButtons final : public RowFormWidget {
  ActionListener &dialog;
  ActionListener *list;

public:
  AirspaceListButtons(const DialogLook &look, ActionListener &_dialog)
    :RowFormWidget(look), dialog(_dialog) {}

  void SetList(ActionListener *_list) {
    list = _list;
  }

  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override {
    AddButton(_("Details"), *list, DETAILS);
    AddButton(_("Close"), dialog, mrCancel);
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
  { OTHER, _T("Other") },
  { RESTRICT, _T("Restricted areas") },
  { PROHIBITED, _T("Prohibited areas") },
  { DANGER, _T("Danger areas") },
  { CLASSA, _T("Class A") },
  { CLASSB, _T("Class B") },
  { CLASSC, _T("Class C") },
  { CLASSD, _T("Class D") },
  { NOGLIDER, _T("No gliders") },
  { CTR, _T("CTR") },
  { WAVE, _T("Wave") },
  { CLASSE, _T("Class E") },
  { CLASSF, _T("Class F") },
  { TMZ, _T("TMZ") },
  { MATZ, _T("MATZ") },
  { 0 }
};

struct AirspaceListWidgetState
{
  double distance;
  unsigned direction;
  unsigned type;

  AirspaceListWidgetState()
    :distance(-1), direction(WILDCARD), type(WILDCARD) {}
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

  dlgAirspaceDetails(items[i].GetAirspace(), airspace_warnings);
}

void
AirspaceListWidget::OnActivateItem(unsigned index)
{
  OnAirspaceListEnter(index);
}

void
AirspaceListWidget::OnAction(int id)
{
  switch (Buttons(id)) {
  case DETAILS:
    OnAirspaceListEnter(GetList().GetCursorIndex());
    break;
  }
}

void
AirspaceListWidget::UpdateList()
{
  AirspaceFilterData data;
  data.Clear();

  if (dialog_state.type != WILDCARD)
    data.cls = (AirspaceClass)dialog_state.type;

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
AirspaceListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
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
AirspaceListWidget::OnModified(DataField &df)
{
  if (filter_widget.IsDataField(DISTANCE, df)) {
    DataFieldEnum &dfe = (DataFieldEnum &)df;
    dialog_state.distance = dfe.GetValue() != WILDCARD
      ? Units::ToSysDistance(dfe.GetValue())
      : -1.;

  } else if (filter_widget.IsDataField(DIRECTION, df)) {
    DataFieldEnum &dfe = (DataFieldEnum &)df;
    dialog_state.direction = dfe.GetValue();

  } else if (filter_widget.IsDataField(TYPE, df)) {
    DataFieldEnum &dfe = (DataFieldEnum &)df;
    dialog_state.type = dfe.GetValue();
  }

  FilterMode(filter_widget.IsDataField(NAME, df));
  UpdateList();
}

void
AirspaceListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                unsigned i)
{
  if (items.empty()) {
    assert(i == 0);

    canvas.DrawText(rc.left + Layout::GetTextPadding(),
                    rc.top + Layout::GetTextPadding(), _("No Match!"));
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

gcc_pure
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
    if (a.AsDelta().AbsoluteDegrees() >= 10) {
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

  df.Set(0u);
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

  df.Set(WILDCARD);
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
AirspaceFilterWidget::Prepare(ContainerWindow &parent,
                              const PixelRect &rc)
{
  Add(_("Name"), nullptr, CreateNameDataField(listener));
  Add(_("Distance"), nullptr, CreateDistanceDataField(listener));
  Add(_("Direction"), nullptr, CreateDirectionDataField(listener));
  AddEnum(_("Type"), nullptr, type_filter_list, WILDCARD, listener);
}

void
ShowAirspaceListDialog(const Airspaces &_airspaces,
                       ProtectedAirspaceWarningManager *_airspace_warnings)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  airspace_warnings = _airspace_warnings;
  airspaces = &_airspaces;
  location = CommonInterface::Basic().location;

  WidgetDialog dialog(look);

  AirspaceFilterWidget *filter_widget = new AirspaceFilterWidget(look);

  AirspaceListButtons *buttons_widget = new AirspaceListButtons(look, dialog);

  TwoWidgets *left_widget =
    new TwoWidgets(filter_widget, buttons_widget, true);

  AirspaceListWidget *const list_widget =
    new AirspaceListWidget(*filter_widget);

  filter_widget->SetListener(list_widget);
  buttons_widget->SetList(list_widget);

  TwoWidgets *widget = new TwoWidgets(left_widget, list_widget, false);

  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Select Airspace"), widget);
  dialog.ShowModal();
}

