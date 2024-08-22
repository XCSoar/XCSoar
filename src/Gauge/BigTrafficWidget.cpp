// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BigTrafficWidget.hpp"
#include "Dialogs/Traffic/TrafficDialogs.hpp"
#include "Math/Screen.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "Form/Button.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "UIState.hpp"
#include "UIGlobals.hpp"
#include "PageActions.hpp"
#include "Look/Look.hpp"
#include "Profile/Profile.hpp"
#include "FLARM/Friends.hpp"
#include "Look/FlarmTrafficLook.hpp"
#include "Gauge/FlarmTrafficWindow.hpp"
#include "Language/Language.hpp"
#include "UIUtil/GestureManager.hpp"
#include "Formatter/UserUnits.hpp"
#include "Renderer/UnitSymbolRenderer.hpp"
#include "Input/InputEvents.hpp"
#include "Interface.hpp"
#include "Asset.hpp"

/**
 * A Window which renders FLARM traffic, with user interaction.
 */
class FlarmTrafficControl : public FlarmTrafficWindow {
protected:
  bool enable_auto_zoom = true, dragging = false;
  unsigned zoom = 3;
  static constexpr unsigned num_zoom_options = 5;
  Angle task_direction = Angle::Degrees(-1);
  GestureManager gestures;

public:
  FlarmTrafficControl(const FlarmTrafficLook &look)
    :FlarmTrafficWindow(look, Layout::Scale(10),
                        Layout::GetMinimumControlHeight() + Layout::Scale(10)) {}

protected:
  void CalcAutoZoom();

public:
  void Update(Angle new_direction, const TrafficList &new_data,
              const TeamCodeSettings &new_settings);
  void UpdateTaskDirection(bool show_task_direction, Angle bearing);

  bool GetNorthUp() const {
    return enable_north_up;
  }

  void SetNorthUp(bool enabled);

  void ToggleNorthUp() {
    SetNorthUp(!GetNorthUp());
  }

  bool GetAutoZoom() const {
    return enable_auto_zoom;
  }

  static unsigned GetZoomDistance(unsigned zoom);

  void SetZoom(unsigned _zoom) {
    zoom = _zoom;
    SetDistance(GetZoomDistance(_zoom));
  }

  void SetAutoZoom(bool enabled);

  void ToggleAutoZoom() {
    SetAutoZoom(!GetAutoZoom());
  }

  bool CanZoomOut() const {
    return zoom < num_zoom_options;
  }

  bool CanZoomIn() const {
    return zoom > 0;
  }

  void ZoomOut();
  void ZoomIn();

  void SwitchData();
  void OpenDetails();

protected:
  void PaintTrafficInfo(Canvas &canvas) const;
  void PaintClimbRate(Canvas &canvas, PixelRect rc, double climb_rate) const;
  void PaintDistance(Canvas &canvas, PixelRect rc, double distance) const;
  void PaintRelativeAltitude(Canvas &canvas, PixelRect rc,
                             double relative_altitude) const;
  void PaintID(Canvas &canvas, PixelRect rc, const FlarmTraffic &traffic) const;
  void PaintTaskDirection(Canvas &canvas) const;

  void StopDragging() {
    if (!dragging)
      return;

    dragging = false;
    ReleaseCapture();
  }

protected:
  bool OnMouseGesture(const TCHAR* gesture);

  /* virtual methods from class Window */
  void OnCreate() noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseDouble(PixelPoint p) noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
  void OnCancelMode() noexcept override;

  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;
};

void
FlarmTrafficControl::OnCreate() noexcept
{
  FlarmTrafficWindow::OnCreate();

  const TrafficSettings &settings = CommonInterface::GetUISettings().traffic;

  Profile::GetEnum(ProfileKeys::FlarmSideData, side_display_type);
  enable_auto_zoom = settings.auto_zoom;
  enable_north_up = settings.north_up;
}

unsigned
FlarmTrafficControl::GetZoomDistance(unsigned zoom)
{
  switch (zoom) {
  case 0:
    return 100;
  case 1:
    return 500;
  case 2:
    return 1000;
  case 4:
    return 5000;
  case 5:
    return 10000;
  case 3:
  default:
    return 2000;
  }
}

void
FlarmTrafficControl::SetNorthUp(bool enabled)
{
  TrafficSettings &settings = CommonInterface::SetUISettings().traffic;
  settings.north_up = enable_north_up = enabled;
  Profile::Set(ProfileKeys::FlarmNorthUp, enabled);
  //north_up->SetState(enabled);
}

