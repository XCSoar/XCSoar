// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "dlgQuickGuide.hpp"
#include "dlgGestureHelp.hpp"
#include "WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/QuickGuidePageWidget.hpp"
#include "Widget/VScrollWidget.hpp"
#include "Widget/RichTextWidget.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Keys.hpp"
#include "system/Path.hpp"
#include "Language/Language.hpp"
#include "Version.hpp"
#include "Simulator.hpp"
#include "Inflate.hpp"
#include "Message.hpp"
#include "Interface.hpp"
#include "Device/Config.hpp"
#include "Tracking/SkyLines/Features.hpp"
#ifdef HAVE_SKYLINES_TRACKING
#include "Tracking/SkyLines/Key.hpp"
#endif
#ifdef ANDROID
#include "Android/Permissions.hpp"
#endif
#include "util/AllocatedString.hxx"
#include "util/ConvertString.hpp"
#include "util/StringCompare.hxx"
#include "util/StaticString.hxx"

#include <vector>

extern "C"
{
  extern const uint8_t NEWS_txt_gz[];
  extern const size_t NEWS_txt_gz_size;
}

// Page indices for the can-advance guard
static constexpr unsigned INVALID_PAGE = ~0u;

/**
 * Track conditional page indices for post-dialog result handling.
 */
struct QuickGuideState {
  unsigned warranty_page_index = INVALID_PAGE;
  unsigned news_page_index = INVALID_PAGE;
  unsigned cloud_page_index = INVALID_PAGE;
  unsigned location_page_index = INVALID_PAGE;
  bool warranty_accepted = false;
  QuickGuidePageWidget *warranty_widget = nullptr;
};

/* ---- Welcome / Logo page text ---- */

static const char *
GetWelcomeText(bool dark_mode)
{
  static StaticString<1024> welcome;
  welcome.Format(
    "![XCSoar Logo](resource:IDB_LOGO_HD)\n\n"
    "![XCSoar](resource:%s)\n\n"
    "**Version %s**\n\n"
    "%s\n\n"
    "%s\n\n"
    "- [%s](https://xcsoar.org/discover/manual.html)\n"
    "- [%s](https://github.com/XCSoar/XCSoar)\n"
    "- [%s](https://github.com/XCSoar/XCSoar/discussions)\n"
    "- [https://xcsoar.org](https://xcsoar.org)",
    dark_mode ? "IDB_TITLE_HD_WHITE" : "IDB_TITLE_HD",
    XCSoar_VersionString,
    _("To get the most out of XCSoar and to learn about its many "
      "functions in detail, it is highly recommended to read the "
      "Quick Guide or the complete documentation."),
    _("Documentation is available in several languages. "
      "You can always access the latest versions online:"),
    _("XCSoar Manual & Quick Guide"),
    _("GitHub - Source Code & Contributions"),
    _("GitHub Discussions - Questions & Community"));
  return welcome.c_str();
}

/* ---- Disclaimer / Warranty text ---- */

static constexpr const char *disclaimer_text =
  "![](resource:IDB_WARNING_TRIANGLE)\n\n"
  "!!! warning\n"
  "# Important Safety Notice\n\n"
  "By using XCSoar, you acknowledge and accept the following:\n\n"
  "## Limitations\n\n"
  "- XCSoar is for **situational awareness only**\n"
  "- XCSoar is **not** a FLARM display\n"
  "- XCSoar is **not** aviation certified in any way\n"
  "- The artificial horizon is **not** fit for any purpose\n"
  "- Databanks (airspace, waypoints) may contain errors, be "
  "incomplete, or **not** up to date\n"
  "- XCSoar is **not** guaranteed to be error free\n\n"
  "## Pilot Responsibility\n\n"
  "The **Pilot in Command** is always responsible for the safe "
  "operation of the aircraft. Never rely solely on XCSoar for "
  "navigation or situational awareness.\n\n"
  "## No Warranty (GPL Section 11)\n\n"
  "BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY "
  "FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN "
  "OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES "
  "PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER "
  "EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED "
  "WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. "
  "THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM "
  "IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE "
  "COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.\n\n"
  "[Full GPL License](xcsoar://dialog/credits)";

/* ---- Configuration help text (dynamic, with checkbox status) ---- */

