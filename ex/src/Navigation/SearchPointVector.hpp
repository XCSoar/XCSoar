#ifndef SEARCHPOINTVECTOR_HPP
#define SEARCHPOINTVECTOR_HPP

#include "SearchPoint.hpp"
#include <vector>

typedef std::vector<SearchPoint> SearchPointVector;

bool prune_interior(SearchPointVector& spv);
void project(SearchPointVector& spv, const TaskProjection& tp);

#endif
