// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Permissions.hpp"
#include "Main.hpp"
#include "java/Global.hxx"
#include "java/String.hxx"
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/QuickGuidePageWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "util/Compiler.h"

#include <jni.h>
#include <mutex>
#include <string>

/**
 * Optional callback invoked on the native UI thread when a permission
 * request completes.  Protected by #permission_callback_mutex.
 */
static std::mutex permission_callback_mutex;
static std::function<void(bool)> permission_callback;

bool
AreLocationPermissionsGranted() noexcept
{
  if (permission_manager == nullptr)
    return false;

  JNIEnv *env = Java::GetEnv();
  jclass cls = env->GetObjectClass(permission_manager);
  jmethodID mid = env->GetMethodID(cls, "areLocationPermissionsGranted",
                                   "()Z");
  env->DeleteLocalRef(cls);
  if (mid == nullptr)
    return false;

  return env->CallBooleanMethod(permission_manager, mid);
}

bool
IsNotificationPermissionGranted() noexcept
{
  if (permission_manager == nullptr)
    return true;

  JNIEnv *env = Java::GetEnv();
  jclass cls = env->GetObjectClass(permission_manager);
  jmethodID mid = env->GetMethodID(cls, "isNotificationPermissionGranted",
                                   "()Z");
  env->DeleteLocalRef(cls);
  if (mid == nullptr)
    return true;

  return env->CallBooleanMethod(permission_manager, mid);
}

void
RequestLocationPermissions() noexcept
{
  if (permission_manager == nullptr)
    return;

  JNIEnv *env = Java::GetEnv();
  jclass cls = env->GetObjectClass(permission_manager);
  jmethodID mid = env->GetMethodID(cls,
                                   "requestAllLocationPermissionsDirect",
                                   "()V");
  env->DeleteLocalRef(cls);
  if (mid == nullptr)
    return;

  env->CallVoidMethod(permission_manager, mid);
}

void
RequestLocationPermissions(std::function<void(bool)> callback) noexcept
{
  {
    const std::lock_guard lock{permission_callback_mutex};
    permission_callback = std::move(callback);
  }
  RequestLocationPermissions();
}

void
RequestNotificationPermission() noexcept
{
  if (permission_manager == nullptr)
    return;

  JNIEnv *env = Java::GetEnv();
  jclass cls = env->GetObjectClass(permission_manager);
  jmethodID mid = env->GetMethodID(cls,
                                   "requestNotificationPermissionDirect",
                                   "()V");
  env->DeleteLocalRef(cls);
  if (mid == nullptr)
    return;

  env->CallVoidMethod(permission_manager, mid);
}

void
RequestNotificationPermission(std::function<void(bool)> callback) noexcept
{
  {
    const std::lock_guard lock{permission_callback_mutex};
    permission_callback = std::move(callback);
  }
  RequestNotificationPermission();
}

void
SuppressPermissionDialogs() noexcept
{
  if (permission_manager == nullptr)
    return;

  JNIEnv *env = Java::GetEnv();
  jclass cls = env->GetObjectClass(permission_manager);
  jmethodID mid = env->GetMethodID(cls,
                                   "suppressPermissionDialogs",
                                   "()V");
  env->DeleteLocalRef(cls);
  if (mid == nullptr)
    return;

  env->CallVoidMethod(permission_manager, mid);
}

/* ---- Native permission disclosure dialog ---- */

/**
 * Send the disclosure result back to Java's PermissionManager.
 * Can be called from any thread.
 */
static void
SendDisclosureResult(bool accepted) noexcept
{
  if (permission_manager == nullptr)
    return;

  JNIEnv *env = Java::GetEnv();
  if (env == nullptr)
    return;

  jclass cls = env->GetObjectClass(permission_manager);
  jmethodID mid = env->GetMethodID(cls, "onDisclosureResult", "(Z)V");
  env->DeleteLocalRef(cls);
  if (mid == nullptr)
    return;

  env->CallVoidMethod(permission_manager, mid,
                      accepted ? JNI_TRUE : JNI_FALSE);

  if (env->ExceptionCheck())
    env->ExceptionClear();
}

/**
 * Get the disclosure text for a given Android permission string.
 */
