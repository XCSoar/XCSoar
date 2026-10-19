// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WeGlideDeviceSignIn.hpp"

#include "Dialogs/CoFunctionDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Language/Language.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "UIGlobals.hpp"
#include "net/client/WeGlide/Account.hpp"
#include "Widget/RichTextWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "co/Task.hxx"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Global.hxx"
#include "lib/curl/Handler.hxx"
#include "lib/curl/Request.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/curl/Slist.hxx"
#include "net/client/WeGlide/Auth.hpp"
#include "net/http/Init.hpp"
#include "system/OpenLink.hpp"
#include "ui/event/Notify.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "util/SpanCast.hxx"

#ifdef HAVE_ZXING
#include "QR/QRCode.hpp"
#include "Renderer/QRCodeRenderer.hpp"
#include "Resources.hpp"
#include "Screen/Layout.hpp"
#include "Widget/DrawWidget.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Icon.hpp"
#endif

#include <chrono>
#include <ctime>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#ifdef HAVE_ZXING

/** QR pane. A non-zero minimum lets TwoWidgets give it the spare space. */
class SignInCodeWidget final : public DrawWidget {
public:
  using DrawWidget::DrawWidget;

  PixelSize GetMinimumSize() const noexcept override {
    const unsigned side = Layout::GetMinimumControlHeight() * 4;
    return {side, side};
  }
};

static const MaskedIcon *
WeGlideMark() noexcept
{
  static MaskedIcon icon;
  static bool ready = false;
  if (!ready) {
    ready = true;
    try {
      icon.LoadResource(IDB_WEGLIDE_ALL);
    } catch (...) {
    }
  }
  return icon.IsDefined() ? &icon : nullptr;
}

static std::unique_ptr<Widget>
MakeCodeWidget(const QRSymbol &symbol)
{
  return std::make_unique<SignInCodeWidget>(
    [&symbol](Canvas &canvas, const PixelRect &rc) {
      canvas.Clear(COLOR_WHITE);
      if (symbol.IsValid())
        DrawQRSymbol(canvas, rc, symbol, WeGlideMark());
    });
}

#endif

static uint32_t
Now() noexcept
{
  const auto now = std::time(nullptr);
  return now > 0 ? static_cast<uint32_t>(now) : 0;
}

/**
 * POST a form. The body and the Content-Type list stay in this
 * frame until the transfer finishes: curl does not copy them.
 */
static Co::Task<std::pair<unsigned, std::string>>
CoPostForm(CurlGlobal &curl, std::string url, std::string body)
{
  CurlEasy easy{url.c_str()};
  Curl::Setup(easy);
  easy.SetTimeout(30);
  easy.SetPost();
  easy.SetRequestBody(body);

  CurlSlist headers;
  headers.Append("Content-Type: application/x-www-form-urlencoded");
  easy.SetRequestHeaders(headers.Get());

  auto response = co_await Curl::CoRequest(curl, std::move(easy));
  co_return std::pair{response.status, std::move(response.body)};
}

static bool sign_in_open = false;

bool
IsWeGlideSignInOpen() noexcept
{
  return sign_in_open;
}

bool
WeGlideAccountConnected() noexcept
{
  return !WeGlide::LoadAccountToken().access_token.empty();
}

void
ForgetWeGlideAccount() noexcept
{
  WeGlide::SaveAccountToken({});
}

static void
StoreToken(const Net::Client::Auth::DeviceFlow &flow) noexcept
{
  WeGlide::AccountToken account;
  account.access_token = flow.token.access_token;
  account.refresh_token = flow.token.refresh_token;
  account.access_expires_at = flow.access_expires_at;
  WeGlide::SaveAccountToken(account);
}

