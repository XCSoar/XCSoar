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

#include "LiveTrack24.hpp"

#include "Util/StringUtil.hpp"
#include "Net/Session.hpp"
#include "Net/Request.hpp"
#include "Net/ToBuffer.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "DateTime.hpp"
#include "Version.hpp"

#include <assert.h>
#include <cstdlib>

namespace LiveTrack24
{
  bool use_test_server = false;

  const TCHAR *GetServer();
  bool SendRequest(const TCHAR *url);
}

LiveTrack24::UserID
LiveTrack24::GetUserID(const TCHAR *username, const TCHAR *password)
{
  // http://www.livetrack24.com/client.php?op=login&user=<username>&pass=<pass>

  assert(username != NULL);
  assert(!string_is_empty(username));
  assert(password != NULL);
  assert(!string_is_empty(password));

  TCHAR url[1024];
  _sntprintf(url, 1024, _T("http://%s/client.php?op=login&user=%s&pass=%s"),
             GetServer(), username, password);

  // Open download session
  Net::Session session;
  if (session.Error())
    return 0;

  // Request the file
  char buffer[1024];
  Net::Request request(session, url, 3000);
  if (!request.Created())
    return false;

  unsigned size = request.Read(buffer, sizeof(buffer), 10000);
  if (size == 0)
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
                           VehicleType vtype, const TCHAR *vname)
{
  // http://www.livetrack24.com/track.php?leolive=2&sid=42664778&pid=1&
  //   client=YourProgramName&v=1&user=yourusername&pass=yourpass&
  //   phone=Nokia 2600c&gps=BT GPS&trk1=4&vtype=16388&
  //   vname=vehicle name and model


  TCHAR url[2048];
  _sntprintf(url, 2048, _T("http://%s/track.php?leolive=2&sid=%u&pid=%u&"
                           "client=%s&v=%s&user=%s&pass=%s&vtype=%u&vname=%s"),
             GetServer(), session, 1, _T("XCSoar"), XCSoar_VersionLong,
             username, password, vtype, vname);

  return SendRequest(url);
}

bool
LiveTrack24::SendPosition(SessionID session, unsigned packet_id,
                          GeoPoint position, unsigned altitude,
                          unsigned ground_speed, Angle track,
                          int64_t timestamp_utc)
{
  // http://www.livetrack24.com/track.php?leolive=4&sid=42664778&pid=321&
  //   lat=22.3&lon=40.2&alt=23&sog=40&cog=160&tm=1241422845

  TCHAR url[2048];
  _sntprintf(url, 2048, _T("http://%s/track.php?leolive=4&sid=%u&pid=%u&"
                           "lat=%f&lon=%f&alt=%d&sog=%d&cog=%d&tm=%lld"),
             GetServer(), session, packet_id,
             (double)position.latitude.Degrees(),
             (double)position.longitude.Degrees(),
             altitude, ground_speed,
             (unsigned)track.AsBearing().Degrees(),
             (long long int)timestamp_utc);

  return SendRequest(url);
}

bool
LiveTrack24::EndTracking(SessionID session, unsigned packet_id)
{
  // http://www.livetrack24.com/track.php?leolive=3&sid=42664778&pid=453&prid=0

  TCHAR url[1024];
  _sntprintf(url, 1024, _T("http://%s/track.php?leolive=3&sid=%u&pid=%u&prid=0"),
             GetServer(), session, packet_id);

  return SendRequest(url);
}

void
LiveTrack24::SetTestServer(bool _use_test_server)
{
  use_test_server = _use_test_server;
}

const TCHAR *
LiveTrack24::GetServer()
{
  return use_test_server ? _T("test.livetrack24.com") : _T("www.livetrack24.com");
}

bool
LiveTrack24::SendRequest(const TCHAR *url)
{
  // Open download session
  Net::Session session;
  if (session.Error())
    return false;

  // Request the file
  Net::Request request(session, url, 3000);
  if (!request.Created())
    return false;

  TCHAR buffer[16];
  unsigned size = request.Read(buffer, 16, 10000);
  return size >= 2 && buffer[0] == _T('O') && buffer[1] == _T('K');
}
