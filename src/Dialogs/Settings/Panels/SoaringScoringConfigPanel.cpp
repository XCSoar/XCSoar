// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SoaringScoringConfigPanel.hpp"
#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Language/Language.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "UIGlobals.hpp"
#include "Widget/RowFormWidget.hpp"
#include "net/http/Features.hpp"

#ifdef HAVE_HTTP
#include "Dialogs/Settings/SoaringScoringDeviceSignIn.hpp"
#include "Form/Button.hpp"
#include "net/client/SoaringScoring/Account.hpp"
#include "net/client/SoaringScoring/Client.hpp"
#include "net/http/Init.hpp"

#include <exception>
#endif

class SoaringScoringConfigPanel final : public RowFormWidget {
#ifdef HAVE_HTTP
  Button *account_button = nullptr;

  void UpdateButton() noexcept;
  void OnAccountClicked() noexcept;
#endif

public:
  SoaringScoringConfigPanel() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
};

#ifdef HAVE_HTTP

void
SoaringScoringConfigPanel::UpdateButton() noexcept
{
  if (account_button == nullptr)
    return;

  account_button->SetCaption(SoaringScoring::AccountConnected()
                              ? C_("Button", "Disconnect")
                              : C_("Button", "Connect"));
}

void
SoaringScoringConfigPanel::OnAccountClicked() noexcept
{
  if (SoaringScoring::AccountConnected()) {
    const auto token = SoaringScoring::LoadAccountToken().access_token;
    if (Net::curl != nullptr && !token.empty()) {
      try {
        PluggableOperationEnvironment env;
        [[maybe_unused]] const auto revoked = ShowCoFunctionDialog(
          UIGlobals::GetMainWindow(),
          UIGlobals::GetDialogLook(),
                             C_("Button", "Disconnect"),
          SoaringScoring::RevokeToken(*Net::curl, token),
          &env);
      } catch (...) {
        ShowError(std::current_exception(), _("SoaringScoring"));
      }
    }
    SoaringScoring::ClearAccountToken();
  } else {
    RunSoaringScoringDeviceSignIn();
  }

  UpdateButton();
}

#endif

void
SoaringScoringConfigPanel::Prepare(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

#ifdef HAVE_HTTP
  account_button = AddButton(C_("Button", "Connect"), [this]() {
    OnAccountClicked();
  });
  UpdateButton();
#else
  AddReadOnly(_("Not connected"));
#endif
}

void
SoaringScoringConfigPanel::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);
#ifdef HAVE_HTTP
  UpdateButton();
#endif
}

std::unique_ptr<Widget>
CreateSoaringScoringConfigPanel() noexcept
{
  return std::make_unique<SoaringScoringConfigPanel>();
}
