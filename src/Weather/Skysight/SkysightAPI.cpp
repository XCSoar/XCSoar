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

#include "CDFDecoder.hpp"
#include "SkysightAPI.hpp"
#include "Request.hpp"
#include "SkysightRegions.hpp"

#include "Event/Timer.hpp"

#include "Util/StaticString.hxx"

#include <memory>
#include <vector>
#include <map>

#include "OS/Path.hpp"
#include "LocalPath.hpp"
#include "OS/FileUtil.hpp"

#include "Net/HTTP/Session.hpp"
#include "Net/HTTP/Request.hpp"
#include "Net/HTTP/Handler.hpp"

#include "IO/FileLineReader.hpp"

#include "Time/BrokenDateTime.hpp"
#include "Metrics.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#include "IO/BufferedReader.hxx"

#include "Operation/Operation.hpp"


SkysightAPI *SkysightAPI::self;

SkysightAPI::~SkysightAPI() {
  Timer::Cancel();
}

SkysightMetric SkysightAPI::GetMetric(int index) {

  assert(index < (int)metrics.size());
  auto &i = metrics.at(index);
  
  return i;

}

SkysightMetric SkysightAPI::GetMetric(const tstring id) {
  
  std::vector<SkysightMetric>::iterator i;
  for(i = metrics.begin(); i<metrics.end();++i) {
    if(!i->id.compare(id)) {
      assert(i < metrics.end());
      return (*i);
    }
  }

  return (*i);
}

//TODO: Use auto ptr, use TCHAR for all, try to whittle down to pointer only ver

SkysightMetric *SkysightAPI::GetMetric(const TCHAR *const id) {
  
 bool metric_exists = false;
  std::vector<SkysightMetric>::iterator i;
  for(i = metrics.begin(); i<metrics.end();++i)
    if(!i->id.compare(id)) {
      metric_exists = true;
      break;
    }
  assert(metric_exists);
  
  return &(*i);
}

bool SkysightAPI::MetricExists(const tstring id) {

  std::vector<SkysightMetric>::iterator i;
  for(i = metrics.begin(); i<metrics.end();++i)
    if(!i->id.compare(id)) {
      return true;
    }
  return false;
}


int SkysightAPI::NumMetrics() {
  return (int)metrics.size(); 
}

const tstring SkysightAPI::GetUrl(SkysightCallType type, const char *const layer, const uint64_t from) {
  NarrowString<256> url;
  switch(type) {
    case SkysightCallType::Regions:
      url = SKYSIGHTAPI_BASE_URL"/regions";
      break;
    case SkysightCallType::Layers:
      url.Format(SKYSIGHTAPI_BASE_URL "/layers?region_id=%s", region.c_str());
      break;
    case SkysightCallType::LastUpdates:
      url.Format(SKYSIGHTAPI_BASE_URL "/data/last_updated?region_id=%s", region.c_str());
      break;    
    case SkysightCallType::DataDetails:
      url.Format(SKYSIGHTAPI_BASE_URL "/data?region_id=%s&layer_ids=%s&from_time=%llu", region.c_str(), layer, from);
      break;    
    case SkysightCallType::Data:
    case SkysightCallType::Image:
    //caller should already have URL
      break;    
    case SkysightCallType::Login:
      url = SKYSIGHTAPI_BASE_URL"/auth";
      break;
  }
  return url.c_str();
}

AllocatedPath SkysightAPI::GetPath(SkysightCallType type, const char *const layer,
                                   const uint64_t fctime) {
  NarrowString<256> filename;
  BrokenDateTime fc;
  switch(type) {
    case SkysightCallType::Regions:
      filename = "regions.json";
      break;
    case SkysightCallType::Layers:
      filename.Format("layers-%s.json", region.c_str());
      break;
    case SkysightCallType::LastUpdates:
      filename.Format("lastupdated-%s.json", region.c_str());
      break;    
    case SkysightCallType::DataDetails:
      fc = FromUnixTime(fctime);
      filename.Format("%s-datafiles-%s-%04d%02d%02d%02d%02d.json", region.c_str(), layer, 
          fc.year, fc.month, fc.day, fc.hour, fc.minute);
      break;    
    case SkysightCallType::Data:
      fc = FromUnixTime(fctime);
      filename.Format("%s-%s-%04d%02d%02d%02d%02d.nc", region.c_str(), layer, 
          fc.year, fc.month, fc.day, fc.hour, fc.minute);
      break;  
    case SkysightCallType::Image:
      return GetPath(SkysightCallType::Data, layer, fctime).WithExtension(".tif");
      break;
    case SkysightCallType::Login:
      // local path should not be used
      filename = "credentials.json";
      break;
  }
  return AllocatedPath::Build(cache_path, filename);
}


