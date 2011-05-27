/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Net/Context.hpp"
#include "Net/Session.hpp"
#include "Net/Connection.hpp"
#include "Net/Request.hpp"

#include <iostream>
using namespace std;

static bool
TestConnection(const char *server, const char *url)
{
  cout << "Creating Session ... ";
  Net::Session session;
  cout << (session.Error() ? "failed" : "done") << endl;
  if (session.Error())
    return false;

  cout << "Creating Connection ... ";
  Net::Connection connection(session, server);
  cout << (!connection.Connected() ? "failed" : "done") << endl;
  if (!connection.Connected())
    return false;

  cout << "Creating Request ... ";
  Net::Request request(connection, url);
  cout << (!request.Created() ? "failed" : "done") << endl;
  if (!request.Created())
    return false;

  cout << "Sending Request ... ";
  if (!request.Send()) {
    cout << "failed" << endl;
    return false;
  }
  cout << "done" << endl;

  cout << "Reading Response:" << endl;
  cout << "-------------------------------------------------" << endl;

  char buffer[256];
  while (request.Read(buffer, sizeof(buffer)))
    cout << buffer;

  cout << "-------------------------------------------------" << endl;

  return true;
}

int
main(int argc, char *argv[])
{
  if (argc != 3) {
    cout << "Usage: " << argv[0] << " <server> <url>" << endl;
    cout << "   <server> is the hostname of the http server" << endl;
    cout << "   <url> is the url of the object you are requesting (without the hostname)" << endl << endl;
    cout << "   Example: " << argv[0] << " www.domain.com /docs/readme.htm" << endl;
    return 1;
  }

  TestConnection(argv[1], argv[2]);

  return 0;
}
