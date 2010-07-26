#include "MainWindow.hpp"
#include "resource.h"
#include "Protection.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Interface.hpp"
#include "ButtonLabel.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Components.hpp"
#include "ProcessTimer.hpp"
#include "LogFile.hpp"
#include "Screen/Fonts.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Gauge/GaugeVario.hpp"
#include "Gauge/GaugeCDI.hpp"
#include "MenuBar.hpp"
#include "InputEvents.h"
#include "Appearance.hpp"

#ifdef WINDOWSPC
#include "Asset.hpp" /* for SCREENWIDTH and SCREENHEIGHT */
#endif

/**
 * Destructor of the MainWindow-Class
 * @return
 */
MainWindow::~MainWindow()
{
  if (vario != NULL)
    delete vario;
  if (flarm != NULL)
    delete flarm;
}

bool
MainWindow::register_class(HINSTANCE hInstance)
{
#ifdef ENABLE_SDL
  return true;
#else /* !ENABLE_SDL */
  WNDCLASS wc;

  wc.style                      = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = Window::WndProc;
  wc.cbClsExtra                 = 0;
  wc.cbWndExtra = 0;
  wc.hInstance                  = hInstance;
#if defined(GNAV) && !defined(PCGNAV)
  wc.hIcon = NULL;
#else
  wc.hIcon                      = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XCSOAR));
#endif
  wc.hCursor                    = 0;
  wc.hbrBackground              = (HBRUSH) GetStockObject(WHITE_BRUSH);
  wc.lpszMenuName               = 0;
  wc.lpszClassName = _T("XCSoarMain");

  return (RegisterClass(&wc)!= FALSE);
#endif /* !ENABLE_SDL */
}

void
MainWindow::set(LPCTSTR text,
                int left, int top, unsigned width, unsigned height)
{
  SingleWindow::set(_T("XCSoarMain"), text, left, top, width, height);

  RECT rc;
#ifdef WINDOWSPC
  rc.left = 0;
  rc.right = SCREENWIDTH;
  rc.top = 0;
  rc.bottom = SCREENHEIGHT;
#else
  rc = get_client_rect();
#endif

  Layout::Initialize(rc.right - rc.left, rc.bottom - rc.top);

  LogStartUp(_T("InfoBox geometry"));
  InfoBoxLayout::Init(rc);
  RECT map_rect = InfoBoxLayout::GetRemainingRect(rc);

  // color/pattern chart (must have infobox geometry before this)
  MapGfx.Initialise(XCSoarInterface::SettingsMap());

  LogStartUp(_T("Initialise fonts"));
  Fonts::Initialize(Appearance);

  LogStartUp(_T("Create info boxes"));
  InfoBoxManager::Create(rc);

  LogStartUp(_T("Create button labels"));
  ButtonLabel::CreateButtonLabels(*this);
  ButtonLabel::SetLabelText(0,_T("MODE"));
  ButtonLabel::SetFont(Fonts::MapBold);

  map.set(*this, map_rect, rc);
  map.set_font(Fonts::Map);
  map.SetMapRect(map_rect);

  vario = new GaugeVario(*this,
                         rc.right - InfoBoxLayout::ControlWidth, 0,
                         InfoBoxLayout::ControlWidth,
                         InfoBoxLayout::ControlHeight * 3);

  flarm = new GaugeFLARM(*this,
                         rc.right - InfoBoxLayout::ControlWidth * 2 + 1,
                         rc.bottom - InfoBoxLayout::ControlHeight * 2 + 1,
                         InfoBoxLayout::ControlWidth * 2 - 1,
                         InfoBoxLayout::ControlHeight * 2 - 1);
  flarm->bring_to_top();

  LogStartUp(_T("Initialise message system"));
  popup.set(rc);
}

// Windows event handlers

bool
MainWindow::on_command(unsigned id, unsigned code)
{
  if (id >= MenuBar::FIRST_ID && id <= MenuBar::LAST_ID) {
    InputEvents::processButton(id - MenuBar::FIRST_ID);
    return true;
  } else
    return SingleWindow::on_command(id, code);
}

bool
MainWindow::on_activate()
{
  SingleWindow::on_activate();

  full_screen();

  return true;
}

bool
MainWindow::on_setfocus()
{
  if (!has_dialog()) {
    /* the main window should never have the keyboard focus; if we
       happen to get the focus despite of that, forward it to the map
       window to make keyboard shortcuts work */
    if (map.defined())
      map.set_focus();
    return true;
  }

  return SingleWindow::on_setfocus();
}

bool
MainWindow::on_timer(timer_t id)
{
  if (id != timer_id)
    return SingleWindow::on_timer(id);

  if (globalRunningEvent.test()) {
    XCSoarInterface::AfterStartup();
    ProcessTimer::Process();
  }
  return true;
}

bool MainWindow::on_create(void)
{
  SingleWindow::on_create();

  timer_id = set_timer(1000, 500); // 2 times per second

  return true;
}

bool MainWindow::on_destroy(void) {
  kill_timer(timer_id);

  SingleWindow::on_destroy();

  return true;
}

bool MainWindow::on_close() {
  if (SingleWindow::on_close())
    return true;

  if (XCSoarInterface::CheckShutdown()) {
    XCSoarInterface::Shutdown();
  }
  return true;
}
