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

#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "PopupMessage.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "UIActions.hpp"
#include "PageActions.hpp"
#include "Input/InputEvents.hpp"
#include "Menu/ButtonLabel.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "Audio/Sound.hpp"
#include "Components.hpp"
#include "ProcessTimer.hpp"
#include "LogFile.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Gauge/GaugeThermalAssistant.hpp"
#include "Gauge/GlueGaugeVario.hpp"
#include "Form/Form.hpp"
#include "Widget/Widget.hpp"
#include "UtilsSystem.hpp"
#include "Look/GlobalFonts.hpp"
#include "Look/DefaultFonts.hpp"
#include "Look/Look.hpp"
#include "ProgressGlue.hpp"
#include "UIState.hpp"
#include "DrawThread.hpp"
#include "UIReceiveBlackboard.hpp"
#include "UISettings.hpp"
#include "Interface.hpp"

#ifdef ANDROID
#include "Dialogs/Message.hpp"
#endif

#if !defined(WIN32) && !defined(ANDROID)
#include <unistd.h>
#endif

#if !defined(WIN32) && !defined(ANDROID)
#include <unistd.h> /* for execl() */
#endif

static constexpr unsigned separator_height = 2;

#ifdef HAVE_SHOW_MENU_BUTTON
gcc_pure
static PixelRect
GetShowMenuButtonRect(const PixelRect rc)
{
  const unsigned padding = Layout::GetTextPadding();
  const unsigned size = Layout::GetMaximumControlHeight();
  const int right = rc.right - padding;
  const int left = right - size;
  const int top = rc.top + padding;
  const int bottom = top + size;

  return PixelRect(left, top, right, bottom);
}
#endif

gcc_pure
static PixelRect
GetBottomWidgetRect(const PixelRect &rc, const Widget *bottom_widget)
{
  if (bottom_widget == nullptr) {
    /* no bottom widget: return empty rectangle, map uses the whole
       main area */
    PixelRect result = rc;
    result.top = result.bottom;
    return result;
  }

  const unsigned requested_height = bottom_widget->GetMinimumSize().cy;
  unsigned height;
  if (requested_height > 0) {
    const unsigned max_height = rc.GetHeight() / 2;
    height = std::min(max_height, requested_height);
  } else {
    const unsigned recommended_height = rc.GetHeight() / 3;
    height = recommended_height;
  }

  PixelRect result = rc;
  result.top = result.bottom - height;
  return result;
}

gcc_pure
static PixelRect
GetMapRectAbove(const PixelRect &rc, const PixelRect &bottom_rect)
{
  PixelRect result = rc;
  result.bottom = bottom_rect.top;
  if (bottom_rect.top < bottom_rect.bottom)
    result.bottom -= separator_height;
  return result;
}

MainWindow::MainWindow()
  :look(nullptr),
#ifdef HAVE_SHOW_MENU_BUTTON
   show_menu_button(nullptr),
#endif
   map(nullptr), bottom_widget(nullptr), widget(nullptr), vario(*this),
   traffic_gauge(*this),
   suppress_traffic_gauge(false), force_traffic_gauge(false),
   thermal_assistant(*this),
   dragging(false),
   popup(nullptr),
   timer(*this),
   FullScreen(false),
#ifndef ENABLE_OPENGL
   draw_suspended(false),
#endif
   restore_page_pending(false)
{
}

/**
 * Destructor of the MainWindow-Class
 * @return
 */
MainWindow::~MainWindow()
{
  Destroy();
}

void
MainWindow::Create(PixelSize size, TopWindowStyle style)
{
  SingleWindow::Create(title, size, style);
}

gcc_noreturn
static void
FatalError(const TCHAR *msg)
{
#if defined(HAVE_POSIX) && defined(NDEBUG)
  /* make sure this gets written to stderr in any case; LogFormat()
     will write to stderr only in debug builds */
  fprintf(stderr, "%s\n", msg);
#endif

  /* log the error */
  LogFormat(_T("%s"), msg);

  /* now try to get a GUI error message out to the user */
#ifdef WIN32
  MessageBox(nullptr, msg, _T("XCSoar"), MB_ICONEXCLAMATION|MB_OK);
#elif !defined(ANDROID) && !defined(KOBO)
  execl("/usr/bin/xmessage", "xmessage", msg, nullptr);
  execl("/usr/X11/bin/xmessage", "xmessage", msg, nullptr);
#endif
  exit(EXIT_FAILURE);
}