void
FlarmTrafficControl::SetAutoZoom(bool enabled)
{
  TrafficSettings &settings = CommonInterface::SetUISettings().traffic;
  settings.auto_zoom = enable_auto_zoom = enabled;
  Profile::Set(ProfileKeys::FlarmAutoZoom, enabled);
  //auto_zoom->SetState(enabled);
}

void
FlarmTrafficControl::CalcAutoZoom()
{
  bool warning_mode = WarningMode();
  RoughDistance zoom_dist = 0;

  for (auto it = data.list.begin(), end = data.list.end();
      it != end; ++it) {
    if (warning_mode && !it->HasAlarm())
      continue;

    zoom_dist = std::max(it->distance, zoom_dist);
  }

  double zoom_dist2 = zoom_dist;
  for (unsigned i = 0; i <= num_zoom_options; i++) {
    if (i == num_zoom_options || GetZoomDistance(i) >= zoom_dist2) {
      SetZoom(i);
      break;
    }
  }
}

void
FlarmTrafficControl::Update(Angle new_direction, const TrafficList &new_data,
                            const TeamCodeSettings &new_settings)
{
  FlarmTrafficWindow::Update(new_direction, new_data, new_settings);

  if (enable_auto_zoom || WarningMode())
    CalcAutoZoom();
}

void
FlarmTrafficControl::UpdateTaskDirection(bool show_task_direction, Angle bearing)
{
  if (!show_task_direction)
    task_direction = Angle::Degrees(-1);
  else
    task_direction = bearing.AsBearing();
}

/**
 * Zoom out one step
 */
void
FlarmTrafficControl::ZoomOut()
{
  if (WarningMode())
    return;

  if (zoom < num_zoom_options)
    SetZoom(zoom + 1);

  SetAutoZoom(false);
}

/**
 * Zoom in one step
 */
void
FlarmTrafficControl::ZoomIn()
{
  if (WarningMode())
    return;

  if (zoom > 0)
    SetZoom(zoom - 1);

  SetAutoZoom(false);
}

