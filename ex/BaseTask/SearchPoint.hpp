#ifndef SEARCH_POINT_HPP
#define SEARCH_POINT_HPP

#include "TaskProjection.h"

class SearchPoint {
public:

  SearchPoint(const SearchPoint& sp):
    Location(sp.Location),
    flatLocation(sp.flatLocation),
    actual(sp.actual) {
  };

  SearchPoint(const GEOPOINT &loc, const TaskProjection& tp,
    bool _actual=false):
    Location(loc),
    flatLocation(tp.project(loc)),
    actual(_actual)
    {      
    };

  void setLocation(const GEOPOINT& loc, const TaskProjection& tp)
    {
      Location = loc;
      project(tp);
    }
  void project(const TaskProjection& tp);

  const GEOPOINT& getLocation() const {
    return Location;
  };

  bool equals(const SearchPoint& sp) const;
  bool sort(const SearchPoint& sp) const;

  unsigned flat_distance(const SearchPoint& sp) const;

private:
  GEOPOINT Location;
  FLAT_GEOPOINT flatLocation;
  bool actual;
//  double saved_rank;
};


#endif
