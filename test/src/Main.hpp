/*
 * This header contains common boilerplate initialisation code for a
 * lot of debug programs.
 *
 */

#ifdef ENABLE_CMDLINE
#include "OS/Args.hpp"
#endif

#ifdef ENABLE_XML_DIALOG
#include "Dialogs/XML.hpp"
#define ENABLE_DIALOG
#define ENABLE_RESOURCE_LOADER
#endif

#if defined(ENABLE_RESOURCE_LOADER) && defined(USE_GDI)
#include "ResourceLoader.hpp"
#endif

#ifdef ENABLE_DIALOG
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"
#define ENABLE_DIALOG_LOOK
#endif

#ifdef ENABLE_MAIN_WINDOW
#include "Screen/SingleWindow.hpp"
#include "UIGlobals.hpp"
#endif

#ifdef ENABLE_LOOK
#include "Look/Look.hpp"
#include "UISettings.hpp"
#define ENABLE_SCREEN
#elif defined(ENABLE_DIALOG_LOOK)
#include "Look/DialogLook.hpp"
#define ENABLE_SCREEN
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
static SingleWindow main_window;

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
#ifdef _WIN32_WCE
        LPWSTR lpCmdLine,
#else
        LPSTR lpCmdLine2,
#endif
        int nCmdShow)
#endif
{
#ifdef ENABLE_CMDLINE
#ifdef WIN32
  Args args(GetCommandLine(), USAGE);
#else
  Args args(argc, argv, USAGE);
#endif
  ParseCommandLine(args);
  args.ExpectEnd();
#endif

#if defined(ENABLE_RESOURCE_LOADER) && defined(USE_GDI)
  ResourceLoader::Init(hInstance);
#endif

#ifdef ENABLE_SCREEN
  ScreenGlobalInit screen_init;
  Layout::Initialize({640, 480});
  InitialiseFonts();
#endif

#ifdef ENABLE_DIALOG
  dialog_settings.SetDefaults();
#endif

#ifdef ENABLE_LOOK
  look = new Look();
  look->Initialise(normal_font, bold_font, small_font,
                   normal_font);

  {
    UISettings ui_settings;
    ui_settings.SetDefaults();
    look->InitialiseConfigured(ui_settings,
                               normal_font, bold_font, small_font,
                               normal_font, bold_font, small_font,
                               small_font, monospace_font,
                               normal_font, small_font,
#ifndef GNAV
                               small_font,
#endif
                               small_font);
  }

  dialog_look = &look->dialog;
#elif defined(ENABLE_DIALOG_LOOK)
  dialog_look = new DialogLook();
  dialog_look->Initialise(bold_font, normal_font, small_font,
                          bold_font, bold_font, bold_font);
#endif

#ifdef ENABLE_XML_DIALOG
  SetXMLDialogLook(*dialog_look);
#endif

#ifdef ENABLE_DATA_PATH
  InitialiseDataPath();
#endif

#ifdef ENABLE_PROFILE
  Profile::SetFiles(_T(""));
  Profile::Load();
#endif

#ifdef ENABLE_MAIN_WINDOW
  main_window.Create(_T("Test"), {640, 480});
  main_window.Show();
#endif

  Main();

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
#endif

#ifdef ENABLE_SCREEN
  DeinitialiseFonts();
#endif

  return 0;
}
