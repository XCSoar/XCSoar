// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Init.hpp"
#include "lib/curl/Global.hxx"

#include <curl/curl.h>

CurlGlobal *Net::curl;

void
Net::Initialise(EventLoop &event_loop)
{
  curl_global_init(CURL_GLOBAL_WIN32);

  curl = new CurlGlobal(event_loop);
}

void
Net::Deinitialise()
{
  delete curl;

  curl_global_cleanup();
}