/**
 * Check whether at least one device slot is configured
 * (port type is not DISABLED).
 */
static bool
IsAnyDeviceConfigured() noexcept
{
  const auto &devices =
    CommonInterface::GetSystemSettings().devices;
  for (const auto &dev : devices) {
    if (!dev.IsDisabled())
      return true;
  }
  return false;
}

static const char *
GetConfigurationHelpText()
{
  const char *map = Profile::Get(ProfileKeys::MapFile);
  const bool has_map = map != nullptr && !StringIsEmpty(map);
  const auto plane_path = Profile::GetPath("PlanePath");
  const bool has_polar = plane_path != nullptr;
  const char *pilot = Profile::Get(ProfileKeys::PilotName);
  const bool has_pilot = pilot != nullptr && !StringIsEmpty(pilot);
  const bool has_device = IsAnyDeviceConfigured();

  bool weglide_enabled = false;
  Profile::Get(ProfileKeys::WeGlideEnabled, weglide_enabled);

  bool tim_enabled = false;
  Profile::Get(ProfileKeys::EnableThermalInformationMap, tim_enabled);

  static StaticString<1536> text;
  text.Format(
    "# Getting Started\n\n"
    "To use XCSoar effectively, configure the following:\n\n"
    "- [%s] [Map and data](xcsoar://config/site-files) - "
    "Download maps, waypoints, and airspace files for your "
    "region\n\n"
    "- [%s] [Aircraft polar](xcsoar://config/planes) - "
    "Set up your aircraft with the correct polar curve\n\n"
    "- [%s] [Pilot info](xcsoar://config/logger) - "
    "Enter your name and weight\n\n"
    "- [%s] [Devices](xcsoar://config/devices) - "
    "Connect your flight instruments\n\n"
    "- [%s] [WeGlide](xcsoar://config/weglide) - "
    "Upload flights automatically to WeGlide\n\n"
    "- [%s] [Thermal Information Map](xcsoar://config/weather) - "
    "Show thermal locations from thermalmap.info on the map\n\n"
    "- [ ] [Safety factors](xcsoar://config/safety) - "
    "Set arrival height, terrain clearance and polar degradation\n\n"
    "- [ ] [Terrain display](xcsoar://config/terrain) - "
    "Choose terrain colors, shading and contour lines\n\n"
    "- [ ] [Live tracking](xcsoar://config/tracking) *(optional)* - "
    "Share your position via SkyLines or LiveTrack24\n\n"
    "The easiest way to explore XCSoar is to "
    "[replay an existing IGC flight](xcsoar://dialog/replay).",
    has_map ? "x" : " ",
    has_polar ? "x" : " ",
    has_pilot ? "x" : " ",
    has_device ? "x" : " ",
    weglide_enabled ? "x" : " ",
    tim_enabled ? "x" : " ");
  return text.c_str();
}

/* ---- Preflight text ---- */

static constexpr const char *preflight_text =
  "# Preflight Checks\n\n"
  "Before each flight, verify:\n\n"
  "1. **Plane & Polar** - Correct aircraft and polar selected. "
  "[Config > Plane](xcsoar://config/planes)\n\n"
  "2. **Flight parameters** - Wing loading, bugs, QNH, max "
  "temperature. [Info > Flight](xcsoar://dialog/flight)\n\n"
  "3. **Wind** - Set wind manually or enable auto wind. "
  "[Info > Wind](xcsoar://dialog/wind)\n\n"
  "4. **Task** - Create a task for navigation guidance. "
  "[Nav > Task Manager](xcsoar://dialog/task)";

/* ---- Postflight text ---- */

static constexpr const char *postflight_text =
  "# After Your Flight\n\n"
  "1. **Download logs** - Retrieve flight logs from your "
  "FLARM or logger device. "
  "[Config > Devices](xcsoar://config/devices)\n\n"
  "2. **Analysis** - Review your flight statistics. "
  "[Info > Analysis](xcsoar://dialog/analysis)\n\n"
  "3. **Status** - Check flight timing and statistics. "
  "[Info > Status](xcsoar://dialog/status)\n\n"
  "4. **Upload** - Upload to WeGlide directly from XCSoar. "
  "Configure your WeGlide User ID in "
  "[Config > System > WeGlide](xcsoar://config/weglide)";

