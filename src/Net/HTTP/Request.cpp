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

#include "Request.hpp"
#include "Session.hpp"
#include "Handler.hpp"
#include "FormData.hpp"
#include "Version.hpp"
#include "Util/ConvertString.hpp"

Net::Request::Request(Session &_session, ResponseHandler &_handler,
                      const char *url)
  :session(_session),
   handle(url),
   handler(_handler)
{
  char user_agent[32];
  snprintf(user_agent, 32, "XCSoar/%s",
           (const char *)WideToUTF8Converter(XCSoar_Version));

  handle.SetUserAgent(user_agent);
  handle.SetFailOnError(true);
  handle.SetWriteFunction(WriteCallback, this);

  session.Add(handle.Get());
}

Net::Request::~Request()
{
  session.Remove(handle.Get());
}

void
Net::Request::AddHeader(const char *name, const char *value)
{
  assert(!submitted);

  char buffer[4096];
  snprintf(buffer, sizeof(buffer), "%s: %s", name, value);
  request_headers.Append(buffer);
}

void
Net::Request::SetBasicAuth(const char *username, const char *password)
{
  assert(!submitted);

  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%s:%s", username, password);
  handle.SetBasicAuth(buffer);
}

void
Net::Request::SetRequestBody(const MultiPartFormData &body)
{
  handle.SetHttpPost(body.Get());
}
void
Net::Request::SetNoFailOnError()
{
  handle.SetFailOnError(false);
}

void
Net::Request::SetVerifyPeer(bool value)
{
  handle.SetVerifyPeer(value);
}

void
Net::Request::SetRequestBody(const char *body, size_t size)
{
  handle.SetRequestBody(body, size);
}

size_t
Net::Request::ResponseData(const uint8_t *ptr, size_t size)
{
  if (!submitted)
    SubmitResponse();

  handler.DataReceived(ptr, size);
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
  if (request_headers.Get() != nullptr)
    handle.SetRequestHeaders(request_headers.Get());

  const int timeout_ms = _timeout_ms == INFINITE ? -1 : _timeout_ms;

  CURLMcode mcode = CURLM_CALL_MULTI_PERFORM;
  while (true) {
    CURLcode code = session.InfoRead(handle.Get());
    if (code != CURLE_AGAIN) {
      if (code != CURLE_OK)
        throw std::runtime_error(curl_easy_strerror(code));
      break;
    }

    if (mcode != CURLM_CALL_MULTI_PERFORM)
      session.Select(timeout_ms);

    mcode = session.Perform();
    if (mcode != CURLM_OK && mcode != CURLM_CALL_MULTI_PERFORM)
      throw std::runtime_error(curl_multi_strerror(mcode));
  }

  if (!submitted)
    SubmitResponse();
}

void
Net::Request::SubmitResponse()
{
  assert(!submitted);

  submitted = true;

  handler.ResponseReceived(handle.GetContentLength());
}
