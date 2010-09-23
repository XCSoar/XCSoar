import sys
from map_generator import MapGenerator

def print_help():
    print "Usage:   generate_map.py [Options] -wpf waypoint_file"
    print "         generate_map.py [Options] -b latmin latmax lonmin lonmax"
    print ""
    print "Options: -r resolution (terrain resolution, arcseconds per pixel)"

def main():
    args = sys.argv[1:]
    
    wp = None
    latmin = None
    res = 9.0
    for i in range(len(args)):
        if args[i] == "-r" and i + 1 < len(args):
            res = args[i + 1]
        if args[i] == "-wpf" and i + 1 < len(args):
            wp = args[i + 1]
        if args[i] == "-b" and i + 4 < len(args):
            latmin = args[i + 1]
            latmax = args[i + 2]
            lonmin = args[i + 3]
            lonmax = args[i + 4]            
        
    if wp == None and latmin == None:
        print_help()
        return
    
    m = MapGenerator()
    m.AddWaypointFile(wp)
    if wp != None:
        m.SetBoundsByWaypointFile(wp)
    if latmin != None:
        m.SetBoundsSeperatly(latmin, latmax, lonmin, lonmax)
    m.AddTopology()
    m.AddTerrain(res)
    m.Create("../test.xcm.zip")

if __name__ == "__main__":
    main()