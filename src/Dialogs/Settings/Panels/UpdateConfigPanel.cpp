// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UpdateConfigPanel.hpp"
#include "Components.hpp"
#include "Dialogs/dlgQuickGuide.hpp"
#include "Form/Button.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Update/Service.hpp"
#include "UIGlobals.hpp"
#include "Version.hpp"
#include "Widget/RowFormWidget.hpp"
#include "system/OpenLink.hpp"
#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"

#include <functional>
#include <memory>
#include <utility>

enum UpdateConfigRow {
  CURRENT_VERSION,
  AVAILABLE_VERSION,
  LAST_SUCCESSFUL_CHECK,
  AUTOMATIC_CHECKS,
};

class UpdateConfigPanel final : public RowFormWidget {
  Button *check_button = nullptr;
  Button *download_button = nullptr;
  std::function<void()> previous_result_listener;

  void OnRefresh() noexcept;

public:
  UpdateConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
UpdateConfigPanel::OnRefresh() noexcept
{
  if (update_service == nullptr)
    return;

  StaticString<64> available;
  const UpdateInfo *const info = update_service->GetInfo();
  if (info != nullptr)
    available = info->version;
  else {
    switch (update_service->GetState()) {
    case UpdateState::CHECKING:
      available = _("Checking for updates…");
      break;

    case UpdateState::UP_TO_DATE:
      available = _("No update available");
      break;

    case UpdateState::FAILED:
      available = _("Unable to check for updates");
      break;

    default:
      available = _("No update information");
      break;
    }
  }
  SetText(AVAILABLE_VERSION, available.c_str());

  StaticString<32> last_check;
  const auto timestamp = update_service->GetLastSuccessfulCheck();
  if (timestamp) {
    const BrokenDateTime date_time =
      BrokenDateTime::FromUnixTime(*timestamp).ToLocal();
    last_check.Format("%04u-%02u-%02u %02u:%02u",
                      date_time.year, date_time.month, date_time.day,
                      date_time.hour, date_time.minute);
  } else
    last_check = _("Never");
  SetText(LAST_SUCCESSFUL_CHECK, last_check.c_str());

  check_button->SetEnabled(update_service->HasBackend() &&
                           update_service->GetState() != UpdateState::CHECKING);
  download_button->SetEnabled(info != nullptr);
}

void
UpdateConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  AddReadOnly(_("Current version"),
              _("The XCSoar version installed on this device."),
              XCSoar_Version);
  AddReadOnly(_("Available version"),
              _("The version offered by the most recent update check."),
              _("No update information"));
  AddReadOnly(_("Last successful check"),
              _("Time of the last successful online update check."),
              _("Never"));

  bool automatic_checks = true;
  Profile::Get(ProfileKeys::UpdateCheckEnabled, automatic_checks);
  AddBoolean(_("Automatically check for updates"),
             _("Check once per day for a new stable release."),
             automatic_checks);

  check_button = AddButton(_("Check now"), [this] {
    if (update_service != nullptr && update_service->StartCheck())
      OnRefresh();
  });
  download_button = AddButton(_("Open download page"), [] {
    if (update_service == nullptr)
      return;

    const UpdateInfo *const info = update_service->GetInfo();
    if (info != nullptr)
      OpenLink(info->handoff_url.c_str());
  });
  AddButton(_("Release notes"), [] {
    dlgQuickGuideShowReleaseNotes();
  });
}

void
UpdateConfigPanel::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);
  if (update_service != nullptr)
    try {
      previous_result_listener = update_service->SetResultListener(
        [this] { OnRefresh(); });
    } catch (...) {
    }
  OnRefresh();
}

void
UpdateConfigPanel::Hide() noexcept
{
  if (update_service != nullptr)
    update_service->SetResultListener(std::move(previous_result_listener));
  RowFormWidget::Hide();
}

bool
UpdateConfigPanel::Save(bool &changed) noexcept
{
  bool automatic_checks = true;
  Profile::Get(ProfileKeys::UpdateCheckEnabled, automatic_checks);
  if (SaveValue(AUTOMATIC_CHECKS, ProfileKeys::UpdateCheckEnabled,
                automatic_checks))
    changed = true;

  return true;
}

std::unique_ptr<Widget>
CreateUpdateConfigPanel()
{
  return std::make_unique<UpdateConfigPanel>();
}
