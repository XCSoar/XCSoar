// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "net/client/auth/DeviceFlow.hpp"
#include "TestUtil.hpp"

using Net::Client::Auth::DeviceFlow;
using Net::Client::Auth::DeviceFlowAction;

static constexpr char kAuthorize[] =
  R"({"device_code":"dev","user_code":"KWVP-QLSW",)"
  R"("verification_uri":"https://api.weglide.org/v1/auth/device",)"
  R"("verification_uri_complete":"https://api.weglide.org/v1/auth/device?user_code=KWVP-QLSW",)"
  R"("expires_in":1800,"interval":5})";

static constexpr char kGranted[] =
  R"({"token_type":"Bearer","access_token":"access-1",)"
  R"("expires_in":864000,"refresh_token":"refresh-1",)"
  R"("scope":"upload declare copilot:layers"})";

static void
TestAuthorizeAndPoll()
{
  DeviceFlow flow;
  ok1(flow.Next(1'000, 60) == DeviceFlowAction::AUTHORIZE);
  ok1(!flow.OnAuthorize("{", 1'000));
  ok1(flow.OnAuthorize(kAuthorize, 1'000));
  ok1(flow.pending.user_code == "KWVP-QLSW");
  ok1(flow.pending.verification_uri_complete.find("KWVP-QLSW") !=
      std::string::npos);
  ok1(flow.device_expires_at == 1'000 + 1800);
  ok1(flow.poll_interval_seconds == 5);
  ok1(flow.Next(1'000, 60) == DeviceFlowAction::WAIT);

  ok1(flow.OnPoll(400, R"({"error":"authorization_pending"})", 1'005) ==
      DeviceFlowAction::WAIT);
  ok1(flow.wait_seconds == 5);
  ok1(flow.poll_interval_seconds == 5);

  ok1(flow.OnPoll(400,
                  R"({"detail":{"error":"authorization_pending"}})",
                  1'010) == DeviceFlowAction::WAIT);

  ok1(flow.OnPoll(400, R"({"error":"slow_down"})", 1'015) ==
      DeviceFlowAction::WAIT);
  ok1(flow.poll_interval_seconds == 10);
  ok1(flow.wait_seconds == 10);
  ok1(flow.OnPoll(400, R"({"error":"slow_down"})", 1'020) ==
      DeviceFlowAction::WAIT);
  ok1(flow.poll_interval_seconds == 15);

  ok1(flow.OnPoll(500, "not-json", 1'030) == DeviceFlowAction::RETRY);
  ok1(flow.pending.device_code == "dev");

  ok1(flow.OnPoll(200, kGranted, 2'000) == DeviceFlowAction::READY);
  ok1(flow.pending.device_code.empty());
  ok1(flow.token.access_token == "access-1");
  ok1(flow.token.refresh_token == "refresh-1");
  ok1(flow.access_expires_at == 2'000 + 864000);
  ok1(flow.Next(2'000, 60) == DeviceFlowAction::READY);
}

static void
TestDeniedAndExpired()
{
  DeviceFlow flow;
  ok1(flow.OnAuthorize(kAuthorize, 1'000));

  ok1(flow.OnPoll(400, R"({"error":"access_denied"})", 1'100) ==
      DeviceFlowAction::DENIED);
  ok1(flow.pending.device_code.empty());
  ok1(flow.Next(1'100, 60) == DeviceFlowAction::AUTHORIZE);

  ok1(flow.OnAuthorize(kAuthorize, 1'200));
  ok1(flow.OnPoll(400, R"({"error":"expired_token"})", 1'300) ==
      DeviceFlowAction::RESTART);
  ok1(flow.Next(1'300, 60) == DeviceFlowAction::AUTHORIZE);

  ok1(flow.OnAuthorize(kAuthorize, 1'000));
  ok1(flow.Next(1'000 + 1800, 60) == DeviceFlowAction::RESTART);
}

static void
TestRefresh()
{
  DeviceFlow flow;
  ok1(flow.OnAuthorize(kAuthorize, 1'000));
  ok1(flow.OnPoll(200, kGranted, 2'000) == DeviceFlowAction::READY);

  const uint32_t margin = 24u * 60u * 60u;
  const uint32_t inside = flow.access_expires_at - margin;
  ok1(flow.Next(inside, margin) == DeviceFlowAction::REFRESH);
  ok1(flow.Next(inside - 1, margin) == DeviceFlowAction::READY);

  ok1(flow.OnRefresh(200,
                     R"({"token_type":"Bearer","access_token":"access-2",)"
                     R"("expires_in":864000,"refresh_token":"refresh-2"})",
                     3'000) == DeviceFlowAction::READY);
  ok1(flow.token.access_token == "access-2");
  ok1(flow.token.refresh_token == "refresh-2");
  ok1(flow.access_expires_at == 3'000 + 864000);

  ok1(flow.OnRefresh(401, R"({"error":"invalid_grant"})", 4'000) ==
      DeviceFlowAction::AUTHORIZE);
  ok1(flow.token.refresh_token.empty());
  ok1(flow.token.access_token == "access-2");

  flow.token.refresh_token = "refresh-2";
  ok1(flow.OnRefresh(400, R"({"error":"invalid_request"})", 4'050) ==
      DeviceFlowAction::RETRY);
  ok1(flow.token.refresh_token == "refresh-2");

  flow.token.refresh_token = "refresh-2";
  ok1(flow.OnRefresh(500, R"({"error":"unavailable"})", 4'100) ==
      DeviceFlowAction::RETRY);
  ok1(flow.token.refresh_token == "refresh-2");

  ok1(flow.OnRefresh(200, R"({"access_token":"access-3","expires_in":10})",
                     5'000) == DeviceFlowAction::READY);
  ok1(flow.token.refresh_token.empty());
  ok1(flow.Next(5'000, 60) == DeviceFlowAction::READY);
  ok1(flow.Next(5'010, 60) == DeviceFlowAction::AUTHORIZE);
}

static void
TestForms()
{
  /* Keep '%' out of ok1() expressions: tap prints them with printf. */
  const std::string authorize = Net::Client::Auth::DeviceAuthorizeForm(
    "xcsoar", "upload declare copilot:layers");
  const std::string authorize_expected =
    "client_id=xcsoar&scope=upload+declare+copilot%3Alayers";
  ok1(authorize == authorize_expected);

  const std::string poll =
    Net::Client::Auth::DevicePollForm("xcsoar", "abc+def");
  const std::string poll_expected =
    "client_id=xcsoar&"
    "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code&"
    "device_code=abc%2Bdef";
  ok1(poll == poll_expected);

  const std::string refresh =
    Net::Client::Auth::RefreshForm("xcsoar", "refresh token");
  const std::string refresh_expected =
    "client_id=xcsoar&grant_type=refresh_token&"
    "refresh_token=refresh+token";
  ok1(refresh == refresh_expected);
}

int
main()
{
  plan_tests(25 + 9 + 19 + 3);
  TestAuthorizeAndPoll();
  TestDeniedAndExpired();
  TestRefresh();
  TestForms();
  return exit_status();
}