/* ---- Helpers ---- */

/**
 * Truncate the NEWS text to only the first (current) version
 * section.  Sections are delimited by lines starting with "Version ".
 * This avoids wrapping the entire multi-year release history which
 * would cause a multi-second stall on slow devices (e.g. RPi 3).
 */
static void
TruncateToCurrentVersion(char *text) noexcept
{
  /* Skip past the first "Version ..." line */
  char *p = text;
  while (*p != '\0' && *p != '\n')
    ++p;
  if (*p == '\n')
    ++p;

  /* Find the next "Version " line and terminate there */
  while (*p != '\0') {
    if (*p == '\n' && StringStartsWith(p + 1, "Version ")) {
      *p = '\0';
      return;
    }
    ++p;
  }
}

/**
 * Check if the warranty has already been acknowledged for the
 * current version.
 */
static bool
IsWarrantyAcknowledged() noexcept
{
  const char *acknowledged =
    Profile::Get(ProfileKeys::DisclaimerAcknowledgedVersion);
  return acknowledged != nullptr &&
         StringIsEqual(acknowledged, XCSoar_Version);
}

/**
 * Check if the user has already seen the news for this version.
 */
static bool
IsNewsSeen() noexcept
{
  const char *last_seen =
    Profile::Get(ProfileKeys::LastSeenNewsVersion);
  return last_seen != nullptr &&
         StringIsEqual(last_seen, XCSoar_Version);
}

/**
 * Check if the cloud consent is needed: the user has not yet
 * explicitly enabled or disabled XCSoar Cloud.
 */
[[gnu::pure]]
static bool
IsCloudConsentNeeded() noexcept
{
#ifdef HAVE_SKYLINES_TRACKING
  const auto &settings =
    CommonInterface::GetComputerSettings().tracking.skylines.cloud;
  return settings.enabled == TriState::UNKNOWN;
#else
  return false;
#endif
}

/* ---- Android permission disclosure texts ---- */

#ifdef ANDROID
/**
 * Google Play prominent disclosure for location permissions.
 * Must include the word "location" and indicate background use
 * with "when the app is closed or not in use" per Google Play
 * policy requirements.
 */
static constexpr const char *location_disclosure_text =
  "![](resource:IDB_LOCATION_PIN)\n\n"
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

/**
 * Google Play prominent disclosure for notification permission.
 */
static constexpr const char *notification_disclosure_text =
  "![](resource:IDB_NOTIFICATION_BELL)\n\n"
  "# Notifications\n\n"
  "XCSoar needs notification permission to maintain a persistent "
  "notification while recording your flight. This notification "
  "is required by Android for background operation and provides "
  "a quick way to return to the app.\n\n"
  "Without this permission, Android may stop flight recording "
  "when the app is in the background.";
#endif

/* ---- Cloud consent text ---- */

static constexpr const char *cloud_consent_text =
  "# XCSoar Cloud\n\n"
  "The XCSoar project offers a service that allows sharing "
  "thermal and wave locations with other pilots in real time.\n\n"
  "If you participate, your **position**, **thermal/wave "
  "locations** and other weather data will be transmitted to "
  "the XCSoar Cloud server.\n\n"
  "You can change this at any time in "
  "[Config > Tracking](xcsoar://config/tracking).";

#ifdef HAVE_SKYLINES_TRACKING
/**
 * Enable XCSoar Cloud: set the setting, generate a key if
 * needed, and save to profile.
 */
static void
EnableCloud()
{
  auto &settings =
    CommonInterface::SetComputerSettings().tracking.skylines.cloud;
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
}

/**
 * Disable XCSoar Cloud and save to profile.
 */
static void
DisableCloud()
{
  auto &settings =
    CommonInterface::SetComputerSettings().tracking.skylines.cloud;
  settings.enabled = TriState::FALSE;
  Profile::Set(ProfileKeys::CloudEnabled, false);
  Profile::Save();
}
#endif

/**
 * Check if the user has set "don't show again" for informational
 * pages.
 */
static bool
IsQuickGuideHidden() noexcept
{
  bool hidden = false;
  Profile::Get(ProfileKeys::HideQuickGuideDialogOnStartup, hidden);
  return hidden;
}

