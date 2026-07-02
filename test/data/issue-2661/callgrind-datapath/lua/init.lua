-- Run from the repository root with:
--   XCSOAR_DATA=test/data/issue-2661/callgrind-datapath output/UNIX/dbg/bin/xcsoar

local nmea = "test/data/issue-2661/short_2026-06-20_11-47.nmea"

if not xcsoar.replay.is_active then
  xcsoar.replay.start(nmea)
end

local count = xcsoar.replay.process_all()

xcsoar.map.show()

-- Zoom out so the full trail is on screen, then redraw at several scales.
for _ = 1, 25 do
  xcsoar.map.zoom(-1)
end

for _ = 1, 15 do
  xcsoar.map.zoom(1)
end

for _ = 1, 10 do
  xcsoar.map.zoom(-1)
end
