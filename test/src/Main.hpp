/*
 * This header contains common boilerplate initialisation code for a
 * lot of debug programs.
 *
 */

#include "Util/PrintException.hxx"

#if defined(ENABLE_CMDLINE) || defined(ENABLE_MAIN_WINDOW)
#include "OS/Args.hpp"
#endif

#if defined(ENABLE_MAIN_WINDOW) && !defined(ENABLE_CMDLINE)
#define USAGE "-WxH"
#endif

#if defined(ENABLE_RESOURCE_LOADER) && defined(USE_GDI)
#include "ResourceLoader.hpp"
#endif

#ifdef ENABLE_DIALOG
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"
#define ENABLE_DIALOG_LOOK
#endif

#ifdef ENABLE_CLOSE_BUTTON
#define ENABLE_MAIN_WINDOW
#define ENABLE_BUTTON_LOOK
#include "Form/Button.hpp"
#endif

#ifdef ENABLE_MAIN_WINDOW
#include "Screen/SingleWindow.hpp"
#include "Form/ActionListener.hpp"
#include "UIGlobals.hpp"
#include "Util/CharUtil.hpp"
#include "Util/NumberParser.hpp"
#define ENABLE_SCREEN
#endif

#ifdef ENABLE_LOOK
#include "Look/Look.hpp"
#include "UISettings.hpp"
#define ENABLE_DIALOG_LOOK
#define ENABLE_SCREEN
#elif defined(ENABLE_DIALOG_LOOK)
#include "Look/DialogLook.hpp"
#define ENABLE_SCREEN
#elif defined(ENABLE_BUTTON_LOOK)
#include "Look/ButtonLook.hpp"
#define ENABLE_SCREEN
#endif

#ifdef ENABLE_DIALOG_LOOK
#define ENABLE_BUTTON_LOOK
#endif

#ifdef ENABLE_SCREEN
#include "Screen/Init.hpp"
#include "Screen/Layout.hpp"
#include "Fonts.hpp"
#endif

#ifdef ENABLE_PROFILE
#include "Profile/Profile.hpp"
#define ENABLE_DATA_PATH
#endif

#ifdef ENABLE_DATA_PATH
#include "LocalPath.hpp"
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <stdexcept>

#include <stdio.h>

#ifdef ENABLE_CMDLINE
static void
ParseCommandLine(Args &args);
#endif

static void Main();

#ifdef ENABLE_LOOK
static Look *look;
#endif

#ifdef ENABLE_DIALOG_LOOK
static DialogLook *dialog_look;
#endif

#ifdef ENABLE_BUTTON_LOOK
static ButtonLook *button_look;
#endif

#ifdef ENABLE_DIALOG
static DialogSettings dialog_settings;

const DialogSettings &
UIGlobals::GetDialogSettings()
{
  return dialog_settings;
}

const DialogLook &
UIGlobals::GetDialogLook()
{
  return *dialog_look;
}
#endif

#ifdef ENABLE_MAIN_WINDOW

class TestMainWindow : public SingleWindow, public ActionListener {
  Window *full_window;

#ifdef ENABLE_CLOSE_BUTTON
  Button close_button;
#endif

public:
  enum Buttons {
    CLOSE,
    LAST_BUTTON
  };

  TestMainWindow():full_window(nullptr) {}

  /**
   * Configure a #Window that will be auto-resize to the full client
   * area.
   */
  void SetFullWindow(Window &w) {
    full_window = &w;
  }

  /* virtual methods from class ActionListener */
  void OnAction(int id) override {
    switch (id) {
    case CLOSE:
      Close();
      break;
    }
  }

protected:
  /* virtual methods from class Window */
  void OnCreate() override {
    SingleWindow::OnCreate();

#ifdef ENABLE_CLOSE_BUTTON
    close_button.Create(*this, *button_look, _T("Close"),
                        GetCloseButtonRect(GetClientRect()),
                        WindowStyle(),
                        *this, CLOSE);
#endif
  }

