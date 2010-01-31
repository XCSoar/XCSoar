#include "RenderTask.hpp"
#include "Task/Tasks/AbstractTask.hpp"
#include "Task/Tasks/GotoTask.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/Tasks/AbortTask.hpp"

RenderTask::RenderTask(MapDrawHelper &_helper,
                       bool draw_bearing,
                       const NMEA_INFO& state,
                       MapWindow& map):
  MapDrawHelper(_helper),
  tpv(*this, draw_bearing, state, map)
{
}

void 
RenderTask::draw_layers(const AbstractTask& task) 
{
  for (unsigned i=0; i<4; i++) {
    tpv.set_layer(i);
    task.Accept(tpv);
  }
}

void 
RenderTask::Visit(const AbortTask& task) 
{
  tpv.set_active_index(task.getActiveIndex());
  draw_layers(task);
}

void 
RenderTask::Visit(const OrderedTask& task) 
{
  tpv.set_active_index(task.getActiveIndex());
  draw_layers(task);
}

void 
RenderTask::Visit(const GotoTask& task) 
{
  tpv.set_active_index(0);
  draw_layers(task);
}
