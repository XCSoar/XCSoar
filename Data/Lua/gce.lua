-- Glide computer events-------------------------------------------------------

xcsoar.input_event.new("gce_takeoff",
                                function(e)
                                   xcsoar.fire_legacy_event("AutoLogger","start");
                                   xcsoar.fire_legacy_event("AddWaypoint","takeoff");
                                   xcsoar.fire_legacy_event("StatusMessage","Takeoff");
                                end
)

xcsoar.input_event.new("gce_landing",
                                function(e)
                                   xcsoar.fire_legacy_event("StatusMessage","Landing");
                                   xcsoar.fire_legacy_event("AutoLogger","stop");
                                end
)

xcsoar.input_event.new("gce_flightmode_finalglide_above",
                                function(e)
                                   xcsoar.fire_legacy_event("StatusMessage","Above final glide");
                                end
)

xcsoar.input_event.new("gce_flightmode_finalglide_below",
                                function(e)
                                   xcsoar.fire_legacy_event("StatusMessage","Below final glide");
                                end
)

xcsoar.input_event.new("gce_flightmode_finalglide_terrain",
                                function(e)
                                   xcsoar.fire_legacy_event("StatusMessage","Final glide through terrain");
                                end
)

xcsoar.input_event.new("gce_landable_unreachable",
                                function(e)
                                   xcsoar.fire_legacy_event("Beep","1");
                                end
)

xcsoar.input_event.new("gce_task_start",
                       function(e)
                          xcsoar.fire_legacy_event("Beep","1");
                          xcsoar.fire_legacy_event("TaskTransition","start");
                       end
)

xcsoar.input_event.new("gce_task_finish",
                       function(e)
                          xcsoar.fire_legacy_event("Beep","1");
                          xcsoar.fire_legacy_event("TaskTransition","finish");
                       end
)

xcsoar.input_event.new("gce_task_nextwaypoint",
                       function(e)
                          xcsoar.fire_legacy_event("Beep","1");
                          xcsoar.fire_legacy_event("TaskTransition","next");
                       end
)

xcsoar.input_event.new("gce_startup_simulator",
                                function(e)
                                   print("start sim!");
                                   -- e:cancel();
                                end
)
