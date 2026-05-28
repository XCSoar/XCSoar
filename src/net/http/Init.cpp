// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Init.hpp"
#include "lib/curl/Global.hxx"
#include "event/Call.hxx"
#include "event/Loop.hxx"

#include <curl/curl.h>

CurlGlobal *Net::curl;

namespace {

void
DrainCurl() noexcept
{
  CurlGlobal *const instance = Net::curl;
  if (instance == nullptr)
    return;

  EventLoop &loop = instance->GetEventLoop();
  if (loop.IsInside()) {
    delete instance;
    Net::curl = nullptr;
    return;
  }

  BlockingCall(loop, [instance]() noexcept {
    if (Net::curl == instance) {
      delete Net::curl;
      Net::curl = nullptr;
    }
  });
}

} // namespace

void
Net::Initialise(EventLoop &event_loop)
{
  curl_global_init(CURL_GLOBAL_WIN32);

  curl = new CurlGlobal(event_loop);
}

void
Net::Deinitialise() noexcept
{
  DrainCurl();
  curl_global_cleanup();
}