bool
dlgQuickGuideShowModal(bool force_info)
{
  const bool is_simulator = global_simulator_flag;
  const bool warranty_needed =
    !is_simulator && !IsWarrantyAcknowledged();
  const bool news_needed = !IsNewsSeen();
  const bool cloud_needed =
    !is_simulator && IsCloudConsentNeeded();
#ifdef ANDROID
  const bool permissions_needed =
    !is_simulator && (!AreLocationPermissionsGranted() ||
                      !IsNotificationPermissionGranted());
#else
  const bool permissions_needed = false;
#endif
  const bool info_pages_needed = force_info || !IsQuickGuideHidden();

  // If everything is already satisfied, skip the dialog entirely
  if (!warranty_needed && !news_needed &&
      !cloud_needed && !permissions_needed && !info_pages_needed)
    return true;

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{},
                      UIGlobals::GetMainWindow(),
                      look, _("Welcome to XCSoar"));

  QuickGuideState state;

  auto pager = std::make_unique<ArrowPagerWidget>(
    look.button, [&dialog, &state, warranty_needed]() {
      if (warranty_needed && !state.warranty_accepted) {
        if (ShowMessageBox(
              _("The safety disclaimer must be accepted "
                "to use XCSoar. Quit?"),
              _T("XCSoar"),
              MB_YESNO | MB_ICONWARNING) != IDYES)
          /* User chose not to quit — stay in the dialog */
          return;
      }
      dialog.SetModalResult(mrOK);
    });
  ArrowPagerWidget *pager_ptr = pager.get();

  // Track page titles for the caption callback
  std::vector<const char *> titles;

  /* Gesture callback for pager navigation via horizontal swipe */
  auto pager_gesture = [pager_ptr](bool next) {
    if (next) {
      if (pager_ptr->CanAdvance())
        pager_ptr->Next(true);
    } else {
      pager_ptr->Previous(true);
    }
  };

  /* Helper: create a VScrollWidget with swipe gesture support */
  auto make_scroll_page =
    [&look, &pager_gesture](std::unique_ptr<Widget> &&w) {
      auto scroll =
        std::make_unique<VScrollWidget>(std::move(w), look, true);
      scroll->SetGestureCallback(pager_gesture);
      return scroll;
    };

  /* ---- Logo / Welcome page (always shown) ---- */
  pager->Add(make_scroll_page(
    std::make_unique<RichTextWidget>(look, GetWelcomeText(look.dark_mode))));
  titles.push_back(_("Welcome"));

  /* ---- Warranty page (conditional) ---- */
  if (warranty_needed) {
    state.warranty_page_index = pager->GetSize();

    auto page = QuickGuidePageWidget::CreateCheckboxPage(
      look, disclaimer_text,
      _("I have read and understand the above disclaimer"),
      false,
      [&state, pager_ptr](bool checked) {
        state.warranty_accepted = checked;
        pager_ptr->UpdateNextButtonState();
        pager_ptr->SetCloseButtonCaption(checked
                                         ? _("Close")
                                         : _("Quit"));
      });
    state.warranty_widget = page.get();

    /* QuickGuidePageWidget already has its own VScrollWidget
       inside, so do NOT wrap it in another one (causes double
       scrolling and inflated virtual height).  Set the gesture
       callback directly instead. */
    page->SetGestureCallback(pager_gesture);

    pager->Add(std::move(page));
    titles.push_back(_("Safety Disclaimer"));
  }

  /* ---- What's New page (conditional, shown on version change) ---- */
  /* Inflate NEWS.txt at function scope so the text remains valid
     until after ShowModal() returns. */
  AllocatedString news_inflated;
  if (news_needed) {
    news_inflated = InflateToString(NEWS_txt_gz, NEWS_txt_gz_size);
    TruncateToCurrentVersion(news_inflated.data());
    const UTF8ToWideConverter news_text(news_inflated.c_str());
    if (news_text.IsValid()) {
      state.news_page_index = pager->GetSize();

      auto page = QuickGuidePageWidget::CreateCheckboxPage(
        look, news_text,
        _("Don't show these release notes again"),
        false,
        [](bool) { /* state is read on dialog close */ });
      page->SetParseLinks(false);
      page->SetGestureCallback(pager_gesture);

      pager->Add(std::move(page));
      titles.push_back(_("What's New"));
    }
  }

  /* ---- Cloud consent page (conditional, fly mode only) ---- */