gcc_noreturn
static void
NoFontsAvailable()
{
  FatalError(_T("Font initialisation failed"));
}

void
MainWindow::Initialise()
{
  Layout::Initialize(GetSize(),
                     CommonInterface::GetUISettings().GetPercentScale(),
                     CommonInterface::GetUISettings().custom_dpi);

  LogFormat("Initialise fonts");
  if (!Fonts::Initialize()) {
    Destroy();
    NoFontsAvailable();
  }

  if (look == nullptr)
    look = new Look();

  look->Initialise(Fonts::map);
}

void
MainWindow::InitialiseConfigured()
{
  const UISettings &ui_settings = CommonInterface::GetUISettings();

  if (ui_settings.scale != 100)
    /* call Initialise() again to reload fonts with the new scale */
    Initialise();

  PixelRect rc = GetClientRect();

  const InfoBoxLayout::Layout ib_layout =
    InfoBoxLayout::Calculate(rc, ui_settings.info_boxes.geometry);

  assert(look != nullptr);
  look->InitialiseConfigured(CommonInterface::GetUISettings(),
                             Fonts::map, Fonts::map_bold,
                             ib_layout.control_size.cx);

  InfoBoxManager::Create(*this, ib_layout, look->info_box);
  map_rect = ib_layout.remaining;

  ButtonLabel::CreateButtonLabels(*this, look->dialog.button);

  ReinitialiseLayout_vario(ib_layout);

  ReinitialiseLayoutTA(rc, ib_layout);

  WindowStyle hidden_border;
  hidden_border.Hide();
  hidden_border.Border();

  ReinitialiseLayout_flarm(rc, ib_layout);

#ifdef HAVE_SHOW_MENU_BUTTON
  const UISettings &settings = CommonInterface::GetUISettings();
  if (settings.show_menu_button){
    show_menu_button = new ShowMenuButton();
    show_menu_button->Create(*this, GetShowMenuButtonRect(map_rect));
  }
#endif

  map = new GlueMapWindow(*look);
  map->SetComputerSettings(CommonInterface::GetComputerSettings());
  map->SetMapSettings(CommonInterface::GetMapSettings());
  map->SetUIState(CommonInterface::GetUIState());
  map->Create(*this, map_rect);

  popup = new PopupMessage(*this, look->dialog, ui_settings);
  popup->Create(rc);
}

void
MainWindow::Deinitialise()
{
  InfoBoxManager::Destroy();
  ButtonLabel::Destroy();

  delete popup;
  popup = nullptr;

  // During destruction of GlueMapWindow WM_SETFOCUS gets called for
  // MainWindow which tries to set the focus to GlueMapWindow. Prevent
  // this issue by setting map to nullptr before calling delete.
  GlueMapWindow *temp_map = map;
  map = nullptr;
  delete temp_map;

#ifdef HAVE_SHOW_MENU_BUTTON
  delete show_menu_button;
  show_menu_button = nullptr;
#endif

  vario.Clear();
  traffic_gauge.Clear();
  thermal_assistant.Clear();

  delete look;
  look = nullptr;
}

void
MainWindow::ReinitialiseLayout_vario(const InfoBoxLayout::Layout &layout)
{
  if (!layout.HasVario()) {
    vario.Clear();
    return;
  }

  if (!vario.IsDefined())
    vario.Set(new GlueGaugeVario(CommonInterface::GetLiveBlackboard(),
                                 look->vario));

  vario.Move(layout.vario);
  vario.Show();

  // XXX vario->BringToTop();
}

void
MainWindow::ReinitialiseLayoutTA(PixelRect rc,
                                 const InfoBoxLayout::Layout &layout)
{
  unsigned sz = std::min(layout.control_size.cy,
                         layout.control_size.cx) * 2;
  rc.right = rc.left + sz;
  rc.top = rc.bottom - sz;
  thermal_assistant.Move(rc);
}

