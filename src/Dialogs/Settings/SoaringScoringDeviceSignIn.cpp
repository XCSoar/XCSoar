// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SoaringScoringDeviceSignIn.hpp"

#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Language/Language.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "UIGlobals.hpp"
#include "Widget/RichTextWidget.hpp"
#include "co/Task.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Global.hxx"
#include "lib/curl/Handler.hxx"
#include "lib/curl/Request.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/curl/Slist.hxx"
#include "net/client/SoaringScoring/Account.hpp"
#include "net/client/SoaringScoring/Auth.hpp"
#include "net/client/SoaringScoring/Client.hpp"
#include "net/client/auth/DeviceFlow.hpp"
#include "net/http/Init.hpp"
#include "system/OpenLink.hpp"
#include "ui/event/Notify.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "util/SpanCast.hxx"

#include <chrono>
#include <ctime>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>

static uint32_t
Now() noexcept
{
  const auto now = std::time(nullptr);
  return now > 0 ? static_cast<uint32_t>(now) : 0;
}

static void
StoreToken(const Net::Client::Auth::DeviceFlow &flow) noexcept
{
  SoaringScoring::AccountToken account;
  account.access_token = flow.token.access_token;
  account.access_expires_at = flow.access_expires_at;
  SoaringScoring::SaveAccountToken(account);
}

/**
 * One device-code poll at a time while the code dialog is open.
 *
 * The transfer lives here, not in a coroutine frame. Closing the
 * dialog stops it before the form or the body string are destroyed.
 * status, body, and error are written on the network thread and read
 * on the UI thread from OnUi(), after the notify.
 *
 * shortcut: the code is text until the QR renderer from #3369 is on
 * this branch.
 */
struct SignInPoll final : CurlResponseHandler
{
  Net::Client::Auth::DeviceFlow flow;

  std::string form;
  CurlSlist headers;
  std::unique_ptr<CurlRequest> request;

  unsigned status = 0;
  std::string body;
  std::exception_ptr error;

  bool closed = false;
  bool signed_in = false;
  bool denied = false;
  bool expired = false;
  bool failed = false;

  WidgetDialog *dialog = nullptr;
  UI::Notify *notify = nullptr;
  UI::PeriodicTimer *timer = nullptr;

  void OnHeaders(unsigned _status, Curl::Headers &&) override
  {
    status = _status;
  }

  void OnData(std::span<const std::byte> data) override
  {
    body.append(ToStringView(data));
  }

  void OnEnd() override
  {
    if (notify != nullptr)
      notify->SendNotification();
  }

  void OnError(std::exception_ptr failure) noexcept override
  {
    error = std::move(failure);
    if (notify != nullptr)
      notify->SendNotification();
  }

  void Stop() noexcept
  {
    if (request == nullptr)
      return;
    request->StopIndirect();
    request.reset();
  }

  void Expire() noexcept
  {
    expired = true;
    if (timer != nullptr)
      timer->Cancel();
    if (dialog != nullptr)
      dialog->SetModalResult(mrCancel);
  }

  void Start() noexcept
  {
    if (closed || request != nullptr || flow.pending.device_code.empty())
      return;

    if (flow.Next(Now(), 0) ==
        Net::Client::Auth::DeviceFlowAction::RESTART) {
      Expire();
      return;
    }

    try {
      form = Net::Client::Auth::DevicePollForm(
        SoaringScoring::kOAuthClientId, flow.pending.device_code);

      CurlEasy easy{SoaringScoring::kTokenUrl.data()};
      Curl::Setup(easy);
      easy.SetTimeout(30);
      easy.SetPost();
      easy.SetRequestBody(form);

      headers.Clear();
      headers.Append("Content-Type: application/x-www-form-urlencoded");
      easy.SetRequestHeaders(headers.Get());

      status = 0;
      body.clear();
      error = nullptr;
      request = std::make_unique<CurlRequest>(*Net::curl, std::move(easy),
                                             *this);
      request->StartIndirect();
    } catch (...) {
      error = std::current_exception();
      request.reset();
      if (notify != nullptr)
        notify->SendNotification();
    }
  }

