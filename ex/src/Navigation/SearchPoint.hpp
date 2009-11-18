#ifndef SEARCH_POINT_HPP
#define SEARCH_POINT_HPP

#include "GeoPoint.hpp"
#include "Navigation/Flat/FlatGeoPoint.hpp"

class TaskProjection;

class SearchPoint {
public:

  SearchPoint(const SearchPoint& sp):
    Location(sp.Location),
    flatLocation(sp.flatLocation),
    actual(sp.actual) {
  };

  SearchPoint(const GEOPOINT &loc, const TaskProjection& tp,
    bool _actual=false);

  void project(const TaskProjection& tp);

  const GEOPOINT& getLocation() const {
    return Location;
  };

  bool equals(const SearchPoint& sp) const;
  bool sort(const SearchPoint& sp) const;

  unsigned flat_distance(const SearchPoint& sp) const;

  const FLAT_GEOPOINT& get_flatLocation() const {
    return flatLocation;
  };

private:
  GEOPOINT Location;
  FLAT_GEOPOINT flatLocation;
  bool actual;
//  double saved_rank;
};


#endif