void
MainWindow::ReinitialiseLayout()
{
  if (map == nullptr)
    /* without the MapWindow, it is safe to assume that the MainWindow
       is just being initialized, and the InfoBoxes aren't initialized
       yet either, so there is nothing to do here */
    return;

  const PixelRect rc = GetClientRect();

#ifndef ENABLE_OPENGL
  if (draw_thread == nullptr)
    /* no layout changes during startup */
    return;
#endif

  InfoBoxManager::Destroy();

  const UISettings &ui_settings = CommonInterface::GetUISettings();

  const InfoBoxLayout::Layout ib_layout =
    InfoBoxLayout::Calculate(rc, ui_settings.info_boxes.geometry);

  look->ReinitialiseLayout(ib_layout.control_size.cx);

  InfoBoxManager::Create(*this, ib_layout, look->info_box);
  InfoBoxManager::ProcessTimer();
  map_rect = ib_layout.remaining;

  popup->UpdateLayout(rc);

  ReinitialiseLayout_vario(ib_layout);

  ReinitialiseLayout_flarm(rc, ib_layout);

  ReinitialiseLayoutTA(rc, ib_layout);

  if (map != nullptr) {
    if (FullScreen)
      InfoBoxManager::Hide();
    else
      InfoBoxManager::Show();

    const PixelRect main_rect = GetMainRect();
    const PixelRect bottom_rect = GetBottomWidgetRect(main_rect,
                                                      bottom_widget);

    if (HaveBottomWidget())
      bottom_widget->Move(bottom_rect);

    map->Move(GetMapRectAbove(main_rect, bottom_rect));
    map->FullRedraw();
  }

  if (widget != nullptr)
    widget->Move(GetMainRect(rc));

#ifdef HAVE_SHOW_MENU_BUTTON
  if (show_menu_button != nullptr)
    show_menu_button->Move(GetShowMenuButtonRect(GetMainRect()));
#endif

  if (map != nullptr)
    map->BringToBottom();
}

void 
MainWindow::ReinitialiseLayout_flarm(PixelRect rc, const InfoBoxLayout::Layout ib_layout)
{
  TrafficSettings::GaugeLocation val =
    CommonInterface::GetUISettings().traffic.gauge_location;

  // Automatic mode - follow info boxes
  if (val == TrafficSettings::GaugeLocation::Auto) {
    switch (InfoBoxManager::layout.geometry) {
    case InfoBoxSettings::Geometry::TOP_LEFT_8:
    case InfoBoxSettings::Geometry::TOP_LEFT_12:
      if (InfoBoxManager::layout.landscape)
        val = TrafficSettings::GaugeLocation::BottomLeft;
      else
        val = TrafficSettings::GaugeLocation::TopRight;
      break;

    default:
      val = TrafficSettings::GaugeLocation::BottomRight;    // Assume bottom right unles...
      break;
    }
  }

  switch (val) {
  case TrafficSettings::GaugeLocation::TopLeft:
    rc.right = rc.left + ib_layout.control_size.cx * 2;
    ++rc.left;
    rc.bottom = rc.top + ib_layout.control_size.cy * 2;
    ++rc.top;
    break;

  case TrafficSettings::GaugeLocation::TopRight:
    rc.left = rc.right - ib_layout.control_size.cx * 2 + 1;
    rc.bottom = rc.top + ib_layout.control_size.cy * 2;
    ++rc.top;
    break;

  case TrafficSettings::GaugeLocation::BottomLeft:
    rc.right = rc.left + ib_layout.control_size.cx * 2;
    ++rc.left;
    rc.top = rc.bottom - ib_layout.control_size.cy * 2 + 1;
    break;

  case TrafficSettings::GaugeLocation::CentreTop:
    rc.left = (rc.left + rc.right) / 2 - ib_layout.control_size.cx;
    rc.right = rc.left + ib_layout.control_size.cx * 2 - 1;
    rc.bottom = rc.top + ib_layout.control_size.cy * 2;
    ++rc.top;
    break;

  case TrafficSettings::GaugeLocation::CentreBottom:
    rc.left = (rc.left + rc.right) / 2 - ib_layout.control_size.cx;
    rc.right = rc.left + ib_layout.control_size.cx * 2 - 1;
    rc.top = rc.bottom - ib_layout.control_size.cy * 2 + 1;
    break;

  default:    // aka flBottomRight
    rc.left = rc.right - ib_layout.control_size.cx * 2 + 1;
    rc.top = rc.bottom - ib_layout.control_size.cy * 2 + 1;
    break;
  }

  traffic_gauge.Move(rc);
}

void
MainWindow::Destroy()
{
  Deinitialise();

  TopWindow::Destroy();
}