  void OnResize(PixelSize new_size) override {
    SingleWindow::OnResize(new_size);
    Layout::Initialize(new_size);

    if (full_window != nullptr)
      full_window->Resize(new_size);
  }

protected:
#ifdef ENABLE_CLOSE_BUTTON
  gcc_pure
  PixelRect GetCloseButtonRect(PixelRect rc) const {
    rc.right -= 5;
    rc.left = rc.right - 120;
    rc.top += 5;
    rc.bottom = rc.top + 50;
    return rc;
  }
#endif
};

static TestMainWindow main_window;

SingleWindow &
UIGlobals::GetMainWindow()
{
  return main_window;
}
#endif

#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine2,
        int nCmdShow)
#endif
{
#if defined(ENABLE_CMDLINE) || defined(ENABLE_MAIN_WINDOW)
#ifdef WIN32
  Args args(GetCommandLine(), USAGE);
#else
  Args args(argc, argv, USAGE);
#endif
#ifdef ENABLE_MAIN_WINDOW
  PixelSize window_size{640, 480};
  const char *a = args.PeekNext();
  if (a != nullptr && a[0] == '-' && IsDigitASCII(a[1])) {
    args.GetNext();
    char *p;
    window_size.cx = ParseUnsigned(a + 1, &p);
    if (*p != 'x' && *p != 'X')
      args.UsageError();
    a = p;
    window_size.cy = ParseUnsigned(a + 1, &p);
    if (*p != '\0')
      args.UsageError();
  }
#endif
#ifdef ENABLE_CMDLINE
  ParseCommandLine(args);
#endif
  args.ExpectEnd();
#endif

#if defined(ENABLE_RESOURCE_LOADER) && defined(USE_GDI)
  ResourceLoader::Init(hInstance);
#endif

#ifdef ENABLE_SCREEN
#ifndef ENABLE_MAIN_WINDOW
  constexpr PixelSize window_size{800, 600};
#endif

  ScreenGlobalInit screen_init;
  Layout::Initialize(window_size);
  InitialiseFonts();
#endif

#ifdef ENABLE_DIALOG
  dialog_settings.SetDefaults();
#endif

#ifdef ENABLE_LOOK
  look = new Look();
  look->Initialise(normal_font);

  {
    UISettings ui_settings;
    ui_settings.SetDefaults();
    look->InitialiseConfigured(ui_settings,
                               normal_font, bold_font,
                               100);
  }

  dialog_look = &look->dialog;
  button_look = &dialog_look->button;
#elif defined(ENABLE_DIALOG_LOOK)
  dialog_look = new DialogLook();
  dialog_look->Initialise();
  button_look = &dialog_look->button;
#elif defined(ENABLE_BUTTON_LOOK)
  button_look = new ButtonLook();
  button_look->Initialise(bold_font);
#endif

#ifdef ENABLE_DATA_PATH
  InitialiseDataPath();
#endif

#ifdef ENABLE_PROFILE
  Profile::SetFiles(nullptr);
  Profile::Load();
#endif

#ifdef ENABLE_MAIN_WINDOW
  main_window.Create(_T("Test"), window_size);
  main_window.Show();
#endif

  int result = EXIT_SUCCESS;
  try {
    Main();
  } catch (const std::exception &e) {
    PrintException(e);
    result = EXIT_FAILURE;
  }

#ifdef ENABLE_MAIN_WINDOW
  main_window.Destroy();
#endif

#ifdef ENABLE_DATA_PATH
  DeinitialiseDataPath();
#endif

#ifdef ENABLE_LOOK
  delete look;
#elif defined(ENABLE_DIALOG_LOOK)
  delete dialog_look;
#elif defined(ENABLE_BUTTON_LOOK)
  delete button_look;
#endif

#ifdef ENABLE_SCREEN
  DeinitialiseFonts();
#endif

  return result;
}
