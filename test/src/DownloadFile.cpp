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

#include "Net/Session.hpp"
#include "Net/Request.hpp"
#include "Net/Init.hpp"
#include "OS/PathName.hpp"

#include <iostream>
#include <stdio.h>
using namespace std;

static bool
Download(const TCHAR *url, const TCHAR *path)
{
  cout << "Creating Session ... ";
  Net::Session session;
  cout << (session.Error() ? "failed" : "done") << endl;
  if (session.Error())
    return false;

  cout << "Creating Request ... ";
  Net::Request request(session, url);
  cout << (!request.Created() ? "failed" : "done") << endl;
  if (!request.Created())
    return false;

  cout << "Reading Response:" << endl;
  cout << "-------------------------------------------------" << endl;

  FILE *file = path ? _tfopen(path, _T("wb")) : NULL;

  char buffer[256];
  while (request.Read(buffer, sizeof(buffer))) {
    cout << buffer;

    if (file != NULL)
      fputs(buffer, file);
  }

  if (file != NULL)
    fclose(file);

  cout << "-------------------------------------------------" << endl;

  return true;
}

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    cout << "Usage: " << argv[0] << " <url> [<filename>]" << endl;
    cout << "   <url> is the absolute url of the resource you are requesting" << endl << endl;
    cout << "   <filename> is the path where the requested file should be saved (optional)" << endl << endl;
    cout << "   Example: " << argv[0] << " http://www.domain.com/docs/readme.htm readme.htm" << endl;
    return 1;
  }

  Net::Initialise();

  PathName url(argv[1]);
  Download(url, argc > 2 ? (const TCHAR *)PathName(argv[2]) : NULL);

  Net::Deinitialise();

  return 0;
}
