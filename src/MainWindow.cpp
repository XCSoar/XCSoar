// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "PopupMessage.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "UIActions.hpp"
#include "PageActions.hpp"
#include "Input/InputEvents.hpp"
#include "Menu/MenuBar.hpp"
#include "Menu/Glue.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "Screen/Layout.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "Audio/Sound.hpp"
#include "ProcessTimer.hpp"
#include "LogFile.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Gauge/GaugeThermalAssistant.hpp"
#include "Gauge/GlueGaugeVario.hpp"
#include "Form/Form.hpp"
#include "Widget/Widget.hpp"
#include "Look/GlobalFonts.hpp"
#include "Look/DefaultFonts.hpp"
#include "Look/Look.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "Device/MultipleDevices.hpp"
#include "ProgressGlue.hpp"
#include "UIState.hpp"
#include "DrawThread.hpp"
#include "UIReceiveBlackboard.hpp"
#include "UISettings.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

#ifdef ANDROID
#include "Android/ReceiveTask.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

static constexpr unsigned separator_height = 2;

#ifdef HAVE_SHOW_MENU_BUTTON
[[gnu::pure]]
static PixelRect
GetShowMenuButtonRect(const PixelRect rc) noexcept
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

[[gnu::pure]]
static PixelRect
GetTopWidgetRect(const PixelRect &rc, const Widget *top_widget) noexcept
{
  if (top_widget == nullptr) {
    /* no top widget: return empty rectangle, map uses the whole main
       area */
    PixelRect result = rc;
    result.bottom = result.top;
    return result;
  }

  const unsigned requested_height = top_widget->GetMinimumSize().height;
  unsigned height;
  if (requested_height > 0) {
    const unsigned max_height = rc.GetHeight() / 2;
    height = std::min(max_height, requested_height);
  } else {
    const unsigned recommended_height = rc.GetHeight() / 4;
    height = recommended_height;
  }

  PixelRect result = rc;
  result.bottom = result.top + height;
  return result;
}