void
MainWindow::FinishStartup()
{
  timer.Schedule(500); // 2 times per second

  ResumeThreads();
}

void
MainWindow::BeginShutdown()
{
  timer.Cancel();

  KillBottomWidget();
}

void
MainWindow::SuspendThreads()
{
  if (map != nullptr)
    map->SuspendThreads();
}

void
MainWindow::ResumeThreads()
{
  if (map != nullptr)
    map->ResumeThreads();
}

void
MainWindow::SetDefaultFocus()
{
  if (map != nullptr && widget == nullptr)
    map->SetFocus();
  else if (widget == nullptr || !widget->SetFocus())
    SetFocus();
}

void
MainWindow::FlushRendererCaches()
{
  if (map != nullptr)
    map->FlushCaches();
}

void
MainWindow::FullRedraw()
{
  if (map != nullptr)
    map->FullRedraw();
}

// Windows event handlers

void
MainWindow::OnResize(PixelSize new_size)
{
  Layout::Initialize(new_size,
                     CommonInterface::GetUISettings().GetPercentScale(),
                     CommonInterface::GetUISettings().custom_dpi);

  SingleWindow::OnResize(new_size);

  ReinitialiseLayout();

  const PixelRect rc = GetClientRect();
  ButtonLabel::OnResize(rc);
  ProgressGlue::Move(rc);
}

void
MainWindow::OnSetFocus()
{
  SingleWindow::OnSetFocus();

  if (!HasDialog()) {
    /* the main window should never have the keyboard focus; if we
       happen to get the focus despite of that, forward it to the map
       window to make keyboard shortcuts work */
    if (map != nullptr && widget == nullptr)
      map->SetFocus();
    else if (widget != nullptr)
      widget->SetFocus();
  } else
    /* recover the dialog focus if it got lost */
    GetTopDialog().FocusFirstControl();
}

void
MainWindow::StopDragging()
{
  if (!dragging)
    return;

  dragging = false;
  ReleaseCapture();
}

void
MainWindow::OnCancelMode()
{
  SingleWindow::OnCancelMode();
  StopDragging();
}

bool
MainWindow::OnMouseDown(PixelPoint p)
{
  if (SingleWindow::OnMouseDown(p))
    return true;

  if (!dragging && !HasDialog()) {
    dragging = true;
    SetCapture();
    gestures.Start(p, Layout::Scale(20));
  }

  return true;
}

bool
MainWindow::OnMouseUp(PixelPoint p)
{
  if (SingleWindow::OnMouseUp(p))
    return true;

  if (dragging) {
    StopDragging();

    const TCHAR *gesture = gestures.Finish();
    if (gesture && InputEvents::processGesture(gesture))
      return true;
  }

  return false;
}

bool
MainWindow::OnMouseDouble(PixelPoint p)
{
  if (SingleWindow::OnMouseDouble(p))
    return true;

  StopDragging();

  if (!HasDialog())
    InputEvents::ShowMenu();
  return false;
}

bool
MainWindow::OnMouseMove(PixelPoint p, unsigned keys)
{
  if (SingleWindow::OnMouseMove(p, keys))
    return true;

  if (dragging)
    gestures.Update(p);

  return true;
}

bool
MainWindow::OnKeyDown(unsigned key_code)
{
  return (widget != nullptr && widget->KeyPress(key_code)) ||
    (HaveBottomWidget() && bottom_widget->KeyPress(key_code)) ||
    InputEvents::processKey(key_code) ||
    SingleWindow::OnKeyDown(key_code);
}

bool
MainWindow::OnTimer(WindowTimer &_timer)
{
  if (_timer != timer)
    return SingleWindow::OnTimer(_timer);

  ProcessTimer();

  UpdateGaugeVisibility();

  if (!CommonInterface::GetUISettings().enable_thermal_assistant_gauge) {
    thermal_assistant.Clear();
  } else if (!CommonInterface::Calculated().circling ||
             InputEvents::IsFlavour(_T("TA"))) {
    thermal_assistant.Hide();
  } else if (!HasDialog()) {
    if (!thermal_assistant.IsDefined())
      thermal_assistant.Set(new GaugeThermalAssistant(CommonInterface::GetLiveBlackboard(),
                                                      look->thermal_assistant_gauge));

    if (!thermal_assistant.IsVisible()) {
      thermal_assistant.Show();

      GaugeThermalAssistant *widget =
        (GaugeThermalAssistant *)thermal_assistant.Get();
      widget->Raise();
    }
  }

  battery_timer.Process();

  return true;
}