BrokenDateTime SkysightAPI::FromUnixTime(uint64_t t) {
#ifdef HAVE_POSIX
  return BrokenDateTime::FromUnixTimeUTC(t);
#else
  // Only use for skysight-provided dates. 
  // (We can rely on their epoch being consistent)
 return BrokenDateTime(1970, 1, 1, 0, 0, 0) + (int)t;
#endif
}



void SkysightAPI::Init(tstring email, tstring password, tstring _region, SkysightCallback cb) {

  inited_regions = false;
  inited_layers = false;
  inited_lastupdates = false;

  region = (_region.empty()) ? "EUROPE" : _region;
  LoadDefaultRegions();
  
  if(email.empty() || password.empty())
    return;
  
  queue.SetCredentials(email.c_str(), password.c_str());
  
  
  GetData(SkysightCallType::Regions, cb);
  
  // Check for maintenance actions every 15 mins
  Timer::Schedule(std::chrono::milliseconds(900000));
   
}

bool SkysightAPI::IsInited() {
  return inited_regions && inited_layers && inited_lastupdates;
}



void
SkysightAPI::ParseResponse(const tstring &&result, const bool success, const SkysightRequestArgs req) {

  if(!self) return;

  if(!success) {
    if(req.calltype == SkysightCallType::Login) {
      self->queue.Clear(_T("Login error"));
    } else {
      self->MakeCallback(req.cb, result.c_str() , false, req.layer.c_str(), req.from);
    }
    return;
  }

  switch(req.calltype) {
    case SkysightCallType::Regions:
      self->ParseRegions(req, result);
      break;
    case SkysightCallType::Layers:
      self->ParseLayers(req, result);
      break;
    case SkysightCallType::LastUpdates:
      self->ParseLastUpdates(req, result);
      break;
    case SkysightCallType::DataDetails:
      self->ParseDataDetails(req, result);
      break;
    case SkysightCallType::Data:
      self->ParseData(req, result);
      break;
    case SkysightCallType::Image:
      break;
    case SkysightCallType::Login:
      self->ParseLogin(req, result);
    default:
      break;
  }

}


bool SkysightAPI::ParseRegions(const SkysightRequestArgs &args, const tstring &result) {

  boost::property_tree::ptree details;
  
  if(!GetResult(args, result.c_str(), details)) {
    LoadDefaultRegions();
    return false;
  }
  
  regions.clear();

  bool success = false;
  
  for(auto &i : details) {
    boost::property_tree::ptree &node = i.second;
    auto id = node.find("id");
    auto name = node.find("name");
    if(id != node.not_found()) {
      regions.insert(std::pair<tstring, tstring>(id->second.data(), name->second.data()));
      success = true;
    }
  }

  if(success) {
    inited_regions = true;
  } else { //fall back to defaults
    LoadDefaultRegions();
    return false;
  }

  //region loaded from settings is not in our regions list. Fall back to Europe.
  if(regions.find(region) == regions.end()) {
    region = "EUROPE";
    return false;
  }

  if(success) {
    if(!inited_layers)
      GetData(SkysightCallType::Layers, args.cb);
    else
       MakeCallback(args.cb, result.c_str(), success, _T(""), 0);

    inited_regions = true;
    return true;
  }
  
  MakeCallback(args.cb, _T(""), success, _T(""), 0);
  return false; 
}

void SkysightAPI::LoadDefaultRegions() {
  for(auto r = skysight_region_defaults; r->id != nullptr;++r)
    regions.insert(std::pair<tstring, tstring>(r->id, r->name));
  
  if(regions.find(region) == regions.end()) {
    region = "EUROPE";
  }
}

