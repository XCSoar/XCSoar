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

#include "net/http/Session.hpp"
#include "net/http/Request.hpp"
#include "net/http/Handler.hpp"
#include "net/http/Init.hpp"
#include "system/ConvertPathName.hpp"
#include "util/PrintException.hxx"

#include <exception>
#include <iostream>
#include <stdio.h>

#include <tchar.h>

using namespace std;

class MyResponseHandler final : public Net::ResponseHandler {
  FILE *const file;

public:
  explicit MyResponseHandler(FILE *_file):file(_file) {}

  bool ResponseReceived(int64_t content_length) noexcept override {
    return true;
  }

  bool DataReceived(const void *data, size_t length) noexcept override {
    fwrite(data, 1, length, stdout);

    if (file != nullptr)
      fwrite(data, 1, length, file);

    return true;
  }
};

static void
Download(const char *url, Path path)
{
  cout << "Creating Session ... ";
  Net::Session session;
  cout << "done" << endl;

  cout << "Creating Request ... ";

  FILE *file = path != nullptr ? _tfopen(path.c_str(), _T("wb")) : nullptr;
  MyResponseHandler handler(file);
  Net::Request request(session, handler, url);
  cout << "done" << endl;

  request.Send();

  if (file != NULL)
    fclose(file);
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

  try {
    Net::Initialise();

    const char *url = argv[1];
    Download(url, argc > 2 ? (Path)PathName(argv[2]) : nullptr);

    Net::Deinitialise();
  } catch (const std::exception &exception) {
    PrintException(exception);
    return EXIT_FAILURE;
  }

  return 0;
}
