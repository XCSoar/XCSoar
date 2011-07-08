/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Net/Request.hpp"
#include "Net/Session.hpp"
#include "Version.hpp"

#include <assert.h>

Net::Request::Request(Session &_session, const TCHAR *url,
                      unsigned long timeout)
  :session(_session), handle(curl_easy_init())
{
  // XXX implement timeout

  if (handle == NULL)
    return;

  char user_agent[32];
  snprintf(user_agent, 32, "XCSoar/%s", XCSoar_Version);

  curl_easy_setopt(handle, CURLOPT_USERAGENT, user_agent);
  curl_easy_setopt(handle, CURLOPT_FAILONERROR, true);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, this);

  CURLcode code = curl_easy_setopt(handle, CURLOPT_URL, url);
  if (code != CURLE_OK) {
    curl_easy_cleanup(handle);
    handle = NULL;
    return;
  }

  if (!session.Add(handle)) {
    curl_easy_cleanup(handle);
    handle = NULL;
    return;
  }
}

Net::Request::~Request()
{
  if (handle != NULL) {
    session.Remove(handle);
    curl_easy_cleanup(handle);
  }
}

size_t
Net::Request::ResponseData(const uint8_t *ptr, size_t size)
{
  Buffer::Range range = buffer.write();
  if ((size_t)range.second < size)
    /* buffer is full, pause CURL */
    return CURL_WRITEFUNC_PAUSE;

  std::copy(ptr, ptr + size, range.first);
  buffer.append(size);
  return size;
}

size_t
Net::Request::WriteCallback(void *ptr, size_t size, size_t nmemb,
                            void *userdata)
{
  Request *request = (Request *)userdata;
  return request->ResponseData((const uint8_t *)ptr, size * nmemb);
}

bool
Net::Request::Created() const
{
  return handle != NULL;
}

size_t
Net::Request::Read(void *_buffer, size_t buffer_size, unsigned long timeout)
{
  assert(handle != NULL);

  const int timeout_ms = timeout == INFINITE ? -1 : timeout;

  Buffer::Range range;
  CURLMcode mcode = CURLM_CALL_MULTI_PERFORM;
  while (true) {
    range = buffer.read();
    if (range.second > 0)
      break;

    CURLcode code = session.InfoRead(handle);
    if (code != CURLE_AGAIN)
      return 0;

    if (mcode != CURLM_CALL_MULTI_PERFORM &&
        !session.Select(timeout_ms))
      return 0;

    CURLMcode mcode = session.Perform();
    if (mcode != CURLM_OK && mcode != CURLM_CALL_MULTI_PERFORM)
      return 0;
  }

  --buffer_size;
  if (buffer_size > range.second)
    buffer_size = range.second;

  uint8_t *p = (uint8_t *)_buffer;
  std::copy(range.first, range.first + buffer_size, p);
  p[buffer_size] = 0;

  buffer.consume(buffer_size);
  curl_easy_pause(handle, CURLPAUSE_CONT);

  return buffer_size;
}
