// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#ifndef WEATHER_SKYSIGHTAPI_HPP
#define WEATHER_SKYSIGHTAPI_HPP

#include "Request.hpp"
#include "APIGlue.hpp"
#include "APIQueue.hpp"
#include "Metrics.hpp"
#include "util/tstring.hpp"
#include <memory>
#include <map>
#include "system/Path.hpp"
#include <tchar.h>
#include "LocalPath.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

//maintain two-hour local data cache
#define SKYSIGHTAPI_LOCAL_CACHE 7200 

#define SKYSIGHTAPI_BASE_URL "https://skysight.io/api"

struct BrokenDateTime;

class SkysightAPI final {
  friend struct SkysightRequest;
  friend struct SkysightAsyncRequest;
  UI::PeriodicTimer timer{[this]{ OnTimer(); }};
  
public:
  tstring region;
  std::map<tstring, tstring> regions;
  std::vector<SkysightMetric> metrics;

  SkysightAPI(tstring email, tstring password, tstring _region,
	      SkysightCallback cb);
  ~SkysightAPI();
  
  bool IsInited();
  SkysightMetric GetMetric(int index);
  SkysightMetric GetMetric(const tstring id);
  SkysightMetric *GetMetric(const TCHAR *const id);
  bool MetricExists(const tstring id);
  int NumMetrics();

  bool GetImageAt(const TCHAR *const layer, BrokenDateTime fctime,
		  BrokenDateTime maxtime, SkysightCallback cb = nullptr);

  BrokenDateTime FromUnixTime(uint64_t t);
  static void GenerateLoginRequest();

protected:
  static SkysightAPI *self;
  bool inited_regions;
  bool inited_layers;
  bool inited_lastupdates;
  SkysightAPIQueue queue;
  const AllocatedPath cache_path;

  void LoadDefaultRegions();
  
  bool IsLoggedIn();
  void OnTimer();
  inline const tstring
  GetUrl(SkysightCallType type, const char *const layer = nullptr,
	 const uint64_t from = 0); 
  inline AllocatedPath
  GetPath(SkysightCallType type, const char *const layer = nullptr,
	  const uint64_t fctime = 0);

  bool GetResult(const SkysightRequestArgs &args, const tstring result,
		 boost::property_tree::ptree &output);
  bool CacheAvailable(Path path, SkysightCallType calltype,
		      const TCHAR *const layer = nullptr);

  static void ParseResponse(const tstring &&result, const bool success,
			    const SkysightRequestArgs &req);
  bool ParseRegions(const SkysightRequestArgs &args, const tstring &result);
  bool ParseLayers(const SkysightRequestArgs &args, const tstring &result);
  bool ParseLastUpdates(const SkysightRequestArgs &args,
			const tstring &result);
  bool ParseDataDetails(const SkysightRequestArgs &args,
			const tstring &result);
  bool ParseData(const SkysightRequestArgs &args, const tstring &result);
  bool ParseLogin(const SkysightRequestArgs &args, const tstring &result);

  static void MakeCallback(SkysightCallback cb, const tstring &&details,
			   const bool success, const tstring &&layer,
			   const uint64_t time_index);

  inline bool GetData(SkysightCallType t, SkysightCallback cb = nullptr,
		      bool force_recache = false) {
    return GetData(t,  nullptr,  0, 0, nullptr, cb, force_recache);
  }

  inline bool
  GetData(SkysightCallType t, const TCHAR *const layer, uint64_t from,
	  uint64_t to,
	  SkysightCallback cb = nullptr,  bool force_recache = false) {
    return GetData(t, layer,  from, to, nullptr, cb, force_recache);
  }

  bool
  GetData(SkysightCallType t, const TCHAR *const layer, const uint64_t from,
	  const uint64_t to, const TCHAR *const link,
	  SkysightCallback cb = nullptr, bool force_recache = false);

  bool Login(const SkysightCallback cb = nullptr);

};

#endif
