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

#ifndef XCSOAR_SOCKET_OPTION_HPP
#define XCSOAR_SOCKET_OPTION_HPP

/**
 * Parameter type for boost::asio::basic_stream_socket::set_option()
 * which sets SO_SNDTIMEO.
 */
class SendTimeoutS {
#ifdef WIN32
  DWORD value;
#else
  struct timeval value;
#endif

public:
#ifdef WIN32
  explicit constexpr SendTimeoutS(unsigned _value_s):value(_value_s * 1000) {}
#else
  explicit constexpr SendTimeoutS(unsigned _value_s):value{time_t(_value_s), 0} {}
#endif

  template<class Protocol>
  int level(const Protocol &p) const {
    return SOL_SOCKET;
  }

  template<class Protocol>
  int name(const Protocol &p) const {
    return SO_SNDTIMEO;
  }

  template<class Protocol>
  const void *data(const Protocol &p) const {
    return &value;
  }

  template<class Protocol>
  size_t size(const Protocol& p) const {
    return sizeof(value);
  }
};

#endif
