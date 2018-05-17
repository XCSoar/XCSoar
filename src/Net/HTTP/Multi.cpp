/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Multi.hpp"

#include <assert.h>

Net::CurlMulti::CurlMulti()
  :multi(curl_multi_init())
{
}

Net::CurlMulti::~CurlMulti()
{
  assert(results.empty());

  if (multi != nullptr)
    curl_multi_cleanup(multi);
}

void
Net::CurlMulti::Remove(CURL *easy)
{
  auto i = results.find(easy);
  if (i != results.end())
    results.erase(i);

  curl_multi_remove_handle(multi, easy);
}

CURLcode
Net::CurlMulti::InfoRead(const CURL *easy)
{
  auto i = results.find(easy);
  if (i != results.end())
    return i->second;

  const CURLMsg *msg;
  int msgs_in_queue;
  while ((msg = curl_multi_info_read(multi, &msgs_in_queue)) != nullptr) {
    if (msg->msg == CURLMSG_DONE) {
      if (msg->easy_handle == easy)
        return msg->data.result;

      results.insert(std::make_pair(easy, msg->data.result));
    }
  }

  return CURLE_AGAIN;
}
