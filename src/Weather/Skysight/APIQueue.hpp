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
#ifndef WEATHER_SKYSIGHTAPI_QUEUE_HPP
#define WEATHER_SKYSIGHTAPI_QUEUE_HPP

#include "Request.hpp"
#include "CDFDecoder.hpp"
#include "Layers.hpp"
#include "Event/PeriodicTimer.hpp"
#include <deque>
#include <queue>


class SkysightAPIQueue final {
  std::deque<std::unique_ptr<SkysightAsyncRequest>> request_deque;
  std::queue<std::unique_ptr<CDFDecoder>> decode_queue;
  bool is_busy = false;
  PeriodicTimer timer{[this]{ Process(); }};
  
  void Process();
  
  tstring key;
  uint64_t key_expiry_time = 0;
  tstring email;
  tstring password;
  
public:
  SkysightAPIQueue(){};
  ~SkysightAPIQueue() {
    timer.Cancel();
  };
  
  void SetCredentials(const tstring _email, const tstring _pass);
  void SetKey(const tstring &&_key, const uint64_t _key_expiry_time);
  bool IsLoggedIn();
  void AddRequest(std::unique_ptr<SkysightAsyncRequest> request, bool append_end = true);
  void AddDecodeJob(std::unique_ptr<CDFDecoder> job);
  void Clear(const tstring &&msg);
  
};

#endif