bool
MainWindow::OnUser(unsigned id)
{
  switch ((Command)id) {
  case Command::GPS_UPDATE:
    UIReceiveSensorData();
    return true;

  case Command::CALCULATED_UPDATE:
    UIReceiveCalculatedData();
    return true;

  case Command::RESTORE_PAGE:
    if (restore_page_pending)
      PageActions::Restore();
    return true;
  }

  return false;
}

void
MainWindow::OnDestroy()
{
  timer.Cancel();

  KillWidget();
  KillBottomWidget();

  SingleWindow::OnDestroy();
}

bool MainWindow::OnClose() {
  if (HasDialog() || !IsRunning())
    /* no shutdown dialog if XCSoar hasn't completed initialization
       yet (e.g. if we are in the simulator prompt) */
    return SingleWindow::OnClose();

  if (UIActions::CheckShutdown()) {
    PostQuit();
  }
  return true;
}

void
MainWindow::OnPaint(Canvas &canvas)
{
  if (HaveBottomWidget() && map != nullptr) {
    /* draw a separator between main area and bottom area */
    PixelRect rc = map->GetPosition();
    rc.top = rc.bottom;
    rc.bottom += separator_height;
    canvas.DrawFilledRectangle(rc, COLOR_BLACK);
  }

  SingleWindow::OnPaint(canvas);
}

void
MainWindow::SetFullScreen(bool _full_screen)
{
  if (_full_screen == FullScreen)
    return;

  FullScreen = _full_screen;

  if (FullScreen)
    InfoBoxManager::Hide();
  else
    InfoBoxManager::Show();

  if (widget != nullptr)
    widget->Move(GetMainRect());

  if (map != nullptr)
    map->FastMove(GetMainRect());

  // the repaint will be triggered by the DrawThread

  UpdateVarioGaugeVisibility();
}

void
MainWindow::SetTerrain(RasterTerrain *terrain)
{
  if (map != nullptr)
    map->SetTerrain(terrain);
}

void
MainWindow::SetTopography(TopographyStore *topography)
{
  if (map != nullptr)
    map->SetTopography(topography);
}

void
MainWindow::SetComputerSettings(const ComputerSettings &settings_computer)
{
  if (map != nullptr)
    map->SetComputerSettings(settings_computer);
}

void
MainWindow::SetMapSettings(const MapSettings &settings_map)
{
  if (map != nullptr)
    map->SetMapSettings(settings_map);
}

void
MainWindow::SetUIState(const UIState &ui_state)
{
  if (map != nullptr) {
    map->SetUIState(ui_state);
    map->FullRedraw();
  }
}

GlueMapWindow *
MainWindow::GetMapIfActive()
{
  return IsMapActive() ? map : nullptr;
}

GlueMapWindow *
MainWindow::ActivateMap()
{
  restore_page_pending = false;

  if (map == nullptr)
    return nullptr;

  if (widget != nullptr) {
    KillWidget();
    map->Show();
    map->SetFocus();

    if (bottom_widget != nullptr) {
      assert(HaveBottomWidget());
      bottom_widget->Show(GetBottomWidgetRect(GetMainRect(),
                                              bottom_widget));
    }

#ifndef ENABLE_OPENGL
    if (draw_suspended) {
      draw_suspended = false;
      draw_thread->Resume();
    }
#endif
  }

  return map;
}

void
MainWindow::DeferredRestorePage()
{
  if (restore_page_pending)
    return;

  restore_page_pending = true;
  SendUser((unsigned)Command::RESTORE_PAGE);
}

void
MainWindow::KillWidget()
{
  if (widget == nullptr)
    return;

  widget->Leave();
  widget->Hide();
  widget->Unprepare();
  delete widget;
  widget = nullptr;

  InputEvents::SetFlavour(nullptr);
}

void
MainWindow::KillBottomWidget()
{
  if (bottom_widget == nullptr)
    return;

  if (widget == nullptr)
    /* the bottom widget is only visible below the map, but not below
       a custom main widget; see HaveBottomWidget() */
    bottom_widget->Hide();

  bottom_widget->Unprepare();
  delete bottom_widget;
  bottom_widget = nullptr;
}