/**
 * Paints an arrow into the direction of the current task leg
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficControl::PaintTaskDirection(Canvas &canvas) const
{
  if (task_direction.IsNegative())
    return;

  canvas.Select(look.radar_pen);
  canvas.SelectHollowBrush();

  BulkPixelPoint triangle[3];
  triangle[0].x = 0;
  triangle[0].y = -(int)radar_renderer.GetRadius() / Layout::FastScale(1) + 15;
  triangle[1].x = 7;
  triangle[1].y = triangle[0].y + 30;
  triangle[2].x = -triangle[1].x;
  triangle[2].y = triangle[1].y;

  PolygonRotateShift(triangle, radar_renderer.GetCenter(),
                     task_direction - (enable_north_up ?
                                       Angle::Zero() : heading),
                     Layout::FastScale(100u));

  // Draw the arrow
  canvas.DrawPolygon(triangle, 3);
}

void
FlarmTrafficControl::PaintClimbRate(Canvas &canvas, PixelRect rc,
                                    double climb_rate) const
{
  // Paint label
  canvas.Select(look.info_labels_font);
  const unsigned label_width = canvas.CalcTextSize(_("Vario")).width;
  canvas.DrawText(rc.GetTopRight().At(-(int)label_width, 0), _("Vario"));

  // Format climb rate
  Unit unit = Units::GetUserVerticalSpeedUnit();
  const auto buffer = FormatUserVerticalSpeed(climb_rate, false);

  // Calculate unit size
  canvas.Select(look.info_units_font);
  const unsigned unit_width = UnitSymbolRenderer::GetSize(canvas, unit).width;
  const unsigned unit_height =
      UnitSymbolRenderer::GetAscentHeight(look.info_units_font, unit);

  unsigned space_width = unit_width / 3;

  // Calculate value size
  canvas.Select(look.info_values_font);
  const unsigned value_height = look.info_values_font.GetAscentHeight();
  const unsigned value_width = canvas.CalcTextSize(buffer.c_str()).width;

  // Calculate positions
  const int max_height = std::max(unit_height, value_height);
  const int y = rc.top + look.info_units_font.GetHeight() + max_height;

  const int unit_x = rc.right - unit_width;
  const int unit_y = y - unit_height;

  const int value_x = unit_x - space_width - value_width;
  const int value_y = y - value_height;

  // Paint value
  canvas.DrawText({value_x, value_y}, buffer.c_str());

  // Paint unit
  canvas.Select(look.info_units_font);
  UnitSymbolRenderer::Draw(canvas, {unit_x, unit_y},
                           unit, look.unit_fraction_pen);
}

void
FlarmTrafficControl::PaintDistance(Canvas &canvas, PixelRect rc,
                                   double distance) const
{
  // Format distance
  TCHAR buffer[20];
  Unit unit = FormatUserDistanceSmart(distance, buffer, false, 1000);

  // Calculate unit size
  canvas.Select(look.info_units_font);
  const unsigned unit_width = UnitSymbolRenderer::GetSize(canvas, unit).width;
  const unsigned unit_height =
      UnitSymbolRenderer::GetAscentHeight(look.info_units_font, unit);

  const unsigned space_width = unit_width / 3;

  // Calculate value size
  canvas.Select(look.info_values_font);
  const unsigned value_height = look.info_values_font.GetAscentHeight();
  const unsigned value_width = canvas.CalcTextSize(buffer).width;

  // Calculate positions
  const unsigned max_height = std::max(unit_height, value_height);

  const auto p0 = rc.GetBottomLeft();

  // Paint value
  canvas.DrawText(p0.At(0, -(int)value_height), buffer);

  // Paint unit
  canvas.Select(look.info_units_font);
  UnitSymbolRenderer::Draw(canvas,
                           p0.At(value_width + space_width,
                                 -(int)unit_height),
                           unit, look.unit_fraction_pen);


  // Paint label
  canvas.Select(look.info_labels_font);
  canvas.DrawText(p0.At(0, -int(max_height + look.info_labels_font.GetHeight())),
                  _("Distance"));
}

void
FlarmTrafficControl::PaintRelativeAltitude(Canvas &canvas, PixelRect rc,
                                           double relative_altitude) const
{
  // Format relative altitude
  TCHAR buffer[20];
  Unit unit = Units::GetUserAltitudeUnit();
  FormatRelativeUserAltitude(relative_altitude, buffer, false);

  // Calculate unit size
  canvas.Select(look.info_units_font);
  const unsigned unit_width = UnitSymbolRenderer::GetSize(canvas, unit).width;
  const unsigned unit_height =
      UnitSymbolRenderer::GetAscentHeight(look.info_units_font, unit);

  const unsigned space_width = unit_width / 3;

  // Calculate value size
  canvas.Select(look.info_values_font);
  const unsigned value_height = look.info_values_font.GetAscentHeight();
  const unsigned value_width = canvas.CalcTextSize(buffer).width;

  // Calculate positions
  const unsigned max_height = std::max(unit_height, value_height);

  const auto p0 = rc.GetBottomRight();

  // Paint value
  canvas.DrawText(p0.At(-int(unit_width + space_width + value_width),
                        -(int)value_height),
                  buffer);

  // Paint unit
  canvas.Select(look.info_units_font);
  UnitSymbolRenderer::Draw(canvas,
                           p0.At(-(int)unit_width, -(int)unit_height),
                           unit, look.unit_fraction_pen);


  // Paint label
  canvas.Select(look.info_labels_font);
  const unsigned label_width = canvas.CalcTextSize(_("Rel. Alt.")).width;
  canvas.DrawText(p0.At(-(int)label_width,  -int(max_height + look.info_labels_font.GetHeight())),
                  _("Rel. Alt."));
}

void
FlarmTrafficControl::PaintID(Canvas &canvas, PixelRect rc,
                             const FlarmTraffic &traffic) const
{
  TCHAR buffer[20];

  unsigned font_size;
  if (traffic.HasName()) {
    canvas.Select(look.call_sign_font);
    font_size = look.call_sign_font.GetHeight();

    _tcscpy(buffer, traffic.name);
  } else {
    canvas.Select(look.info_labels_font);
    font_size = look.info_labels_font.GetHeight();

    traffic.id.Format(buffer);
  }

  if (!WarningMode()) {
    // Team color dot
    FlarmColor team_color = FlarmFriends::GetFriendColor(traffic.id);

    // If team color found -> draw a colored circle in front of the name
    if (team_color != FlarmColor::NONE) {
      switch (team_color) {
      case FlarmColor::GREEN:
        canvas.Select(look.team_brush_green);
        break;
      case FlarmColor::BLUE:
        canvas.Select(look.team_brush_blue);
        break;
      case FlarmColor::YELLOW:
        canvas.Select(look.team_brush_yellow);
        break;
      case FlarmColor::MAGENTA:
        canvas.Select(look.team_brush_magenta);
        break;
      default:
        break;
      }

      canvas.SelectNullPen();
      canvas.DrawCircle(rc.GetTopLeft().At(Layout::FastScale(7u), (font_size / 2)),
                        Layout::FastScale(7u));

      rc.left += Layout::FastScale(16);
    }
  }

  canvas.DrawText(rc.GetTopLeft(), buffer);
}

/**
 * Paints the basic info for the selected target on the given canvas
 * @param canvas The canvas to paint on
 */
