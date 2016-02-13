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

#include "../Request.hpp"
#include "../Session.hpp"
#include "Version.hpp"

Net::Request::Request(Session &_session, const TCHAR *url,
                      unsigned timeout_ms)
  :session(_session)
{
  // XXX implement timeout

  char user_agent[32];
  snprintf(user_agent, 32, "XCSoar/%s", XCSoar_Version);

  handle.SetUserAgent(user_agent);
  handle.SetFailOnError(true);
  handle.SetWriteFunction(WriteCallback, this);

  handle.SetURL(url);

  session.Add(handle.GetHandle());
}

Net::Request::~Request()
{
  session.Remove(handle.GetHandle());

  curl_slist_free_all(request_headers);
}

void
Net::Request::AddHeader(const char *name, const char *value)
{
  char buffer[4096];
  snprintf(buffer, sizeof(buffer), "%s: %s", name, value);
  request_headers = curl_slist_append(request_headers, buffer);
}

void
Net::Request::SetBasicAuth(const char *username, const char *password)
{
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%s:%s", username, password);
  handle.SetBasicAuth(buffer);
}

size_t
Net::Request::ResponseData(const uint8_t *ptr, size_t size)
{
  auto range = buffer.Write();
  if (range.size < size) {
    buffer.Shift();
    range = buffer.Write();
    if (range.size < size)
      /* buffer is full, pause CURL */
      return CURL_WRITEFUNC_PAUSE;
  }

  std::copy_n(ptr, size, range.data);
  buffer.Append(size);
  return size;
}

size_t
Net::Request::WriteCallback(char *ptr, size_t size, size_t nmemb,
                            void *userdata)
{
  Request *request = (Request *)userdata;
  return request->ResponseData((const uint8_t *)ptr, size * nmemb);
}

void
Net::Request::Send(unsigned _timeout_ms)
{
  if (request_headers != nullptr)
    handle.SetHeaders(request_headers);

  const int timeout_ms = _timeout_ms == INFINITE ? -1 : _timeout_ms;

  CURLMcode mcode = CURLM_CALL_MULTI_PERFORM;
  while (buffer.IsEmpty()) {
    CURLcode code = session.InfoRead(handle.GetHandle());
    if (code != CURLE_AGAIN) {
      if (code != CURLE_OK)
        throw std::runtime_error(curl_easy_strerror(code));
      return;
    }

    if (mcode != CURLM_CALL_MULTI_PERFORM)
      session.Select(timeout_ms);

    mcode = session.Perform();
    if (mcode != CURLM_OK && mcode != CURLM_CALL_MULTI_PERFORM)
      throw std::runtime_error(curl_multi_strerror(mcode));
  }
}

int64_t
Net::Request::GetLength() const
{
  return handle.GetContentLength();
}

size_t
Net::Request::Read(void *_buffer, size_t buffer_size, unsigned _timeout_ms)
{
  const int timeout_ms = _timeout_ms == INFINITE ? -1 : _timeout_ms;

  Buffer::Range range;
  CURLMcode mcode = CURLM_CALL_MULTI_PERFORM;
  while (true) {
    range = buffer.Read();
    if (!range.IsEmpty())
      break;

    CURLcode code = session.InfoRead(handle.GetHandle());
    if (code != CURLE_AGAIN) {
      if (code != CURLE_OK)
        throw std::runtime_error(curl_easy_strerror(code));
      return 0;
    }

    if (mcode != CURLM_CALL_MULTI_PERFORM)
      session.Select(timeout_ms);

    mcode = session.Perform();
    if (mcode != CURLM_OK && mcode != CURLM_CALL_MULTI_PERFORM)
      throw std::runtime_error(curl_multi_strerror(mcode));
  }

  --buffer_size;
  if (buffer_size > range.size)
    buffer_size = range.size;

  uint8_t *p = (uint8_t *)_buffer;
  std::copy_n(range.data, buffer_size, p);
  p[buffer_size] = 0;

  buffer.Consume(buffer_size);
  handle.Unpause();

  return buffer_size;
}