[[gnu::pure]]
static PixelRect
GetBottomWidgetRect(const PixelRect &rc, const Widget *bottom_widget) noexcept
{
  if (bottom_widget == nullptr) {
    /* no bottom widget: return empty rectangle, map uses the whole
       main area */
    PixelRect result = rc;
    result.top = result.bottom;
    return result;
  }

  const unsigned requested_height = bottom_widget->GetMinimumSize().height;
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

[[gnu::pure]]
static PixelRect
GetMapRectAbove(const PixelRect &rc, const PixelRect &bottom_rect) noexcept
{
  PixelRect result = rc;
  result.bottom = bottom_rect.top;
  if (bottom_rect.top < bottom_rect.bottom)
    result.bottom -= separator_height;
  return result;
}

[[gnu::pure]]
static PixelRect
GetMapRectBelow(const PixelRect &rc, const PixelRect &top_rect) noexcept
{
  PixelRect result = rc;
  result.top = top_rect.bottom;
  if (top_rect.top < top_rect.bottom)
    result.top += separator_height;
  return result;
}

/**
 * Destructor of the MainWindow-Class
 * @return
 */
MainWindow::~MainWindow() noexcept
{
  Destroy();
}

void
MainWindow::Create(PixelSize size, UI::TopWindowStyle style)
{
  SingleWindow::Create(title, size, style);
}

void
MainWindow::Initialise()
{
  Layout::Initialise(GetDisplay(), GetSize(),
                     CommonInterface::GetUISettings().GetPercentScale(),
                     CommonInterface::GetUISettings().custom_dpi);
#ifdef DRAW_MOUSE_CURSOR
  SetCursorSize(CommonInterface::GetDisplaySettings().cursor_size);
  SetCursorColorsInverted(CommonInterface::GetDisplaySettings().invert_cursor_colors);
#endif

  Fonts::Initialize();

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
                             ib_layout.control_size.width);

  InfoBoxManager::Create(*this, ib_layout, look->info_box);
  map_rect = ib_layout.remaining;

  menu_bar = new MenuBar(*this, look->dialog.button);

  ReinitialiseLayout_vario(ib_layout);
  ReinitialiseLayoutTA(rc, ib_layout);
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
MainWindow::Deinitialise() noexcept
{
  InfoBoxManager::Destroy();

  delete menu_bar;
  menu_bar = nullptr;

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
MainWindow::ReinitialiseLayout_vario(const InfoBoxLayout::Layout &layout) noexcept
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
                                 const InfoBoxLayout::Layout &layout) noexcept
{
  unsigned sz = std::min(layout.control_size.height,
                         layout.control_size.width) * 2;

  switch (CommonInterface::GetUISettings().thermal_assistant_position) {
  case (UISettings::ThermalAssistantPosition::BOTTOM_LEFT_AVOID_IB):
    rc.bottom = GetMainRect().bottom;
    rc.left = GetMainRect().left;
    rc.right = rc.left + sz;
    break;
  case (UISettings::ThermalAssistantPosition::BOTTOM_RIGHT_AVOID_IB):
    rc.bottom = GetMainRect().bottom;
    rc.right = GetMainRect().right;
    rc.left = rc.right - sz;
    break;
  case (UISettings::ThermalAssistantPosition::BOTTOM_RIGHT):
    rc.right = GetMainRect().right;
    rc.left = rc.right - sz;
    break;
  default: // BOTTOM_LEFT
    rc.left = GetMainRect().left;
    rc.right = rc.left + sz;
    break;
  }
  rc.top = rc.bottom - sz;
  thermal_assistant.Move(rc);
}

void
MainWindow::ReinitialiseLayout() noexcept
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

  look->ReinitialiseLayout(ib_layout.control_size.width);

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

    PixelRect main_rect = GetMainRect();
    const PixelRect top_rect = GetTopWidgetRect(main_rect,
                                                top_widget);
    main_rect = GetMapRectBelow(main_rect, top_rect);

    if (HaveTopWidget())
      top_widget->Move(top_rect);

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
MainWindow::ReinitialiseLayout_flarm(PixelRect rc,
                                     const InfoBoxLayout::Layout &ib_layout) noexcept
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

  unsigned width = ib_layout.control_size.width * 2;
  unsigned height = ib_layout.control_size.height * 2;

  switch (val) {
  case TrafficSettings::GaugeLocation::TopLeft:
    rc.right = rc.left + width;
    rc.bottom = rc.top + height;
    break;

  case TrafficSettings::GaugeLocation::TopRight:
    rc.left = rc.right - width;
    rc.bottom = rc.top + height;
    break;

  case TrafficSettings::GaugeLocation::BottomLeft:
    rc.right = rc.left + width;
    rc.top = rc.bottom - height;
    break;

  case TrafficSettings::GaugeLocation::CentreTop:
    rc.left = (rc.left + rc.right) / 2 - width - 1;
    rc.right = rc.left + width;
    rc.bottom = rc.top + height;
    break;

  case TrafficSettings::GaugeLocation::CentreBottom:
    rc.left = (rc.left + rc.right) / 2 - width - 1;
    rc.right = rc.left + width;
    rc.top = rc.bottom - height;
    break;

  default:    // aka flBottomRight
    rc.left = rc.right - width;
    rc.top = rc.bottom - height;
    break;
  }

  ++rc.top;
  ++rc.left;
  traffic_gauge.Move(rc);
}

void
MainWindow::ReinitialiseLook() noexcept
{
  const auto &ui_settings = CommonInterface::GetUISettings();

  const InfoBoxLayout::Layout ib_layout =
    InfoBoxLayout::Calculate(GetClientRect(),
                             ui_settings.info_boxes.geometry);

  assert(look != nullptr);
  look->InitialiseConfigured(CommonInterface::GetUISettings(),
                             Fonts::map, Fonts::map_bold,
                             ib_layout.control_size.width);

  InfoBoxManager::ScheduleRedraw();
}

#ifdef ANDROID

void
MainWindow::OnLook() noexcept
{
  ReinitialiseLook();
}

void
MainWindow::OnTaskReceived() noexcept
{
  if (!IsRunning())
    /* postpone until XCSoar is running */
    return;

  if (HasDialog())
    /* don't intercept an existing modal dialog */
    return;

  auto task = GetReceivedTask();
  if (!task)
    return;

  dlgTaskManagerShowModal(std::move(task));
}

#endif // ANDROID

void
MainWindow::Destroy() noexcept
{
  Deinitialise();

  TopWindow::Destroy();
}

void
MainWindow::FinishStartup() noexcept
{
  timer.Schedule(std::chrono::milliseconds(500)); // 2 times per second

  ResumeThreads();
}

void
MainWindow::BeginShutdown() noexcept
{
  timer.Cancel();

  KillTopWidget();
  KillBottomWidget();
}

void
MainWindow::SuspendThreads() noexcept
{
  if (map != nullptr)
    map->SuspendThreads();
}

