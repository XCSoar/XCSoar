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

#ifndef WEATHER_SKYSIGHT_HPP
#define WEATHER_SKYSIGHT_HPP

#include "Util/StaticString.hxx"
#include "OS/Path.hpp"
#include "LocalPath.hpp"
#include <map>
#include <vector>
#include <tchar.h>
#include "Operation/VerboseOperationEnvironment.hpp"
#include "Util/tstring.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Thread/StandbyThread.hpp"
#include "Event/Timer.hpp"

#include "Weather/Skysight/Metrics.hpp"
#include "Weather/Skysight/SkysightAPI.hpp"

#include "Blackboard/BlackboardListener.hpp"


#define SKYSIGHT_MAX_METRICS 5

struct BrokenDateTime;


struct SkysightImageFile {
public:
  SkysightImageFile(Path _filename);
  SkysightImageFile(Path _filename, Path _path);
  Path fullpath;
  Path filename;
  tstring metric;
  tstring region;
  uint64_t datetime;
  bool is_valid;
  uint64_t mtime;
};

class Skysight final : private NullBlackboardListener { //: public Timer {
  public:
    tstring region = "EUROPE";
    DisplayedMetric displayed_metric;
    
    static void DownloadComplete(const tstring &&details,  const bool success,  
                const tstring &&layer_id,  const uint64_t time_index);
    
    static void APIInited(const tstring &&details,  const bool success,  
                const tstring &&layer_id,  const uint64_t time_index);

    std::map<tstring, tstring> GetRegions() {
      return api.regions;
    }
    tstring GetRegion() {
      return api.region;
    }
    SkysightMetric GetMetric(int index) {
      return api.GetMetric(index);
    }
    SkysightMetric GetMetric(const tstring id) {
      return api.GetMetric(id);
    }
    bool MetricExists(const tstring id) {
      return api.MetricExists(id);
    }
    int NumMetrics() {
      return api.NumMetrics();
    }

    Skysight();
    ~Skysight();

    void Init();
    bool IsReady(bool force_update = false);

    void SaveActiveMetrics();
    void LoadActiveMetrics();

    void RemoveActiveMetric(int index);
    void RemoveActiveMetric(const tstring id);
    bool ActiveMetricsUpdating();
    bool GetActiveMetricState(tstring metric_name, SkysightActiveMetric &m);
    void SetActveMetricUpdateState(const tstring id, bool state = false);
    void RefreshActiveMetric(tstring id);
    SkysightActiveMetric GetActiveMetric(int index);
    SkysightActiveMetric GetActiveMetric(const tstring id);
    int NumActiveMetrics();
    bool ActiveMetricsFull();
    bool IsActiveMetric(const TCHAR *const id);
    int AddActiveMetric(const TCHAR *const id);
    bool DownloadActiveMetric(tstring id);
    bool DisplayActiveMetric(const TCHAR *const id = nullptr);


    static inline 
    AllocatedPath GetLocalPath() {
      return MakeLocalPath(_T("skysight"));
    }

    BrokenDateTime FromUnixTime(uint64_t t);
    BrokenDateTime GetNow(bool use_system_time = false);
    
    void Render(bool force_update = false);

  protected:
    SkysightAPI api;
    static Skysight *self;
   // void OnTimer() override;

  private:
    tstring email;
    tstring password;
    bool update_flag = false;
    BrokenDateTime curr_time;
    
  /* virtual methods from class BlackboardListener */
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated) override;    
    
    bool SetDisplayedMetric(const TCHAR *const id, BrokenDateTime forecast_time = BrokenDateTime());
    BrokenDateTime GetForecastTime(BrokenDateTime curr_time);
    std::vector<SkysightActiveMetric> active_metrics;

    std::vector<SkysightImageFile>  ScanFolder(tstring search_pattern);
    void CleanupFiles();

};


#endif
