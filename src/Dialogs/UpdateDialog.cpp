// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UpdateDialog.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Interface.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "MainWindow.hpp"
#include "Logger/Logger.hpp"
#include "Update/Service.hpp"
#include "UIGlobals.hpp"
#include "Widget/RichTextWidget.hpp"
#include "system/OpenLink.hpp"
#include "util/StaticString.hxx"

static bool
MayOfferUpdate() noexcept
{
  return !CommonInterface::Calculated().flight.flying &&
    (backend_components == nullptr || backend_components->igc_logger == nullptr ||
     !backend_components->igc_logger->IsLoggerActive());
}

static void
FormatUpdateText(StaticString<1024> &text,
                 const UpdateService &service) noexcept
{
  const UpdateInfo *const info = service.GetInfo();
  if (service.GetState() == UpdateState::CHECKING) {
    text = _("Checking for updates…");
  } else if (info != nullptr) {
    const char *const title = info->title.empty()
      ? _("A new XCSoar version is available.")
      : info->title.c_str();
    if (info->summary.empty())
      text = title;
    else
      text.Format("%s\n\n%s", title, info->summary.c_str());
  } else if (service.GetState() == UpdateState::UP_TO_DATE) {
    text = _("XCSoar is up to date.");
  } else if (service.GetState() == UpdateState::FAILED) {
    text = _("Unable to check for updates.");
  } else if (!service.HasBackend()) {
    text = _("Update checking is not available in this build.");
  } else {
    text = _("No update information is available yet.");
  }
}

static void
dlgUpdateShowModal(UI::SingleWindow &parent, UpdateService &service)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(WidgetDialog::Auto{}, parent, look, _("Software Update"));
  StaticString<1024> initial_text;
  FormatUpdateText(initial_text, service);
  auto content = std::make_unique<RichTextWidget>(look, initial_text.c_str(),
                                                  false);
  dialog.FinishPreliminary(std::move(content));

  Button *update_button = dialog.AddButton(_("Update"), [&service, &dialog] {
    const UpdateInfo *const info = service.GetInfo();
    if (info != nullptr && MayOfferUpdate()) {
      OpenLink(info->handoff_url.c_str());
      dialog.SetModalResult(mrOK);
    }
  });
  Button *dismiss_button = dialog.AddButton(_("Skip this version"),
                                             [&service, &dialog] {
    const UpdateInfo *const info = service.GetInfo();
    if (info != nullptr) {
      service.Dismiss(*info);
      dialog.SetModalResult(mrCancel);
    }
  });
  dialog.AddButton(_("Close"), mrCancel);
  const bool available = service.GetState() == UpdateState::AVAILABLE &&
    service.GetInfo() != nullptr;
  update_button->SetEnabled(available);
  dismiss_button->SetEnabled(available);
  dialog.ShowModal();
}

void
ShowAutomaticUpdateDialog(UpdateService &service) noexcept
{
  if (!service.ShouldNotifyAutomatically() ||
      service.GetState() != UpdateState::AVAILABLE ||
      !MayOfferUpdate() || CommonInterface::main_window == nullptr ||
      CommonInterface::main_window->HasDialog())
    return;

  const UpdateInfo *const info = service.GetInfo();
  if (info == nullptr)
    return;
  if (service.IsDismissed(*info)) {
    service.MarkAutomaticNotificationPresented();
    return;
  }

  service.MarkAutomaticNotificationPresented();
  try {
    dlgUpdateShowModal(*CommonInterface::main_window, service);
  } catch (...) {
  }
}