void
MainWindow::ResumeThreads() noexcept
{
  if (map != nullptr)
    map->ResumeThreads();
}

void
MainWindow::SetDefaultFocus() noexcept
{
  if (map != nullptr && widget == nullptr)
    map->SetFocus();
  else if (widget == nullptr || !widget->SetFocus())
    SetFocus();
}

void
MainWindow::FlushRendererCaches() noexcept
{
  if (map != nullptr)
    map->FlushCaches();
}

void
MainWindow::FullRedraw() noexcept
{
  if (map != nullptr)
    map->FullRedraw();
}

// Windows event handlers

void
MainWindow::OnResize(PixelSize new_size) noexcept
{
  Layout::Initialise(GetDisplay(), new_size,
                     CommonInterface::GetUISettings().GetPercentScale(),
                     CommonInterface::GetUISettings().custom_dpi);

  SingleWindow::OnResize(new_size);

  ReinitialiseLayout();

  const PixelRect rc = GetClientRect();

  if (menu_bar != nullptr)
    menu_bar->OnResize(rc);

  ProgressGlue::Move(rc);
}

void
MainWindow::OnSetFocus() noexcept
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
MainWindow::StopDragging() noexcept
{
  if (!dragging)
    return;

  dragging = false;
  ReleaseCapture();
}

void
MainWindow::OnCancelMode() noexcept
{
  SingleWindow::OnCancelMode();
  StopDragging();
}

bool
MainWindow::OnMouseDown(PixelPoint p) noexcept
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
MainWindow::OnMouseUp(PixelPoint p) noexcept
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
MainWindow::OnMouseDouble(PixelPoint p) noexcept
{
  if (SingleWindow::OnMouseDouble(p))
    return true;

  StopDragging();

  if (!HasDialog())
    InputEvents::ShowMenu();
  return false;
}

bool
MainWindow::OnMouseMove(PixelPoint p, unsigned keys) noexcept
{
  if (SingleWindow::OnMouseMove(p, keys))
    return true;

  if (dragging)
    gestures.Update(p);

  return true;
}

bool
MainWindow::OnKeyDown(unsigned key_code) noexcept
{
  return (widget != nullptr && widget->KeyPress(key_code)) ||
    (HaveTopWidget() && top_widget->KeyPress(key_code)) ||
    (HaveBottomWidget() && bottom_widget->KeyPress(key_code)) ||
    InputEvents::processKey(key_code) ||
    SingleWindow::OnKeyDown(key_code);
}

inline void
MainWindow::LateInitialise() noexcept
{
  if (late_initialised)
    return;

  late_initialised = true;

  if (backend_components->devices != nullptr) {
    /* this OperationEnvironment instance must be persistent, because
       DeviceDescriptor::Open() is asynchronous */
    static PopupOperationEnvironment env;

    /* opening all devices needs to be postponed to here because
       during early initialisation (before the main event loop runs),
       opening some devices may be intercepted by Android which pauses
       XCSoar in order to ask the user for permission; pausing works
       properly only if the main event loop runs */
    backend_components->devices->Open(env);
  }
}

