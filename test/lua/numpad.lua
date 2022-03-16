panDelta=3
zoomFactor = 1.41

function alternates()
 	xcsoar.task.alternates() 	
end

function cursorUp()
  if xcsoar.map.is_panning then
   xcsoar.map.pancursor( 0, panDelta)
  else
 	xcsoar.map.zoom(-zoomFactor) -- zoom out 	
  end
end  

function cursorDown()
  if xcsoar.map.is_panning then
   xcsoar.map.pancursor( 0,-panDelta)
  else
 	xcsoar.map.zoom(zoomFactor) -- zoom in 	
  end
end  

function cursorLeft()
  if xcsoar.map.is_panning then
   xcsoar.map.pancursor( panDelta,0)
  else
   xcsoar.map.prev()
  end
end 

function cursorRight()
  if xcsoar.map.is_panning then
   xcsoar.map.pancursor( -panDelta,0)
  else
   xcsoar.map.next()
  end
end  

function togglePan()
  if xcsoar.map.is_panning then
  	xcsoar.map.leavepan()
  else
  	xcsoar.map.enterpan()
  end
end

function next()
	xcsoar.map.next()
end
function prev()
	xcsoar.map.prev()
end


-- declare keyboard functions for lua
xcsoar.input_event.new("key_KP2", cursorDown )
xcsoar.input_event.new("key_KP_DOWN", cursorDown )
xcsoar.input_event.new("key_KP8", cursorUp )
xcsoar.input_event.new("key_KP_UP", cursorUp )

xcsoar.input_event.new("key_KP4", cursorLeft )
xcsoar.input_event.new("key_KP_LEFT", cursorLeft )
xcsoar.input_event.new("key_KP6", cursorRight )
xcsoar.input_event.new("key_KP_RIGHT", cursorRight )

xcsoar.input_event.new("key_KP9", xcsoar.task.nextWaypoint )
xcsoar.input_event.new("key_KP_PAGE_DOWN", xcsoar.task.nextWaypoint )
xcsoar.input_event.new("key_KP3", xcsoar.task.previousWaypoint )
xcsoar.input_event.new("key_KP_PAGE_UP", xcsoar.task.previousWaypoint )

xcsoar.input_event.new("key_KP7", xcsoar.map.show )
xcsoar.input_event.new("key_KP_HOME", xcsoar.map.show )


xcsoar.input_event.new("key_NUMLOCK", togglePan )

xcsoar.input_event.new("key_KPSLASH", xcsoar.task.alternates )
xcsoar.input_event.new("key_KPMINUS", xcsoar.misc.analysis )
xcsoar.input_event.new("key_KPPLUS", xcsoar.waypoint.showCurrent )
xcsoar.input_event.new("key_KP5", xcsoar.waypoint.showSelected )
xcsoar.input_event.new("key_KP_BEGIN", xcsoar.waypoint.showSelected )