bool SkysightAPI::ParseLayers(const SkysightRequestArgs &args, const tstring &result) {
  
  boost::property_tree::ptree details;

  if(!GetResult(args, result.c_str(), details)) {
    MakeCallback(args.cb, _T(""), false, _T(""), 0);
    return false;
  }

  metrics.clear();
  bool success = false;

  for(auto &i : details) {
    boost::property_tree::ptree &node = i.second;
    auto id = node.find("id");
    auto legend = node.find("legend");
    if(id != node.not_found() && legend != node.not_found()) {
      SkysightMetric m = SkysightMetric(
        tstring(id->second.data()), 
        tstring(node.find("name")->second.data()),
        tstring(node.find("description")->second.data())
      );

      auto colours = legend->second.find("colors");
      if(colours != legend->second.not_found()) {
        success = true;
        for(auto &j : colours->second) {
          auto c = j.second.get_child("color").begin();
          m.legend.insert(std::pair<float, LegendColor>(
            std::stof(j.second.find("value")->second.data()),
            {
              static_cast<unsigned char>(std::stoi(c->second.data())),
              static_cast<unsigned char>(std::stoi(std::next(c, 1)->second.data())),
              static_cast<unsigned char>(std::stoi(std::next(c, 2)->second.data()))
            }
          ));
        }
        metrics.push_back(m);
      }
    }
  }


  if(success) {
    if(!inited_lastupdates) 
      GetData(SkysightCallType::LastUpdates, args.cb);
    else
      MakeCallback(args.cb, result.c_str(), success, _T(""), 0);

    inited_layers = true;
    return true;
  }

  MakeCallback(args.cb, _T(""), false, _T(""), 0);

  return false;
}

bool SkysightAPI::ParseLastUpdates(const SkysightRequestArgs &args, const tstring &result) {

  boost::property_tree::ptree details;
  if(!GetResult(args, result.c_str(), details)) {
    MakeCallback(args.cb, _T(""), false, _T(""), 0);
    return false;
  }

  bool success = false;
  for(auto &i : metrics) {
    for(auto &j : details) {
      auto id = j.second.find("layer_id");
      auto time = j.second.find("time");
      if((id != j.second.not_found()) && (time != j.second.not_found())
                                                && (i.id.compare(id->second.data()) == 0)) {
        i.last_update = std::strtoull(time->second.data().c_str(), NULL, 0);
        success = true;
      }
    }
  }

  inited_lastupdates = success;
  MakeCallback(args.cb, _T(""), success, _T(""), 0);

  return success;
}

bool SkysightAPI::ParseDataDetails(const SkysightRequestArgs &args, const tstring &result) {

  boost::property_tree::ptree details;
  if(!GetResult(args, result.c_str(), details)) {
    MakeCallback(args.cb, _T(""), false, args.layer.c_str(), args.from);
    return false;
  }

  bool success = false;
  uint64_t time_index;

  for(auto &j : details) {
    auto time = j.second.find("time");
    auto link = j.second.find("link");
    if((time != j.second.not_found()) && (link != j.second.not_found())) {
      time_index = static_cast<uint64_t>(std::strtoull(time->second.data().c_str(), NULL, 0));

      if(time_index > args.to) {
        if(!success)
          MakeCallback(args.cb, _T("") , false, args.layer.c_str(), args.from);

        return success;
      }

      success = GetData(SkysightCallType::Data, args.layer.c_str(), time_index, args.to,
                  link->second.data().c_str(), args.cb);

      if(!success)
        return false;
    }
  }

  return success;
}

bool SkysightAPI::ParseLogin(const SkysightRequestArgs &args, const tstring &result) {

  boost::property_tree::ptree details;
  if(!GetResult(args, result.c_str(), details)) {
    queue.Clear(_T("Login error"));
    return false;
  }

  bool success = false;
  auto key = details.find("key");
  auto valid_until = details.find("valid_until");

  if((key != details.not_found()) && (valid_until != details.not_found())) {
    queue.SetKey(key->second.data().c_str(), static_cast<uint64_t>(std::strtoull(valid_until->second.data().c_str(), NULL, 0)));
    success = true;

    //TODO: trim available regions from allowed_regions
  } else {
    queue.Clear(_T("Login error"));
  }
  return success;
}



bool SkysightAPI::ParseData(const SkysightRequestArgs &args, const tstring &result) {

  auto output_img = GetPath(SkysightCallType::Image, args.layer.c_str(), args.from);
  
  queue.AddDecodeJob(std::make_unique<CDFDecoder>(args.path.c_str(), output_img.c_str(),
                                          args.layer.c_str(), args.from, 
                                          GetMetric(args.layer.c_str())->legend, args.cb));
  
  return true;
}






bool SkysightAPI::GetData(SkysightCallType t, const TCHAR *const layer,
                          const uint64_t from, const uint64_t to, const TCHAR *const link,
                          SkysightCallback cb, bool force_recache) {

  const tstring &&url = link ? link : GetUrl(t, layer,  from);

  const auto path = GetPath(t,  layer,  from);

  bool to_file = !(t == SkysightCallType::DataDetails || t == SkysightCallType::Login || t == SkysightCallType::LastUpdates);
    
  SkysightRequestArgs ra(
    url.c_str(),
    path.c_str(),
    to_file,
    t,
    region.c_str(),
    layer ? layer : "",
    from,
    to,
    cb
  );

  // If cache is available, parse it directly regardless of whether async or sync
  if(!force_recache && CacheAvailable(path, t)) {
    ParseResponse(path.c_str(), true, ra);
    return true;
  }

  queue.AddRequest(std::make_unique<SkysightAsyncRequest>(ra), (t != SkysightCallType::Login));

  return true;
}

