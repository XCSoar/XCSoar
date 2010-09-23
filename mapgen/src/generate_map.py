import sys
from map_generator import MapGenerator

def print_help():
    print "Usage:   generate_map.py [Options] -wpf waypoint_file output_file"
    print "         generate_map.py [Options] -b latmin latmax lonmin lonmax output_file"
    print ""
    print "Options: -r resolution (terrain resolution, arcseconds per pixel)"

def main():
    args = sys.argv[1:]

    wp = None
    latmin = None
    output_file = None
    res = 9.0
    for i in range(len(args)):
        if args[i] == "-r" and i + 1 < len(args):
            res = args[i + 1]
        if args[i] == "-wpf" and i + 2 < len(args):
            wp = args[i + 1]
            output_file = args[i + 2]
        if args[i] == "-b" and i + 5 < len(args):
            latmin = args[i + 1]
            latmax = args[i + 2]
            lonmin = args[i + 3]
            lonmax = args[i + 4]
            output_file = args[i + 5]

    if (wp == None and latmin == None) or output_file == None:
        print_help()
        return

    m = MapGenerator()
    if wp != None:
        m.AddWaypointFile(wp)
        m.SetBoundsByWaypointFile(wp)
    if latmin != None:
        m.SetBoundsSeperatly(latmin, latmax, lonmin, lonmax)
    m.AddTopology()
    m.AddTerrain(res)
    m.Create(output_file)
    m.Cleanup()

if __name__ == "__main__":
    main()
    