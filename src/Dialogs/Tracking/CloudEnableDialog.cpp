// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CloudEnableDialog.hpp"
#include "Tracking/SkyLines/Features.hpp"
#include "Tracking/SkyLines/Key.hpp"
#include "Dialogs/Message.hpp"
#include "net/State.hpp"
#include "Asset.hpp"
#include "Interface.hpp"
#include "Simulator.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"
#include "UIGlobals.hpp"
#include "ui/window/SingleWindow.hpp"
#ifdef ANDROID
#include "Android/Main.hpp"
#include "java/Global.hxx"
#include "java/Class.hxx"
#include <mutex>

static Java::TrivialClass permission_manager_cls;
static jmethodID areLocationPermissionsGranted_method;
static std::once_flag permission_manager_init_flag;

static void
InitPermissionManagerCheck(JNIEnv *env) noexcept
{
  permission_manager_cls.Find(env, "org/xcsoar/PermissionManager");
  areLocationPermissionsGranted_method =
    env->GetMethodID(permission_manager_cls, "areLocationPermissionsGranted", "()Z");
}

static bool
AreLocationPermissionsGranted() noexcept
{
  if (permission_manager == nullptr)
    return false;

  JNIEnv *env = Java::GetEnv();
  if (env == nullptr)
    return false;

  std::call_once(permission_manager_init_flag, InitPermissionManagerCheck, env);

  if (areLocationPermissionsGranted_method == nullptr)
    return false;

  bool result = env->CallBooleanMethod(permission_manager, areLocationPermissionsGranted_method);

  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return false;
  }

  return result;
}
#endif

void
CloudEnableDialog() noexcept
{
#ifdef HAVE_SKYLINES_TRACKING
  auto &settings =
    CommonInterface::SetComputerSettings().tracking.skylines.cloud;
  if (settings.enabled != TriState::UNKNOWN)
    /* explicitly enabled or disabled - don't ask further questions */
    return;

#ifdef ANDROID
  /* On Android, check location permissions early (fail fast).
     This prevents the dialog from overlapping with permission consent dialogs.
     On Android, the dialog is also shown from the permission consent flow,
     but ProcessTimer may call this before permissions are granted. */
  if (!AreLocationPermissionsGranted())
    return;
#endif

  if (UIGlobals::GetMainWindow().HasDialog())
    /* don't ask stupid questions while the user operates in another
       modal dialog */
    return;

  if (is_simulator())
    /* only check in "Fly" mode */
    return;

  if (CommonInterface::MovementDetected())
    /* don't ask stupid questions if we're flying or about to */
    return;

#ifdef HAVE_NET_STATE
  if (GetNetState() != NetState::CONNECTED)
    /* this feature requires an internet connection; don't bother
       asking if we're not connected */
    return;
#endif

  const TCHAR *msg = _("The XCSoar project is currently developing a revolutionary service which allows sharing thermal/wave locations and more with other pilots.\n"
                       "Do you wish to participate in the field test? This means that your position, thermal/wave locations and other weather data will be transmitted to our test server. You can disable it at any time in the \"Tracking\" settings.\n"
                       "Please help us improve XCSoar!");

  int result = ShowMessageBox(msg, _T("XCSoar Cloud"),
                              MB_YESNOCANCEL|MB_ICONQUESTION);

  switch (result) {
  case IDYES:
    settings.enabled = TriState::TRUE;
    Profile::Set(ProfileKeys::CloudEnabled, true);

    if (settings.key == 0) {
      settings.key = SkyLinesTracking::GenerateKey();

      char s[64];
      snprintf(s, sizeof(s), "%llx",
               (unsigned long long)settings.key);
      Profile::Set(ProfileKeys::CloudKey, s);
    }

    Profile::Save();
    break;

  case IDNO:
    settings.enabled = TriState::FALSE;
    Profile::Set(ProfileKeys::CloudEnabled, false);
    Profile::Save();
    break;

  default:
    /* prevent further dialogs during this XCSoar run, but ask again
       later (don't save to profile) */
    settings.enabled = TriState::FALSE;
  }
#endif
}