#ifdef HAVE_SKYLINES_TRACKING
  if (cloud_needed) {
    state.cloud_page_index = pager->GetSize();

    const bool cloud_currently_enabled =
      CommonInterface::GetComputerSettings()
        .tracking.skylines.cloud.enabled == TriState::TRUE;

    auto page = QuickGuidePageWidget::CreateCheckboxPage(
      look, cloud_consent_text,
      _("Enable XCSoar Cloud"),
      cloud_currently_enabled,
      [](bool) { /* state is read on dialog close */ });
    page->SetGestureCallback(pager_gesture);

    pager->Add(std::move(page));
    titles.push_back(_("XCSoar Cloud"));
  }
#endif

  /* ---- Android permission disclosure pages (conditional) ---- */
#ifdef ANDROID
  /* Helper: advance to next page, or close dialog if on last page */
  auto advance_or_close = [pager_ptr, &dialog]() {
    if (!pager_ptr->Next(false))
      dialog.SetModalResult(mrOK);
  };

  /* Helper: skip permission and suppress the lazy permission flow's
     rationale dialogs so the user is not immediately re-prompted
     after choosing "Not Now". */
  auto skip_permissions = [advance_or_close]() {
    SuppressPermissionDialogs();
    advance_or_close();
  };

  if (!is_simulator && !AreLocationPermissionsGranted()) {
    state.location_page_index = pager->GetSize();

    auto page = QuickGuidePageWidget::CreateTwoButtonPage(
      look, location_disclosure_text,
      _("Continue"),
      [advance_or_close]() {
        /* Fire the permission request; advance only when the
           system dialog is dismissed and the result arrives */
        RequestLocationPermissions([advance_or_close](bool) {
          advance_or_close();
        });
      },
      _("Not Now"),
      skip_permissions);
    page->SetGestureCallback(pager_gesture);

    pager->Add(std::move(page));
    titles.push_back(_("Location Access"));
  }

  if (!is_simulator && !IsNotificationPermissionGranted()) {
    auto page = QuickGuidePageWidget::CreateTwoButtonPage(
      look, notification_disclosure_text,
      _("Continue"),
      [advance_or_close]() {
        RequestNotificationPermission([advance_or_close](bool) {
          advance_or_close();
        });
      },
      _("Not Now"),
      skip_permissions);
    page->SetGestureCallback(pager_gesture);

    pager->Add(std::move(page));
    titles.push_back(_("Notifications"));
  }
