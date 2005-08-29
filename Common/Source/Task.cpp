#include "Task.h"
#include "XCSoar.h"
#include "externs.h"


void FlyDirectTo(int index) {
  ActiveWayPoint = -1; AATEnabled = FALSE;
  for(int j=0;j<MAXTASKPOINTS;j++)
  {
    Task[j].Index = -1;
  }
  Task[0].Index = index;
  ActiveWayPoint = 0;
}


// Inserts a waypoint into the task, in the
// position of the ActiveWaypoint
void InsertWaypoint(int index) {
  int i;
  
  if (ActiveWayPoint<0) {
    ActiveWayPoint = 0;
    Task[ActiveWayPoint].Index = index;
    return;
  }
  
  if (Task[MAXTASKPOINTS-1].Index != -1) {
    // No room for any more task points!
    MessageBox(hWndMapWindow,
      gettext(TEXT("Too many waypoints in task!")),
      gettext(TEXT("Insert Waypoint")),
      MB_OK|MB_ICONEXCLAMATION);
    
    return;
  }
  
  // Shuffle ActiveWaypoint and all later task points
  // to the right by one position
  for (i=MAXTASKPOINTS-1; i>ActiveWayPoint; i--) {
    Task[i].Index = Task[i-1].Index;
    
  }
  
  // Insert new point and update task details
  Task[ActiveWayPoint].Index = index;
  RefreshTaskWaypoint(ActiveWayPoint+1);
  RefreshTaskWaypoint(ActiveWayPoint);
  
  CalculateTaskSectors();
  CalculateAATTaskSectors();
}


// RemoveTaskpoint removes a single waypoint
// from the current task.  index specifies an entry
// in the Task[] array - NOT a waypoint index.
//
// If you call this function, you MUST deal with
// correctly setting ActiveWayPoint yourself!
void RemoveTaskPoint(int index) {
  
  int i;
  
  if (index < 0 || index >= MAXTASKPOINTS) {
    return; // index out of bounds
    
  }
  
  if (Task[index].Index == -1) {
    return; // There's no WP at this location
  }
  
  // Shuffle all later taskpoints to the left to
  // fill the gap
  for (i=index; i<MAXTASKPOINTS-1; ++i) {
    Task[i].Index = Task[i+1].Index;
  }
  Task[MAXTASKPOINTS-1].Index = -1;
  
  // Only need to refresh info where the removal happened
  // as the order of other taskpoints hasn't changed
  if (Task[index].Index != -1) {
    RefreshTaskWaypoint(index);
  }
  
  CalculateTaskSectors();
  CalculateAATTaskSectors();
  
}


// Index specifies a waypoint in the WP list
// It won't necessarily be a waypoint that's
// in the task
void RemoveWaypoint(int index) {
  int i;
  
  if (ActiveWayPoint<0) {
    return; // No waypoint to remove
  }
  
  // Check to see whether selected WP is actually
  // in the task list.
  // If not, we'll ask the user if they want to remove
  // the currently active task point.
  // If the WP is in the task multiple times then we'll
  // remove the first instance after (or including) the
  // active WP.
  // If they're all before the active WP then just remove
  // the nearest to the active WP
  
  // Search forward first
  i = ActiveWayPoint;
  while (i < MAXTASKPOINTS && Task[i].Index != index) {
    ++i;
  }
  
  if (i < MAXTASKPOINTS) {
    // Found WP, so remove it
    RemoveTaskPoint(i);
    
    if (Task[ActiveWayPoint].Index == -1) {
      // We've just removed the last task point and it was
      // active at the time
      ActiveWayPoint--;
    }
    
  } else {
    // Didn't find WP, so search backwards
    
    i = ActiveWayPoint;
    do {
      --i;
    } while (i >= 0 && Task[i].Index != index);
    
    if (i >= 0) {
      // Found WP, so remove it
      RemoveTaskPoint(i);
      ActiveWayPoint--;
      
    } else {
      // WP not found, so ask user if they want to
      // remove the active WP
      int ret = MessageBox(hWndMapWindow,
        gettext(TEXT("Chosen Waypoint not in current task.\nRemove active WayPoint?")),
        gettext(TEXT("Remove Waypoint")),
        MB_YESNO|MB_ICONQUESTION);
      
      if (ret == IDYES) {
        RemoveTaskPoint(ActiveWayPoint);
        if (Task[ActiveWayPoint].Index == -1) {
          // Active WayPoint was last in the list so is currently
          // invalid.
          ActiveWayPoint--;
        }
      }
    }
  }
}


void ReplaceWaypoint(int index) {
  
  // ARH 26/06/05 Fixed array out-of-bounds bug
  if (ActiveWayPoint>=0) {	
    
    Task[ActiveWayPoint].Index = index;
    RefreshTaskWaypoint(ActiveWayPoint);
    
    if (ActiveWayPoint>0) {
      RefreshTaskWaypoint(ActiveWayPoint-1);
    }
    
    if (ActiveWayPoint+1<MAXTASKPOINTS) {
      if (Task[ActiveWayPoint+1].Index != -1) {
        RefreshTaskWaypoint(ActiveWayPoint+1);
      }
    }
    
    CalculateTaskSectors();
    CalculateAATTaskSectors();
    
  } else {
    
    // Insert a new waypoint since there's
    // nothing to replace
    ActiveWayPoint=0;
    Task[ActiveWayPoint].Index = index;
    
  }
}

