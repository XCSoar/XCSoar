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

#include "LiveTrack24.hpp"
#include "Util/StringCompare.hxx"
#include "Util/ConvertString.hpp"
#include "Net/HTTP/Session.hpp"
#include "Net/HTTP/ToBuffer.hpp"
#include "Geo/GeoPoint.hpp"
#include "Util/StaticString.hxx"
#include "Version.hpp"

#include <assert.h>
#include <cstdlib>

namespace LiveTrack24
{
  NarrowString<256> server;

  static const char *GetServer();
  static bool SendRequest(const char *url, OperationEnvironment &env);
}

LiveTrack24::UserID
LiveTrack24::GetUserID(const TCHAR *username, const TCHAR *password,
                       OperationEnvironment &env)
{
  // http://www.livetrack24.com/client.php?op=login&user=<username>&pass=<pass>

  assert(username != NULL);
  assert(!StringIsEmpty(username));
  assert(password != NULL);
  assert(!StringIsEmpty(password));

  const WideToUTF8Converter username2(username);
  const WideToUTF8Converter password2(password);
  if (!username2.IsValid() || !password2.IsValid())
    return 0;

  NarrowString<1024> url;
  url.Format("http://%s/client.php?op=login&user=%s&pass=%s",
             GetServer(), (const char *)username2, (const char *)password);

  // Open download session
  Net::Session session;

  // Request the file
  char buffer[1024];
  size_t size = Net::DownloadToBuffer(session, url, buffer, sizeof(buffer) - 1,
                                      env);
  if (size == 0 || size == size_t(-1))
    return 0;

  buffer[size] = 0;

  char *p_end;
  UserID user_id = strtoul(buffer, &p_end, 10);
  if (buffer == p_end)
    return 0;

  return user_id;
}

LiveTrack24::SessionID
LiveTrack24::GenerateSessionID()
{
  int r = rand();
  return (r & 0x7F000000) | 0x80000000;
}

LiveTrack24::SessionID
LiveTrack24::GenerateSessionID(UserID user_id)
{
  return GenerateSessionID() | (user_id & 0x00ffffff);
}

bool
LiveTrack24::StartTracking(SessionID session, const TCHAR *username,
                           const TCHAR *password, unsigned tracking_interval,
                           VehicleType vtype, const TCHAR *vname,
                           OperationEnvironment &env)
{
  // http://www.livetrack24.com/track.php?leolive=2&sid=42664778&pid=1&
  //   client=YourProgramName&v=1&user=yourusername&pass=yourpass&
  //   phone=Nokia 2600c&gps=BT GPS&trk1=4&vtype=16388&
  //   vname=vehicle name and model

  const WideToUTF8Converter username2(username);
  const WideToUTF8Converter password2(password);
  const WideToUTF8Converter vname2(vname);
  if (!username2.IsValid() || !password2.IsValid() || !vname2.IsValid())
    return false;

#ifdef _UNICODE
  NarrowString<32> version;
  version.SetASCII(XCSoar_VersionLong);
#else
  const char *version = XCSoar_VersionLong;
#endif

  NarrowString<2048> url;
  url.Format("http://%s/track.php?leolive=2&sid=%u&pid=%u&"
             "client=%s&v=%s&user=%s&pass=%s&vtype=%u&vname=%s",
             GetServer(), session, 1, "XCSoar", version,
             (const char *)username2, (const char *)password, vtype, vname);

  return SendRequest(url, env);
}

bool
LiveTrack24::SendPosition(SessionID session, unsigned packet_id,
                          GeoPoint position, unsigned altitude,
                          unsigned ground_speed, Angle track,
                          int64_t timestamp_utc,
                          OperationEnvironment &env)
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
             (long long int)timestamp_utc);

  return SendRequest(url, env);
}

bool
LiveTrack24::EndTracking(SessionID session, unsigned packet_id,
                         OperationEnvironment &env)
{
  // http://www.livetrack24.com/track.php?leolive=3&sid=42664778&pid=453&prid=0

  NarrowString<1024> url;
  url.Format("http://%s/track.php?leolive=3&sid=%u&pid=%u&prid=0",
             GetServer(), session, packet_id);

  return SendRequest(url, env);
}

void
LiveTrack24::SetServer(const TCHAR * _server)
{
  server.SetASCII(_server);
}

const char *
LiveTrack24::GetServer()
{
  return server;
}

bool
LiveTrack24::SendRequest(const char *url, OperationEnvironment &env)
{
  // Open download session
  Net::Session session;

  // Request the file
  char buffer[64];
  size_t size = Net::DownloadToBuffer(session, url, buffer, sizeof(buffer),
                                      env);
  return size != size_t(-1) && size >= 2 &&
    buffer[0] == 'O' && buffer[1] == 'K';
}