void
FlarmTrafficControl::PaintTrafficInfo(Canvas &canvas) const
{
  // Don't paint numbers if no plane selected
  if (selection == -1 && !WarningMode())
    return;

  // Shortcut to the selected traffic
  FlarmTraffic traffic = data.list[WarningMode() ? warning : selection];
  assert(traffic.IsDefined());

  const unsigned padding = Layout::GetTextPadding();
  PixelRect rc;
  rc.left = padding;
  rc.top = padding;
  rc.right = canvas.GetWidth() - padding;
  rc.bottom = canvas.GetHeight() - padding;

  // Set the text color and background
  switch (traffic.alarm_level) {
  case FlarmTraffic::AlarmType::LOW:
  case FlarmTraffic::AlarmType::INFO_ALERT:
    canvas.SetTextColor(look.warning_color);
    break;
  case FlarmTraffic::AlarmType::IMPORTANT:
  case FlarmTraffic::AlarmType::URGENT:
    canvas.SetTextColor(look.alarm_color);
    break;
  case FlarmTraffic::AlarmType::NONE:
    canvas.SetTextColor(look.default_color);
    break;
  }

  canvas.SetBackgroundTransparent();

  // Climb Rate
  if (!WarningMode() && traffic.climb_rate_avg30s_available)
    PaintClimbRate(canvas, rc, traffic.climb_rate_avg30s);

  // Distance
  PaintDistance(canvas, rc, traffic.distance);

  // Relative Height
  PaintRelativeAltitude(canvas, rc, traffic.relative_altitude);

  // ID / Name
  if (!traffic.HasAlarm())
    canvas.SetTextColor(look.selection_color);

  PaintID(canvas, rc, traffic);
}

void
FlarmTrafficControl::OnPaint(Canvas &canvas) noexcept
{
  canvas.Clear(look.background_color);

  PaintTaskDirection(canvas);
  FlarmTrafficWindow::Paint(canvas);
  PaintTrafficInfo(canvas);
}

void
FlarmTrafficControl::OpenDetails()
{
  // If warning is displayed -> prevent from opening details dialog
  if (WarningMode())
    return;

  // Don't open the details dialog if no plane selected
  const FlarmTraffic *traffic = GetTarget();
  if (traffic == NULL)
    return;

  // Show the details dialog
  dlgFlarmTrafficDetailsShowModal(traffic->id);
}

static Button
MakeSymbolButton(ContainerWindow &parent, const ButtonLook &look,
                const TCHAR *caption,
                const PixelRect &rc,
                Button::Callback callback) noexcept
{
  return Button(parent, rc, WindowStyle(),
                std::make_unique<SymbolButtonRenderer>(look, caption),
                std::move(callback));
}

struct TrafficWidget::Windows {
  Button zoom_in_button, zoom_out_button;
  Button previous_item_button, next_item_button;
  Button details_button;
  Button close_button;

  FlarmTrafficControl view;

  Windows(TrafficWidget &widget, ContainerWindow &parent, const PixelRect &r,
          const ButtonLook &button_look, const FlarmTrafficLook &flarm_look)
    :zoom_in_button(MakeSymbolButton(parent, button_look, _T("+"), r,
                                     [&widget](){ widget.ZoomIn(); })),
     zoom_out_button(MakeSymbolButton(parent, button_look,
                                    _T("-"), r,
                                      [&widget](){ widget.ZoomOut(); })),
     previous_item_button(MakeSymbolButton(parent, button_look,
                                           _T("<"), r,
                                           [&widget](){ widget.PreviousTarget(); })),
     next_item_button(MakeSymbolButton(parent, button_look,
                                       _T(">"), r,
                                       [&widget](){ widget.NextTarget(); })),
     details_button(parent, button_look,
                    _("Details"), r, WindowStyle(),
                    [&widget](){ widget.OpenDetails(); }),
     close_button(parent, button_look,
                  _("Close"), r, WindowStyle(),
                  [](){ PageActions::Restore(); }),
     view(flarm_look)
  {
    view.Create(parent, r);
    UpdateLayout(r);
  }