  void Schedule() noexcept
  {
    if (timer == nullptr)
      return;

    unsigned seconds = flow.wait_seconds;
    if (seconds == 0)
      seconds = flow.poll_interval_seconds;
    if (seconds == 0)
      seconds = 5;

    timer->Schedule(std::chrono::seconds{seconds});
  }

  void OnUi() noexcept
  {
    request.reset();

    if (closed || dialog == nullptr)
      return;

    if (error) {
      dialog->SetModalResult(mrCancel);
      return;
    }

    using Net::Client::Auth::DeviceFlowAction;
    switch (flow.OnPoll(status, body, Now())) {
    case DeviceFlowAction::READY:
      StoreToken(flow);
      signed_in = true;
      dialog->SetModalResult(mrOK);
      break;

    case DeviceFlowAction::DENIED:
      denied = true;
      dialog->SetModalResult(mrCancel);
      break;

    case DeviceFlowAction::RESTART:
      Expire();
      break;

    case DeviceFlowAction::AUTHORIZE:
      failed = true;
      dialog->SetModalResult(mrCancel);
      break;

    case DeviceFlowAction::WAIT:
    case DeviceFlowAction::RETRY:
      Schedule();
      break;

    case DeviceFlowAction::REFRESH:
      break;
    }
  }
};

bool
RunSoaringScoringDeviceSignIn() noexcept
{
  if (Net::curl == nullptr) {
    ShowMessageBox(_("Error"), _("SoaringScoring"), MB_OK);
    return false;
  }

  std::optional<std::pair<unsigned, std::string>> started;
  try {
    PluggableOperationEnvironment env;
    started = ShowCoFunctionDialog(
      UIGlobals::GetMainWindow(),
      UIGlobals::GetDialogLook(),
      _("Connecting..."),
      SoaringScoring::PostForm(
        *Net::curl, std::string{SoaringScoring::kDeviceAuthorizeUrl},
        SoaringScoring::AuthorizeForm()),
      &env);
  } catch (...) {
    ShowError(std::current_exception(), _("SoaringScoring"));
    return false;
  }

  if (!started)
    return false;

  SignInPoll poll;
  if (started->first != 200 ||
      !poll.flow.OnAuthorize(started->second, Now())) {
    const auto message = SoaringScoring::ErrorMessage(started->second);
    ShowMessageBox(message.empty() ? _("Error") : message.c_str(),
                   _("SoaringScoring"), MB_OK);
    return false;
  }

  const std::string url =
    poll.flow.pending.verification_uri_complete.empty()
      ? poll.flow.pending.verification_uri
      : poll.flow.pending.verification_uri_complete;

  std::string text = "# ";
  text += poll.flow.pending.user_code;
  text += "\n\n";
  text += url;

  const DialogLook &look = UIGlobals::GetDialogLook();
  auto body = std::make_unique<RichTextWidget>(look, text.c_str());
  WidgetDialog dialog(WidgetDialog::Full{},
                      UIGlobals::GetMainWindow(),
                      look,
                      _("SoaringScoring"),
                      body.release());
  dialog.AddButton(_("Open"), [&url]() { OpenLink(url.c_str()); });
  dialog.AddButton(_("Close"), mrCancel);
  poll.dialog = &dialog;

  UI::Notify notify{[&poll]() noexcept { poll.OnUi(); }};
  poll.notify = &notify;

  UI::PeriodicTimer timer{[&poll]() noexcept {
    if (poll.timer != nullptr)
      poll.timer->Cancel();
    poll.Start();
  }};
  poll.timer = &timer;

  poll.Schedule();
  dialog.ShowModal();

  poll.closed = true;
  timer.Cancel();
  poll.Stop();

  if (poll.signed_in) {
    ShowMessageBox(_("Connected"), _("SoaringScoring"), MB_OK);
    return true;
  }

  if (poll.error)
    ShowError(poll.error, _("SoaringScoring"));
  else if (poll.denied)
    ShowMessageBox(_("Sign-in was denied."), _("SoaringScoring"), MB_OK);
  else if (poll.expired)
    ShowMessageBox(C_("Status", "Expired"), _("SoaringScoring"), MB_OK);
  else if (poll.failed)
    ShowMessageBox(_("Error"), _("SoaringScoring"), MB_OK);

  return false;
}
