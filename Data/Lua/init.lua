require("gestures");
require("gce");

-- Custom ---------------------------------------------------------------------

xcsoar.replay.start("/home/jmw/.xcsoar/logs/2016-04-03-XFL-A4N-01.igc");
ttarget = 17900.0;
dt = -1;
record = false;


xcsoar.timer.new(10,
                 function(t)
                    -- check for replay to have started
                    if (xcsoar.replay.virtual_time > 0) then
                       -- on first call, set fast forward time
                       if (dt<0) then
                          dt = ttarget - xcsoar.replay.virtual_time;
                          print("Fast forward ", dt)
                          xcsoar.replay.fast_forward(dt);
                       -- detect end condition
                       elseif (xcsoar.replay.virtual_time > ttarget + 60) then
                          print("Stop replay")
                          record = false;
                          xcsoar.replay.stop();
                          t:cancel()
                       -- detect reaching target
                       elseif (xcsoar.replay.virtual_time >= ttarget) then
                          if (not record) then
                             record = true;
                             print("Reached target time")
                          end
                       end
                    end
                 end
)