void
MainWindow::RunTimer() noexcept
{
  LateInitialise();

#ifdef ANDROID
  /* if we still havn't processed the task that was received from a QR
     code, re-post the TASK_RECEIVED event to invoke OnTaskReceived()
     again; we must not open the task manager dialog here because it
     would block the timer while the dialog is open */
  if (IsRunning() && !HasDialog() && HasReceivedTask())
    UI::event_queue->Inject(UI::Event::TASK_RECEIVED);
#endif

  ProcessTimer();

  UpdateGaugeVisibility();

  if (CommonInterface::GetUISettings().thermal_assistant_position == UISettings::ThermalAssistantPosition::OFF) {
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
}

void
MainWindow::OnGpsNotify() noexcept
{
  PopupOperationEnvironment env;
  UIReceiveSensorData(env);
}

void
MainWindow::OnCalculatedNotify() noexcept
{
  UIReceiveCalculatedData();
}

void
MainWindow::OnRestorePageNotify() noexcept
{
  if (restore_page_pending)
    PageActions::Restore();
}

void
MainWindow::OnDestroy() noexcept
{
  timer.Cancel();

  KillWidget();
  KillTopWidget();
  KillBottomWidget();

  SingleWindow::OnDestroy();
}

bool
MainWindow::OnClose() noexcept
{
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
MainWindow::OnPaint(Canvas &canvas) noexcept
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
MainWindow::SetFullScreen(bool _full_screen) noexcept
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
MainWindow::SetTerrain(RasterTerrain *terrain) noexcept
{
  if (map != nullptr)
    map->SetTerrain(terrain);
}

void
MainWindow::SetTopography(TopographyStore *topography) noexcept
{
  if (map != nullptr)
    map->SetTopography(topography);
}

void
MainWindow::SetComputerSettings(const ComputerSettings &settings_computer) noexcept
{
  if (map != nullptr)
    map->SetComputerSettings(settings_computer);
}

void
MainWindow::SetMapSettings(const MapSettings &settings_map) noexcept
{
  if (map != nullptr)
    map->SetMapSettings(settings_map);
}

void
MainWindow::SetUIState(const UIState &ui_state) noexcept
{
  if (map != nullptr) {
    map->SetUIState(ui_state);
    map->FullRedraw();
  }
}

GlueMapWindow *
MainWindow::GetMapIfActive() noexcept
{
  return IsMapActive() ? map : nullptr;
}

GlueMapWindow *
MainWindow::ActivateMap() noexcept
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
MainWindow::DeferredRestorePage() noexcept
{
  if (restore_page_pending)
    return;

  restore_page_pending = true;
  restore_page_notify.SendNotification();
}

void
MainWindow::KillWidget() noexcept
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
MainWindow::KillTopWidget() noexcept
{
  if (top_widget == nullptr)
    return;

  top_widget->Hide();
  top_widget->Unprepare();
  delete top_widget;
  top_widget = nullptr;
}

void
MainWindow::SetTopWidget(Widget *_widget) noexcept
{
  if (top_widget == nullptr && _widget == nullptr)
    return;

  KillTopWidget();

  top_widget = _widget;

  PixelRect main_rect = GetMainRect();
  const PixelRect top_rect = GetTopWidgetRect(main_rect,
                                              top_widget);
  if (top_widget != nullptr) {
    top_widget->Initialise(*this, top_rect);
    top_widget->Prepare(*this, top_rect);
    top_widget->Show(top_rect);
  }

  main_rect = GetMapRectBelow(main_rect, top_rect);

  const PixelRect bottom_rect = GetBottomWidgetRect(main_rect,
                                                    bottom_widget);

  if (HaveBottomWidget())
    bottom_widget->Move(bottom_rect);

  map->Move(GetMapRectAbove(main_rect, bottom_rect));
  map->FullRedraw();
}

void
MainWindow::KillBottomWidget() noexcept
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
MainWindow::SetBottomWidget(Widget *_widget) noexcept
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

  PixelRect main_rect = GetMainRect();
  const PixelRect top_rect = GetTopWidgetRect(main_rect,
                                              top_widget);
  main_rect = GetMapRectBelow(main_rect, top_rect);
  if (HaveTopWidget())
    top_widget->Move(top_rect);

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
MainWindow::SetWidget(Widget *_widget) noexcept
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
MainWindow::GetFlavourWidget(const TCHAR *flavour) noexcept
{
  return InputEvents::IsFlavour(flavour)
    ? widget
    : nullptr;
}

void
MainWindow::ShowMenu(const Menu &menu, const Menu *overlay, bool full) noexcept
{
  assert(menu_bar != nullptr);

  MenuGlue::Set(*menu_bar, menu, overlay, full);
}

bool
MainWindow::IsMenuButtonEnabled(unsigned idx) noexcept
{
  assert(menu_bar != nullptr);

  return menu_bar->IsButtonEnabled(idx);
}

void
MainWindow::UpdateVarioGaugeVisibility() noexcept
{
  bool full_screen = GetFullScreen();

  vario.SetVisible(!full_screen &&
                   !CommonInterface::GetUIState().screen_blanked);
}

void
MainWindow::UpdateGaugeVisibility() noexcept
{
  UpdateVarioGaugeVisibility();
  UpdateTrafficGaugeVisibility();
}

void
MainWindow::UpdateTrafficGaugeVisibility() noexcept
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

    if (!flarm.traffic.InCloseRange())
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
MainWindow::GetProjection() const noexcept
{
  AssertThread();
  assert(map != nullptr);

  return map->VisibleProjection();
}

void
MainWindow::ToggleSuppressFLARMRadar() noexcept
{
  suppress_traffic_gauge = !suppress_traffic_gauge;
}

void
MainWindow::ToggleForceFLARMRadar() noexcept
{
  force_traffic_gauge = !force_traffic_gauge;
  CommonInterface::SetUISettings().traffic.enable_gauge = force_traffic_gauge;
}