/**
 * One device-code poll at a time while the QR dialog is open.
 *
 * The transfer lives here, not in a coroutine frame. Closing the
 * dialog stops it before form, headers, or the body string are
 * destroyed. status, body, and error are written on the network
 * thread and read on the UI thread from OnUi(), after the notify.
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

  bool closed    = false;
  bool signed_in = false;
  bool denied    = false;
  bool failed    = false;

  WidgetDialog *dialog     = nullptr;
  UI::Notify *notify       = nullptr;
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
    if (notify != nullptr) notify->SendNotification();
  }

  void OnError(std::exception_ptr failure) noexcept override
  {
    error = std::move(failure);
    if (notify != nullptr) notify->SendNotification();
  }

  void Stop() noexcept
  {
    if (request == nullptr) return;
    request->StopIndirect();
    request.reset();
  }

  void Start() noexcept
  {
    if (closed || request != nullptr ||
        flow.pending.device_code.empty())
      return;

    try {
      form = Net::Client::Auth::DevicePollForm(
        WeGlide::kOAuthClientId, flow.pending.device_code);

      const std::string url{WeGlide::kTokenUrl};
      CurlEasy easy{url.c_str()};
      Curl::Setup(easy);
      easy.SetTimeout(30);
      easy.SetPost();
      easy.SetRequestBody(form);

      headers.Clear();
      headers.Append(
        "Content-Type: application/x-www-form-urlencoded");
      easy.SetRequestHeaders(headers.Get());

      status = 0;
      body.clear();
      error = nullptr;
      request = std::make_unique<CurlRequest>(*Net::curl,
                                             std::move(easy), *this);
      request->StartIndirect();
    } catch (...) {
      error = std::current_exception();
      request.reset();
      if (notify != nullptr) notify->SendNotification();
    }
  }

  void Schedule() noexcept
  {
    if (timer == nullptr) return;

    unsigned seconds = flow.wait_seconds;
    if (seconds == 0) seconds = flow.poll_interval_seconds;
    if (seconds == 0) seconds = 5;

    timer->Schedule(std::chrono::seconds{seconds});
  }

  void OnUi() noexcept
  {
    /* Done() has already stopped the transfer. */
    request.reset();

    if (closed || dialog == nullptr) return;

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
RunWeGlideDeviceSignIn() noexcept
{
  if (Net::curl == nullptr) {
    ShowMessageBox(_("Failed"), _("WeGlide"), MB_OK);
    return false;
  }

  sign_in_open = true;
  struct ClearSignIn {
    ~ClearSignIn() noexcept { sign_in_open = false; }
  };
  [[maybe_unused]] ClearSignIn clear_sign_in;

  std::optional<std::pair<unsigned, std::string>> started;
  try {
    PluggableOperationEnvironment env;
    started = ShowCoFunctionDialog(
      UIGlobals::GetMainWindow(),
      UIGlobals::GetDialogLook(),
      _("Connecting..."),
      CoPostForm(*Net::curl,
                 std::string{WeGlide::kDeviceAuthorizeUrl},
                 WeGlide::AuthorizeForm()),
      &env);
  } catch (...) {
    ShowError(std::current_exception(), _("WeGlide"));
    return false;
  }

  if (!started) return false;

  SignInPoll poll;
  if (started->first != 200 ||
      !poll.flow.OnAuthorize(started->second, Now())) {
    ShowMessageBox(_("Failed"), _("WeGlide"), MB_OK);
    return false;
  }

  const std::string url = poll.flow.pending.verification_uri_complete.empty()
                            ? poll.flow.pending.verification_uri
                            : poll.flow.pending.verification_uri_complete;

  std::string text = poll.flow.pending.user_code;
  text += "\n\n";
  text += url;

  const DialogLook &look = UIGlobals::GetDialogLook();
  std::unique_ptr<Widget> body =
    std::make_unique<RichTextWidget>(look, text.c_str());

#ifdef HAVE_ZXING
  QRSymbol symbol;
  try {
    symbol = EncodeQRCode(url);
  } catch (...) {
    symbol = {};
  }
  if (symbol.IsValid())
    body =
      std::make_unique<TwoWidgets>(MakeCodeWidget(symbol), std::move(body));
#endif

  WidgetDialog dialog(WidgetDialog::Full{},
                      UIGlobals::GetMainWindow(),
                      look,
                      _("WeGlide"),
                      body.release());
  dialog.AddButton(_("Open"), [&url]() { OpenLink(url.c_str()); });
  dialog.AddButton(_("Close"), mrCancel);
  poll.dialog = &dialog;

  UI::Notify notify{[&poll]() noexcept { poll.OnUi(); }};
  poll.notify = &notify;

  UI::PeriodicTimer timer{[&poll]() noexcept {
    if (poll.timer != nullptr) poll.timer->Cancel();
    poll.Start();
  }};
  poll.timer = &timer;

  poll.Schedule();
  dialog.ShowModal();

  poll.closed = true;
  timer.Cancel();
  poll.Stop();

  if (poll.signed_in) {
    ShowMessageBox(_("Connected"), _("WeGlide"), MB_OK);
    return true;
  }

  if (poll.error)
    ShowError(poll.error, _("WeGlide"));
  else if (poll.denied)
    ShowMessageBox(_("Cancelled"), _("WeGlide"), MB_OK);
  else if (poll.failed)
    ShowMessageBox(_("Failed"), _("WeGlide"), MB_OK);

  return false;
}