  void UpdateLayout(const PixelRect &rc) noexcept;
};

void
TrafficWidget::Windows::UpdateLayout(const PixelRect &rc) noexcept
{
  view.Move(rc);

  const unsigned margin = Layout::Scale(1);
  const unsigned button_height = Layout::GetMinimumControlHeight();
  const unsigned button_width = std::max(unsigned(rc.right / 6),
                                         button_height);

  const int x1 = rc.right / 2;
  const int x0 = x1 - button_width;
  const int x2 = x1 + button_width;

  const int y0 = margin;
  const int y1 = y0 + button_height;
  const int y3 = rc.bottom - margin;
  const int y2 = y3 - button_height;

  PixelRect button_rc;

  button_rc.left = x0;
  button_rc.top = y0;
  button_rc.right = x1 - margin;
  button_rc.bottom = y1;
  zoom_in_button.Move(button_rc);

  button_rc.left = x1;
  button_rc.right = x2 - margin;
  zoom_out_button.Move(button_rc);

  button_rc.left = x0;
  button_rc.top = y2;
  button_rc.right = x1 - margin;
  button_rc.bottom = y3;
  previous_item_button.Move(button_rc);

  button_rc.left = x1;
  button_rc.right = x2 - margin;
  next_item_button.Move(button_rc);

  button_rc.left = margin;
  button_rc.top = button_height * 3 / 2;
  button_rc.right = button_rc.left + Layout::Scale(50);
  button_rc.bottom = button_rc.top + button_height;
  details_button.Move(button_rc);

  button_rc.right = rc.right - margin;
  button_rc.left = button_rc.right - Layout::Scale(50);
  close_button.Move(button_rc);
}

TrafficWidget::TrafficWidget() noexcept = default;
TrafficWidget::~TrafficWidget() noexcept = default;

void
TrafficWidget::OpenDetails() noexcept
{
  windows->view.OpenDetails();
}

void
TrafficWidget::ZoomIn() noexcept
{
  windows->view.ZoomIn();
  UpdateButtons();
}

void
TrafficWidget::ZoomOut() noexcept
{
  windows->view.ZoomOut();
  UpdateButtons();
}

void
TrafficWidget::PreviousTarget() noexcept
{
  windows->view.PrevTarget();
}

void
TrafficWidget::NextTarget() noexcept
{
  windows->view.NextTarget();
}

void
FlarmTrafficControl::SwitchData()
{
  if (side_display_type == FlarmTrafficWindow::SideInfoType::VARIO)
    side_display_type = FlarmTrafficWindow::SideInfoType::RELATIVE_ALTITUDE;
  else
    side_display_type = FlarmTrafficWindow::SideInfoType::VARIO;

  Profile::SetEnum(ProfileKeys::FlarmSideData, side_display_type);
}

void
TrafficWidget::SwitchData() noexcept
{
  windows->view.SwitchData();
}

bool
TrafficWidget::GetAutoZoom() const noexcept
{
  return windows->view.GetAutoZoom();
}

void
TrafficWidget::SetAutoZoom(bool value) noexcept
{
  windows->view.SetAutoZoom(value);
}

void
TrafficWidget::ToggleAutoZoom() noexcept
{
  windows->view.ToggleAutoZoom();
}

bool
TrafficWidget::GetNorthUp() const noexcept
{
  return windows->view.GetNorthUp();
}

void
TrafficWidget::SetNorthUp(bool value) noexcept
{
  windows->view.SetAutoZoom(value);
}

void
TrafficWidget::ToggleNorthUp() noexcept
{
  windows->view.ToggleNorthUp();
}

void
TrafficWidget::Update() noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (CommonInterface::GetUISettings().traffic.auto_close_dialog &&
      basic.flarm.traffic.IsEmpty() &&
      /* auto-close only really closes the FLARM radar if the
         "restored" page has no FLARM radar */
      PageActions::GetConfiguredLayout().main != PageLayout::Main::FLARM_RADAR) {
    /* this must be deferred, because this method is called from
       within the BlackboardListener, and we must not unregister the
       listener in this context */
    PageActions::DeferredRestore();
    return;
  }

  windows->view.Update(basic.track,
               basic.flarm.traffic,
               CommonInterface::GetComputerSettings().team_code);

  windows->view.UpdateTaskDirection(calculated.task_stats.task_valid &&
                            calculated.task_stats.current_leg.solution_remaining.IsOk(),
                            calculated.task_stats.
                            current_leg.solution_remaining.cruise_track_bearing);

  UpdateButtons();
}