static const char *
GetDisclosureText(const char *permission) noexcept
{
  if (strstr(permission, "ACCESS_FINE_LOCATION") != nullptr)
    return "![](resource:IDB_LOCATION_PIN)\n\n"
      "# Location Access\n\n"
      "XCSoar collects location data to enable flight navigation, "
      "thermal mapping, and flight logging even when the app is "
      "closed or not in use.\n\n"
      "- **GPS Position** - Real-time navigation and glide computer\n"
      "- **Background Location** - Continuous flight recording when "
      "the screen is off or another app is in the foreground\n"
      "- **Foreground Service** - Keeps GPS active during your "
      "flight\n\n"
      "Your location data is stored locally on your device. It is "
      "not shared unless you explicitly enable tracking in "
      "[Config > Tracking](xcsoar://config/tracking).\n\n"
      "[Privacy Policy](https://github.com/XCSoar/XCSoar/blob/master/PRIVACY.md)";

  if (strstr(permission, "ACCESS_BACKGROUND_LOCATION") != nullptr)
    return "# Background Location\n\n"
      "XCSoar collects location data to enable continuous flight "
      "logging even when the app is closed or not in use.\n\n"
      "Without this permission, flight recording may stop when "
      "the screen is off or another app is in the foreground.\n\n"
      "[Privacy Policy](https://github.com/XCSoar/XCSoar/blob/master/PRIVACY.md)";

  if (strstr(permission, "FOREGROUND_SERVICE_LOCATION") != nullptr)
    return "# Foreground Service\n\n"
      "XCSoar needs this permission to keep GPS active during "
      "your flight. Android requires it to run a location "
      "service in the foreground.\n\n"
      "Without this permission, GPS tracking may be interrupted.";

  if (strstr(permission, "BLUETOOTH") != nullptr)
    return "![](resource:IDB_BLUETOOTH)\n\n"
      "# Bluetooth Access\n\n"
      "XCSoar uses Bluetooth to connect to external flight "
      "instruments such as variometers, FLARM devices, and "
      "other sensors.\n\n"
      "If you want XCSoar to communicate with Bluetooth "
      "devices, grant this permission.";

  if (strstr(permission, "POST_NOTIFICATIONS") != nullptr)
    return "![](resource:IDB_NOTIFICATION_BELL)\n\n"
      "# Notifications\n\n"
      "XCSoar needs notification permission to maintain a "
      "persistent notification while recording your flight. "
      "This notification is required by Android for background "
      "operation and provides a quick way to return to the app.\n\n"
      "Without this permission, Android may stop flight recording "
      "when the app is in the background.";

  /* Fallback for unknown permissions */
  return "# Permission Required\n\n"
    "XCSoar needs this permission to function properly.";
}

/**
 * Get the dialog title for a given Android permission string.
 */
static const char *
GetDisclosureTitle(const char *permission) noexcept
{
  if (strstr(permission, "FINE_LOCATION") != nullptr)
    return N_("Location Access");
  if (strstr(permission, "BACKGROUND_LOCATION") != nullptr)
    return N_("Background Location");
  if (strstr(permission, "FOREGROUND_SERVICE") != nullptr)
    return N_("Foreground Service");
  if (strstr(permission, "BLUETOOTH") != nullptr)
    return N_("Bluetooth Access");
  if (strstr(permission, "POST_NOTIFICATIONS") != nullptr)
    return N_("Notifications");
  return N_("Permission Required");
}

/**
 * Show the disclosure dialog on the native UI thread.
 * The permission string is passed via a heap-allocated std::string.
 */
static void
ShowDisclosureOnUIThread(void *ctx) noexcept
{
  auto *permission = static_cast<std::string *>(ctx);
  const char *text = GetDisclosureText(permission->c_str());
  const char *title = GetDisclosureTitle(permission->c_str());

  delete permission;

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{},
                      UIGlobals::GetMainWindow(),
                      look, gettext(title));

  bool accepted = false;

  auto page = QuickGuidePageWidget::CreateTwoButtonPage(
    look, text,
    _("Continue"),
    [&dialog, &accepted]() {
      accepted = true;
      dialog.SetModalResult(mrOK);
    },
    _("Not Now"),
    [&dialog]() {
      dialog.SetModalResult(mrCancel);
    });

  dialog.FinishPreliminary(std::move(page));
  dialog.ShowModal();

  SendDisclosureResult(accepted);
}

extern "C"
gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_showPermissionDisclosure(
  JNIEnv *env, [[maybe_unused]] jclass cls,
  jstring permission)
{
  if (UI::event_queue == nullptr)
    return;

  /* Copy the permission string to the heap so it survives until
     the native UI thread processes the event */
  auto *perm = new std::string(
    Java::String::GetUTFChars(env, permission).c_str());

  UI::event_queue->InjectCall(ShowDisclosureOnUIThread, perm);
}

/* ---- Permission result callback ---- */

/**
 * Run on the native UI thread via InjectCall to invoke the
 * pending permission callback.
 */
static void
DispatchPermissionResult(void *ctx) noexcept
{
  const bool granted = ctx != nullptr;

  std::function<void(bool)> cb;
  {
    const std::lock_guard lock{permission_callback_mutex};
    cb = std::move(permission_callback);
    permission_callback = nullptr;
  }

  if (cb)
    cb(granted);
}

extern "C"
gcc_visibility_default
JNIEXPORT void JNICALL
Java_org_xcsoar_NativeView_onPermissionResult(
  [[maybe_unused]] JNIEnv *env, [[maybe_unused]] jclass cls,
  jboolean granted)
{
  if (UI::event_queue == nullptr)
    return;

  /* Use the void* ctx pointer to pass the boolean: non-null = granted */
  UI::event_queue->InjectCall(DispatchPermissionResult,
                              granted ? reinterpret_cast<void *>(1)
                                      : nullptr);
}
