#ifndef TASK_VARIO_HPP
#define TASK_VARIO_HPP

#include "Math/fixed.hpp"
#include "Util/Filter.hpp"
#include "Util/DiffFilter.hpp"
class GlideResult;

class TaskVario
{
public:
  TaskVario();

  double get_value() const;
  void update(const GlideResult& solution, const fixed dt);
  void reset(const GlideResult& solution);

private:
  double value;

  DiffFilter df;
  Filter v_lpf;
};

#endif