void
MainWindow::SetBottomWidget(Widget *_widget)
{
  if (bottom_widget == nullptr && _widget == nullptr)
    return;

  if (map == nullptr) {
    /* this doesn't work without a map */
    delete _widget;
    return;
  }

  KillBottomWidget();

  bottom_widget = _widget;

  const PixelRect main_rect = GetMainRect();
  const PixelRect bottom_rect = GetBottomWidgetRect(main_rect,
                                                    bottom_widget);

  if (bottom_widget != nullptr) {
    bottom_widget->Initialise(*this, bottom_rect);
    bottom_widget->Prepare(*this, bottom_rect);

    if (widget == nullptr)
      /* the bottom widget is only visible below the map, but not
         below a custom main widget; see HaveBottomWidget() */
      bottom_widget->Show(bottom_rect);
  }

  map->Move(GetMapRectAbove(main_rect, bottom_rect));
  map->FullRedraw();
}

void
MainWindow::SetWidget(Widget *_widget)
{
  assert(_widget != nullptr);

  restore_page_pending = false;

  const bool have_bottom_widget = HaveBottomWidget();

  /* delete the old widget */
  KillWidget();

  /* hide the map (might be hidden already) */
  if (map != nullptr) {
    map->FastHide();

#ifndef ENABLE_OPENGL
    if (!draw_suspended) {
      draw_suspended = true;
      draw_thread->BeginSuspend();
    }
#endif
  }

  if (have_bottom_widget)
    bottom_widget->Hide();

  widget = _widget;

  const PixelRect rc = GetMainRect();
  widget->Initialise(*this, rc);
  widget->Prepare(*this, rc);
  widget->Show(rc);

  if (!widget->SetFocus())
    SetFocus();
}

Widget *
MainWindow::GetFlavourWidget(const TCHAR *flavour)
{
  return InputEvents::IsFlavour(flavour)
    ? widget
    : nullptr;
}

void
MainWindow::UpdateVarioGaugeVisibility()
{
  bool full_screen = GetFullScreen();

  vario.SetVisible(!full_screen &&
                   !CommonInterface::GetUIState().screen_blanked);
}

void
MainWindow::UpdateGaugeVisibility()
{
  UpdateVarioGaugeVisibility();
  UpdateTrafficGaugeVisibility();
}

void
MainWindow::UpdateTrafficGaugeVisibility()
{
  const FlarmData &flarm = CommonInterface::Basic().flarm;

  bool traffic_visible =
    (force_traffic_gauge ||
     (CommonInterface::GetUISettings().traffic.enable_gauge &&
      !flarm.traffic.IsEmpty())) &&
    !CommonInterface::GetUIState().screen_blanked &&
    /* hide the traffic gauge while the traffic widget is visible, to
       avoid showing the same information twice */
    !InputEvents::IsFlavour(_T("Traffic"));

  if (traffic_visible && suppress_traffic_gauge) {
    if (flarm.status.available &&
        flarm.status.alarm_level != FlarmTraffic::AlarmType::NONE)
      suppress_traffic_gauge = false;
    else
      traffic_visible = false;
  }

  if (traffic_visible) {
    if (HasDialog())
      return;

    if (!traffic_gauge.IsDefined())
      traffic_gauge.Set(new GaugeFLARM(CommonInterface::GetLiveBlackboard(),
                                       GetLook().flarm_gauge));

    if (!traffic_gauge.IsVisible()) {
      traffic_gauge.Show();

      GaugeFLARM *widget = (GaugeFLARM *)traffic_gauge.Get();
      widget->Raise();
    }
  } else
    traffic_gauge.Hide();
}

const MapWindowProjection &
MainWindow::GetProjection() const
{
  AssertThread();
  assert(map != nullptr);

  return map->VisibleProjection();
}

void
MainWindow::ToggleSuppressFLARMRadar()
{
  suppress_traffic_gauge = !suppress_traffic_gauge;
}

void
MainWindow::ToggleForceFLARMRadar()
{
  force_traffic_gauge = !force_traffic_gauge;
  CommonInterface::SetUISettings().traffic.enable_gauge = force_traffic_gauge;
}

#ifdef ANDROID

void
MainWindow::OnPause()
{
  if (!IsRunning() && HasDialog())
    /* suspending before initialization has finished doesn't leave
       anything worth resuming, so let's just quit now */
    CancelDialog();

  SingleWindow::OnPause();
}

#endif /* ANDROID */
