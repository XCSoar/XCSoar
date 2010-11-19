#include "RenderTask.hpp"
#include "Task/Tasks/AbstractTask.hpp"
#include "Task/Tasks/GotoTask.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/Tasks/AbortTask.hpp"
#include "RenderTaskPoint.hpp"

RenderTask::RenderTask(RenderTaskPoint& _tpv)
  :tpv(_tpv)
{
}

void 
RenderTask::draw_layers(const AbstractTask& task) 
{
  for (unsigned i=0; i<4; i++) {
    tpv.set_layer((RenderTaskLayer)i);
    task.tp_CAccept(tpv);
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
