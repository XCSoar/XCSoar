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
#include "Profile/Profile.hpp"
#include "util/StaticString.hxx"

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
  enum Controls {
    AutomaticUpload = 1,
  };

  Button *account_button = nullptr;
  bool automatic_upload = true;
  /** Help text; %s is the untranslated service name. */
  StaticString<512> automatic_upload_help;

  void UpdateButton() noexcept;
  void OnAccountClicked() noexcept;
#endif

public:
  SoaringScoringConfigPanel() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
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
  SetRowEnabled(AutomaticUpload, SoaringScoring::AccountConnected());
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
        ShowError(std::current_exception(), "SoaringScoring");
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
  Profile::Get(ProfileKeys::SoaringScoringAutomaticUpload,
               automatic_upload);
  automatic_upload_help.Format(
      _("Asks whether to upload flight to %s, after flight is "
        "downloaded from external logger."),
      "SoaringScoring");
  account_button = AddButton(C_("Button", "Connect"), [this]() {
    OnAccountClicked();
  });
  AddBoolean(_("Automatic Upload"), automatic_upload_help.c_str(),
             automatic_upload);
  UpdateButton();
#else
  AddReadOnly(_("Not connected"));
#endif
}

bool
SoaringScoringConfigPanel::Save(bool &changed) noexcept
{
#ifdef HAVE_HTTP
  changed |= SaveValue(AutomaticUpload,
                       ProfileKeys::SoaringScoringAutomaticUpload,
                       automatic_upload);
#else
  (void)changed;
#endif
  return true;
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
