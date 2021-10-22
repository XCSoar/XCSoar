/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Client.hpp"
#include "Operation/Operation.hpp"
#include "util/StringCompare.hxx"
#include "util/ConvertString.hpp"
#include "net/http/CoRequest.hxx"
#include "net/http/Setup.hxx"
#include "Geo/GeoPoint.hpp"
#include "co/Task.hxx"
#include "util/RuntimeError.hxx"
#include "util/StringView.hxx"
#include "Version.hpp"

#include <cassert>
#include <cstdlib>
#include <stdexcept>

namespace LiveTrack24 {

Co::Task<UserID>
Client::GetUserID(const TCHAR *username, const TCHAR *password)
{
  // http://www.livetrack24.com/client.php?op=login&user=<username>&pass=<pass>

  assert(username != NULL);
  assert(!StringIsEmpty(username));
  assert(password != NULL);
  assert(!StringIsEmpty(password));

  // Request the file
  CurlEasy easy;

  {
    const WideToUTF8Converter username2(username);
    const WideToUTF8Converter password2(password);
    if (!username2.IsValid() || !password2.IsValid())
      throw std::runtime_error("WideToUTF8Converter failed");

    NarrowString<1024> url;
    url.Format("http://%s/client.php?op=login&user=%s&pass=%s",
               GetServer(),
               easy.Escape(username2).c_str(),
               easy.Escape(password2).c_str());
    easy.SetURL(url);
  }

  Curl::Setup(easy);
  easy.SetFailOnError();

  const auto response = co_await Curl::CoRequest(curl, std::move(easy));
  const char *body = response.body.c_str();

  char *p_end;
  UserID user_id = strtoul(body, &p_end, 10);
  if (p_end == body || user_id == 0)
    throw std::runtime_error("Login failed");

  co_return user_id;
}

Co::Task<void>
Client::StartTracking(SessionID session, const TCHAR *username,
                      const TCHAR *password, unsigned tracking_interval,
                      VehicleType vtype, const TCHAR *vname)
{
  // http://www.livetrack24.com/track.php?leolive=2&sid=42664778&pid=1&
  //   client=YourProgramName&v=1&user=yourusername&pass=yourpass&
  //   phone=Nokia 2600c&gps=BT GPS&trk1=4&vtype=16388&
  //   vname=vehicle name and model

  CurlEasy easy;

  {
    const WideToUTF8Converter username2(username);
    const WideToUTF8Converter password2(password);
    const WideToUTF8Converter vname2(vname);
    if (!username2.IsValid() || !password2.IsValid() || !vname2.IsValid())
      throw std::runtime_error("WideToUTF8Converter failed");

#ifdef _UNICODE
    NarrowString<32> version;
    version.SetASCII(XCSoar_VersionLong);
#else
    const char *version = XCSoar_VersionLong;
#endif

    NarrowString<2048> url;
    url.Format("http://%s/track.php?leolive=2&sid=%u&pid=%u&"
               "client=%s&v=%s&user=%s&pass=%s&vtype=%u&vname=%s",
               GetServer(), session, 1,
               "XCSoar", easy.Escape(version).c_str(),
               easy.Escape(username2).c_str(),
               easy.Escape(password2).c_str(),
               vtype,
               easy.Escape(vname2).c_str());

    easy.SetURL(url);
  }

  co_return co_await SendRequest(std::move(easy));
}

Co::Task<void>
Client::SendPosition(SessionID session, unsigned packet_id,
                     GeoPoint position, unsigned altitude,
                     unsigned ground_speed, Angle track,
                     std::chrono::system_clock::time_point timestamp_utc)
{
  // http://www.livetrack24.com/track.php?leolive=4&sid=42664778&pid=321&
  //   lat=22.3&lon=40.2&alt=23&sog=40&cog=160&tm=1241422845

  NarrowString<2048> url;
  url.Format("http://%s/track.php?leolive=4&sid=%u&pid=%u&"
             "lat=%f&lon=%f&alt=%d&sog=%d&cog=%d&tm=%lld",
             GetServer(), session, packet_id,
             (double)position.latitude.Degrees(),
             (double)position.longitude.Degrees(),
             altitude, ground_speed,
             (unsigned)track.AsBearing().Degrees(),
             (long long)std::chrono::system_clock::to_time_t(timestamp_utc));

  co_return co_await SendRequest(url);
}

Co::Task<void>
Client::EndTracking(SessionID session, unsigned packet_id)
{
  // http://www.livetrack24.com/track.php?leolive=3&sid=42664778&pid=453&prid=0

  NarrowString<1024> url;
  url.Format("http://%s/track.php?leolive=3&sid=%u&pid=%u&prid=0",
             GetServer(), session, packet_id);

  co_return co_await SendRequest(url);
}

void
Client::SetServer(const TCHAR * _server) noexcept
{
  server.SetASCII(_server);
}

Co::Task<void>
Client::SendRequest(CurlEasy easy)
{
  Curl::Setup(easy);
  easy.SetFailOnError();

  const auto _response = co_await Curl::CoRequest(curl, std::move(easy));
  StringView response{std::string_view{_response.body}};
  if (response.StartsWith("OK"))
    co_return;

  if (response.SkipPrefix("NOK : ") && !response.empty())
    throw FormatRuntimeError("Error from server: %.*s",
                             int(response.size), response.data);

  throw std::runtime_error("Error from server");
}

Co::Task<void>
Client::SendRequest(const char *url)
{
  return SendRequest(CurlEasy{url});
}

} // namespace LiveTrack24