bool SkysightAPI::LoadFromFile(Path path, tstring &content) {
  FileLineReader reader(path, Charset::AUTO);
  content = "";
  char *line;
  while ((line = reader.ReadLine()) != NULL) {
    content += line;
  }

  return true;
}


bool SkysightAPI::CacheAvailable(Path path, SkysightCallType calltype, const TCHAR *const layer) {

  uint64_t layer_updated = 0;
  if(layer) {
    SkysightMetric *m = GetMetric(layer);
    layer_updated = m->last_update;
  }
  
  if(File::Exists(path)) {
    switch(calltype) {
      case SkysightCallType::Regions:
      case SkysightCallType::Layers:
        // cached for as long as we hae the files to allow fast startup
        return true;
        break;
      case SkysightCallType::LastUpdates:
        // always retrieve last updaes if requested
        return false;
        break;
      case SkysightCallType::Image:
        if(!layer)
          return false;
        
        return (layer_updated <= File::GetLastModification(path));
        break;
      case SkysightCallType::DataDetails:
      case SkysightCallType::Data:
      case SkysightCallType::Login:
        // these aren't cached to disk
        return false;
        break;
      default:
        return false;
        break;
    }
  }

  return false;

}

bool SkysightAPI::GetResult(const SkysightRequestArgs &args, const tstring result, boost::property_tree::ptree &output) {

  try {
    if(args.to_file) {
      boost::property_tree::read_json(result.c_str(), output);
    } else {
      std::stringstream result_stream(result);
      boost::property_tree::read_json(result_stream, output);
    }
  } catch(const std::exception &exc) {
    return false;
  }
  return true;
}

bool SkysightAPI::GetImageAt(const TCHAR *const layer, BrokenDateTime fctime,
                             BrokenDateTime maxtime,
                             SkysightCallback cb) {
  
  // round time to nearest 30-min forecast slot
  if((fctime.minute >= 15) && (fctime.minute < 45)) fctime.minute = 30;
  else if(fctime.minute >= 45) { 
    fctime.minute = 0;
    fctime = fctime + (60*60);
  }
  else if(fctime.minute < 15) fctime.minute = 0;

  uint64_t time_index = fctime.ToUnixTimeUTC();
  uint64_t max_index = maxtime.ToUnixTimeUTC();
  uint64_t search_index = time_index;
  
  bool found_image = true;
  while(found_image && (search_index <= max_index)) {
    auto path = GetPath(SkysightCallType::Image, layer, search_index);
    found_image = CacheAvailable(path, SkysightCallType::Image, layer);

    if(found_image) {
      search_index += (60*30);
      
      if(search_index > max_index) {
         MakeCallback(cb, path.c_str(), true, layer, time_index);
        return true;
      }
    }
  }

  return GetData(SkysightCallType::DataDetails, layer, time_index, max_index, cb);

}

void SkysightAPI::GenerateLoginRequest() {
  if(!self)
    return;

  self->GetData(SkysightCallType::Login);
}

void SkysightAPI::MakeCallback(SkysightCallback cb, const tstring &&details, const bool success, 
                   const tstring &&layer, const uint64_t time_index) {
  if(cb)
    cb(details.c_str(), success, layer.c_str(), time_index);
  
}

void SkysightAPI::OnTimer() {
  // various maintenance acions

  uint64_t now = (uint64_t)BrokenDateTime::NowUTC().ToUnixTimeUTC();

  //refresh regions cache file if > 24h old
  auto p = GetPath(SkysightCallType::Regions);
  if(File::Exists(p) && ((File::GetLastModification(p) + 86400) < now))
    GetData(SkysightCallType::Regions, nullptr, true);

  //refresh layers cache file if > 24h old
  p = GetPath(SkysightCallType::Layers);
  if(File::Exists(p) && ((File::GetLastModification(p) + 86400) < now))
    GetData(SkysightCallType::Layers, nullptr, true);
  
  //refresh last update times if > 5h (update freq is usually 5 hours)
  for(auto &m : metrics) {
    if((m.last_update + 18000) < now) {
      GetData(SkysightCallType::LastUpdates);
      break;
    }
  }
  
}