bool
FlarmTrafficControl::OnMouseMove(PixelPoint p,
                                 [[maybe_unused]] unsigned keys) noexcept
{
  if (dragging)
    gestures.Update(p);

  return true;
}

bool
FlarmTrafficControl::OnMouseDown(PixelPoint p) noexcept
{
  if (!dragging) {
    dragging = true;
    SetCapture();
    gestures.Start(p, Layout::Scale(20));
  }

  return true;
}

bool
FlarmTrafficControl::OnMouseUp(PixelPoint p) noexcept
{
  if (dragging) {
    StopDragging();

    const TCHAR *gesture = gestures.Finish();
    if (gesture && OnMouseGesture(gesture))
      return true;
  }

  if (!WarningMode())
    SelectNearTarget(p, Layout::Scale(15));

  return true;
}

bool
FlarmTrafficControl::OnMouseDouble([[maybe_unused]] PixelPoint p) noexcept
{
  StopDragging();
  InputEvents::ShowMenu();
  return true;
}

bool
FlarmTrafficControl::OnMouseGesture(const TCHAR* gesture)
{
  if (StringIsEqual(gesture, _T("U"))) {
    ZoomIn();
    return true;
  }
  if (StringIsEqual(gesture, _T("D"))) {
    ZoomOut();
    return true;
  }
  if (StringIsEqual(gesture, _T("UD"))) {
    SetAutoZoom(true);
    return true;
  }
  if (StringIsEqual(gesture, _T("DR"))) {
    OpenDetails();
    return true;
  }
  if (StringIsEqual(gesture, _T("RL"))) {
    SwitchData();
    return true;
  }

  return InputEvents::processGesture(gesture);
}

void
FlarmTrafficControl::OnCancelMode() noexcept
{
  FlarmTrafficWindow::OnCancelMode();
  StopDragging();
}

bool
FlarmTrafficControl::OnKeyDown(unsigned key_code) noexcept
{
  switch (key_code) {
  case KEY_UP:
    if (!HasPointer())
      break;

    ZoomIn();
    return true;

  case KEY_DOWN:
    if (!HasPointer())
      break;

    ZoomOut();
    return true;
  }

  return FlarmTrafficWindow::OnKeyDown(key_code) ||
    InputEvents::processKey(key_code);
}

void
TrafficWidget::UpdateLayout() noexcept
{
  windows->UpdateLayout(GetContainer().GetClientRect());
}

void
TrafficWidget::UpdateButtons() noexcept
{
  const bool unlocked = !windows->view.WarningMode();
  const TrafficList &traffic = CommonInterface::Basic().flarm.traffic;
  const bool not_empty = !traffic.IsEmpty();
  const bool two_or_more = traffic.GetActiveTrafficCount() >= 2;

  windows->zoom_in_button.SetEnabled(unlocked && windows->view.CanZoomIn());
  windows->zoom_out_button.SetEnabled(unlocked && windows->view.CanZoomOut());
  windows->previous_item_button.SetEnabled(unlocked && two_or_more);
  windows->next_item_button.SetEnabled(unlocked && two_or_more);
  windows->details_button.SetEnabled(unlocked && not_empty);
}

void
TrafficWidget::Prepare(ContainerWindow &parent, const PixelRect &_rc) noexcept
{
  ContainerWidget::Prepare(parent, _rc);

  const Look &look = UIGlobals::GetLook();

  windows = std::make_unique<Windows>(*this, GetContainer(),
                                      GetContainer().GetClientRect(),
                                      look.dialog.button, look.flarm_dialog);
  UpdateLayout();
}

void
TrafficWidget::Show(const PixelRect &rc) noexcept
{
  // Update Radar and Selection for the first time
  Update();

  ContainerWidget::Show(rc);
  UpdateLayout();

  /* show the "Close" button only if this is a "special" page */
  windows->close_button.SetVisible(CommonInterface::GetUIState().pages.special_page.IsDefined());

  CommonInterface::GetLiveBlackboard().AddListener(*this);
}

void
TrafficWidget::Hide() noexcept
{
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
  ContainerWidget::Hide();
}

void
TrafficWidget::Move(const PixelRect &rc) noexcept
{
  ContainerWidget::Move(rc);

  UpdateLayout();
}


bool
TrafficWidget::SetFocus() noexcept
{
  windows->view.SetFocus();
  return true;
}

void
TrafficWidget::OnGPSUpdate([[maybe_unused]] const MoreData &basic)
{
  Update();
}
