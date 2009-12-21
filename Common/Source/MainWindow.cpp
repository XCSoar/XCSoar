#include "MainWindow.hpp"
#include "resource.h"
#include "Protection.hpp"
#include "InfoBoxManager.h"
#include "Interface.hpp"
#include "ButtonLabel.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Components.hpp"
#include "ProcessTimer.hpp"
#include "LogFile.hpp"
#include "InfoBoxLayout.h"
#include "Screen/Fonts.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Gauge/GaugeVario.hpp"
#include "Gauge/GaugeCDI.hpp"

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
#ifdef WINDOWSPC
  wc.cbWndExtra = 0;
#else
  WNDCLASS dc;
  GetClassInfo(hInstance,TEXT("DIALOG"),&dc);
  wc.cbWndExtra                 = dc.cbWndExtra ;
#endif
  wc.hInstance                  = hInstance;
#if defined(GNAV) && !defined(PCGNAV)
  wc.hIcon = NULL;
#else
  wc.hIcon                      = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_XCSOARSWIFT));
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
  TopWindow::set(_T("XCSoarMain"), text, left, top, width, height);

  RECT rc;
#ifdef WINDOWSPC
  rc.left = 0;
  rc.right = SCREENWIDTH;
  rc.top = 0;
  rc.bottom = SCREENHEIGHT;
#else
  rc = get_client_rect();
#endif

  Layout::Initalize(rc.right - rc.left, rc.bottom - rc.top);

  StartupStore(TEXT("InfoBox geometry\n"));
  InfoBoxLayout::ScreenGeometry(rc);

  // color/pattern chart (must have infobox geometry before this)
  MapGfx.Initialise(XCSoarInterface::hInst,
		    XCSoarInterface::SettingsMap());

  StartupStore(TEXT("Create info boxes\n"));
  RECT rcsmall = InfoBoxManager::Create(rc);

  StartupStore(TEXT("Create button labels\n"));
  ButtonLabel::CreateButtonLabels(*this, rc);
  ButtonLabel::SetLabelText(0,TEXT("MODE"));

  StartupStore(TEXT("Initialise fonts\n"));
  InitialiseFonts(get_canvas(), rc);

  map.set(*this, rcsmall, rc);
  map.set_font(MapWindowFont);
  map.SetMapRect(rcsmall);

  vario = new GaugeVario(*this,
                         rc.right - InfoBoxLayout::ControlWidth, 0,
                         InfoBoxLayout::ControlWidth,
                         InfoBoxLayout::ControlHeight * 3);

  flarm = new GaugeFLARM(*this,
                         rc.right - InfoBoxLayout::ControlWidth * 2 + 1,
                         rc.bottom - InfoBoxLayout::ControlHeight * 2 + 1,
                         InfoBoxLayout::ControlWidth * 2 - 1,
                         InfoBoxLayout::ControlHeight * 2 - 1);

  StartupStore(TEXT("Initialise message system\n"));
  popup.set(rc);
}

// Windows event handlers

Brush *
MainWindow::on_color(Window &window, Canvas &canvas)
{
  int i = ButtonLabel::Find(window);
  if (i >= 0) {
    canvas.set_background_color(MapGfx.ColorButton);
    canvas.set_text_color(window.is_enabled()
                          ? MapGfx.ColorBlack
                          : MapGfx.ColorMidGrey);
    return &MapGfx.buttonBrush;
  }

  return TopWindow::on_color(window, canvas);
}

bool
MainWindow::on_activate()
{
  TopWindow::on_activate();

  full_screen();

  return true;
}

bool
MainWindow::on_timer(timer_t id)
{
  if (id != timer_id)
    return TopWindow::on_timer(id);

  if (globalRunningEvent.test()) {
    XCSoarInterface::AfterStartup();
    ProcessTimer::Process();
  }
  return true;
}

bool MainWindow::on_create(void)
{
  TopWindow::on_create();

  timer_id = set_timer(1000, 500); // 2 times per second

  return true;
}

bool MainWindow::on_destroy(void) {
  kill_timer(timer_id);

  TopWindow::on_destroy();

  post_quit();
  return true;
}

bool MainWindow::on_close() {
  if (XCSoarInterface::CheckShutdown()) {
    XCSoarInterface::Shutdown();
  }
  return true;
}