#endif

  /* ---- Informational pages (conditional) ---- */
  unsigned config_page_index = INVALID_PAGE;
  RichTextWidget *config_widget_ptr = nullptr;

  if (info_pages_needed) {
    // Gestures
    pager->Add(make_scroll_page(
      std::make_unique<RichTextWidget>(look, gesture_help_text)));
    titles.push_back(_("Gesture Navigation"));

    // Configuration (checkboxes reflect current profile state)
    auto config_widget =
      std::make_unique<RichTextWidget>(look, GetConfigurationHelpText());
    config_widget_ptr = config_widget.get();
    config_widget->SetLinkReturnCallback([config_widget_ptr]() {
      /* Refresh checkbox state after returning from a config dialog */
      config_widget_ptr->SetText(GetConfigurationHelpText());
    });
    pager->Add(make_scroll_page(std::move(config_widget)));
    config_page_index = pager->GetSize() - 1;
    titles.push_back(_("Getting Started"));

    // Preflight
    pager->Add(make_scroll_page(
      std::make_unique<RichTextWidget>(look, preflight_text)));
    titles.push_back(_("Preflight"));

    // Postflight
    pager->Add(make_scroll_page(
      std::make_unique<RichTextWidget>(look, postflight_text)));
    titles.push_back(_("After Your Flight"));

    // Don't show again - using a checkbox page
    {
      auto done_page = QuickGuidePageWidget::CreateCheckboxPage(
        look,
        _("# That's it!\n\n"
          "You can revisit the gesture help from the "
          "**Info** menu at any time.\n\n"
          "Check the box below to skip this guide on "
          "future startups."),
        _("Don't show this guide again"),
        IsQuickGuideHidden(),
        [](bool) { /* state is read on dialog close */ });
      done_page->SetGestureCallback(pager_gesture);
      pager->Add(std::move(done_page));
    }
    titles.push_back(_("Done"));
  }

  // If no pages were added, skip
  if (pager->GetSize() == 0)
    return true;

  /* ---- Page advance guard ---- */
  pager->SetCanAdvanceCallback(
    [&state](unsigned current_page) -> bool {
      if (current_page == state.warranty_page_index)
        return state.warranty_accepted;
      /* Location disclosure: disable Next arrow/key/swipe so the
         user must explicitly choose "Continue" or "Not Now".
         The buttons bypass CanAdvance() via PagerWidget::Next(). */
      if (current_page == state.location_page_index)
        return false;
      return true;
    });

  /* ---- Caption update on page flip ---- */
  const unsigned total_pages = pager->GetSize();

  auto update_caption =
    [&dialog, &titles, pager_ptr, total_pages,
     config_page_index, config_widget_ptr]() {
    const unsigned current = pager_ptr->GetCurrentIndex();

    /* Refresh the "Getting Started" checkboxes every time the
       page becomes visible, so changes made via xcsoar:// links
       are reflected immediately. */
    if (current == config_page_index && config_widget_ptr != nullptr)
      config_widget_ptr->SetText(GetConfigurationHelpText());

    StaticString<128> caption;
    if (current < titles.size())
      caption.Format("%s (%u/%u)",
                     titles[current],
                     current + 1, total_pages);
    else
      caption = _("Welcome to XCSoar");
    dialog.SetCaption(caption);
  };

  pager->SetPageFlippedCallback(update_caption);
  update_caption();

  dialog.FinishPreliminary(std::move(pager));

  /* Show "Quit" instead of "Close" until the disclaimer is accepted */
  if (warranty_needed)
    pager_ptr->SetCloseButtonCaption(_("Quit"));

  const int result = dialog.ShowModal();

  /* ---- Handle results ---- */

  /* If warranty page was shown and user closed without accepting,
     the close callback already confirmed the quit via message box */
  if (warranty_needed && !state.warranty_accepted)
    return false;

  // Save warranty acknowledgment
  if (warranty_needed && state.warranty_accepted) {
    Profile::Set(ProfileKeys::DisclaimerAcknowledgedVersion,
                 XCSoar_Version);
    Profile::Save();
  }

  /* Mark news as seen only if the user checked the checkbox */
  if (state.news_page_index != INVALID_PAGE) {
    auto &news_widget = static_cast<ArrowPagerWidget &>(
      dialog.GetWidget()).GetWidget(state.news_page_index);
    auto *news_page =
      dynamic_cast<QuickGuidePageWidget *>(&news_widget);
    if (news_page != nullptr && news_page->GetCheckboxState()) {
      Profile::Set(ProfileKeys::LastSeenNewsVersion, XCSoar_Version);
      Profile::Save();
    }
  }

  /* Save cloud consent based on checkbox state */
#ifdef HAVE_SKYLINES_TRACKING
  if (state.cloud_page_index != INVALID_PAGE) {
    auto &cloud_widget = static_cast<ArrowPagerWidget &>(
      dialog.GetWidget()).GetWidget(state.cloud_page_index);
    auto *cloud_page =
      dynamic_cast<QuickGuidePageWidget *>(&cloud_widget);
    if (cloud_page != nullptr) {
      if (cloud_page->GetCheckboxState())
        EnableCloud();
      else
        DisableCloud();
    }
  }
#endif

  // Check if user set "don't show again"
  if (info_pages_needed) {
    // The last page is the don't-show-again page
    const unsigned last_page_idx = total_pages - 1;
    auto &last_widget = static_cast<ArrowPagerWidget &>(
      dialog.GetWidget()).GetWidget(last_page_idx);
    auto *guide_page =
      dynamic_cast<QuickGuidePageWidget *>(&last_widget);
    if (guide_page != nullptr &&
        guide_page->GetCheckboxState()) {
      Profile::Set(ProfileKeys::HideQuickGuideDialogOnStartup, true);
      Profile::Save();
    }
  }

  (void)result;
  return true;
}
