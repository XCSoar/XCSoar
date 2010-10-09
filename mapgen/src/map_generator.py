import os.path
import shutil
from zipfile import ZipFile, ZIP_DEFLATED, ZIP_STORED
from georect import GeoRect
from waypoint_list import WaypointList
import terrain_srtm
import topology_vmap0

class MapGenerator:
    def __init__(self, dir_data = "../data/", dir_temp = "../tmp/"):
        '''
        Constructor of the MapGenerator class
        @param dir_data: Path of the data folder
        @param dir_temp: Path of the temporary folder
        '''
        self.__dir_data = os.path.abspath(dir_data)
        if not os.path.exists(self.__dir_data):
            os.mkdir(self.__dir_data)

        self.__dir_temp = os.path.abspath(dir_temp)
        if not os.path.exists(self.__dir_temp):
            os.mkdir(self.__dir_temp)

        self.__bounds = None
        self.__files = []

        print "MapGenerator created"

    def AddWaypointFile(self, filename):
        '''
        Adds a waypoint file to the map
        @param filename: The file that should be added
        '''
        print "Adding waypoint file ..."
        if not os.path.exists(filename):
            print "failed! (" + filename + " not found!)"
            return False

        dst = os.path.abspath(self.__dir_temp + "/waypoints.xcw")
        shutil.copy(filename, dst)
        if not os.path.exists(dst):
            print ("failed! (Copying " + os.path.basename(filename) +
                   " to " + dst + " not possible!)")
            return False

        self.__files.append([dst, True])
        
        return True
            
    def AddWaypointDetailsFile(self, filename):
        '''
        Adds a waypoint details file to the map
        @param filename: The file that should be added
        '''
        print "Adding waypoint details file ..."
        if not os.path.exists(filename):
            print "failed! (" + filename + " not found!)"
            return False
            
        dst = os.path.abspath(self.__dir_temp + "/airfields.txt")
        shutil.copy(filename, dst)
        if not os.path.exists(dst):
            print ("failed! (Copying " + os.path.basename(filename) +
                   " to " + dst + " not possible!)")
            return False

        self.__files.append([dst, True])
        
        return True
            
    def AddAirspaceFile(self, filename):
        '''
        Adds a airspace file to the map
        @param filename: The file that should be added
        '''
        print "Adding airspace file ..."
        if not os.path.exists(filename):
            print "failed! (" + filename + " not found!)"
            return False
            
        dst = os.path.abspath(self.__dir_temp + "/airspace.txt")
        shutil.copy(filename, dst)
        if not os.path.exists(dst):
            print ("failed! (Copying " + os.path.basename(filename) +
                   " to " + dst + " not possible!)")
            return False

        self.__files.append([dst, True])
        
        return True
            
    def AddTopology(self, bounds = None):
        print "Adding topology ..."

        if bounds == None: 
            if self.__bounds == None:
                print "failed! (Boundaries undefined!)"
                return False
            
            bounds = self.__bounds

        topology_files = topology_vmap0.Create(bounds, self.__dir_data,
                                               self.__dir_temp)
        if topology_files == None:
            print "Topology creation failed!"
            return False

        self.__files.extend(topology_files)
        return True

    def AddTerrain(self, arcseconds_per_pixel = 9.0, bounds = None):
        print "Adding terrain ..."

        if bounds == None: 
            if self.__bounds == None:
                print "failed! (Boundaries undefined!)"
                return False
            
            bounds = self.__bounds

        terrain_files = terrain_srtm.Create(bounds, arcseconds_per_pixel,
                                           self.__dir_data, self.__dir_temp)
        if terrain_files == None:
            print "Terrain creation failed!"
            return False

        self.__files.extend(terrain_files)
        return True

    def SetBounds(self, bounds):
        print "Setting map boundaries ..."

        if not isinstance(bounds, GeoRect):
            print "failed! (GeoRect expected)"
            return False

        self.__bounds = bounds
        print "(", self.__bounds, ")"
        return True

    def SetBoundsSeperatly(self, latmin, latmax, lonmin, lonmax):
        self.__bounds = GeoRect()
        self.__bounds.left = lonmin
        self.__bounds.right = lonmax
        self.__bounds.top = latmax
        self.__bounds.bottom = latmin

    def SetBoundsByWaypointFile(self, filename):
        print "Setting map boundaries to match waypoint file contents ..."

        print "Reading waypoint file ..."
        if not isinstance(filename, basestring):
            print "failed! (String expected)"
            return False

        if not os.path.exists(filename):
            print "failed! (" + filename + " not found!)"
            return False

        f = open(filename, "r")
        wplist = WaypointList()
        wplist.parse(f)
        f.close()

        return self.SetBounds(wplist.get_bounds())

    def Create(self, filename):
        '''
        Creates the map at the given location
        @param filename: Location of the map file that should be created
        '''
        print "Creating map file ..."
        # Open the zip file
        z = ZipFile(filename, "w", ZIP_DEFLATED)
        for file in self.__files:
            # Make sure we have a list
            if not isinstance(file, list):
                # ... or at least a filename string
                if not isinstance(file, str):
                    continue
                # Convert string to list (default = compress)
                file = [file, True]

            # Make sure the list has at least two entries
            if len(file) < 2:
                continue

            # Make sure the file exists
            if not os.path.isfile(file[0]):
                continue

            # Check if we should compress the file
            if file[1] == False:
                z.write(file[0], os.path.basename(file[0]), ZIP_STORED)
            else:
                z.write(file[0], os.path.basename(file[0]), ZIP_DEFLATED)
        z.close()
        
    def Cleanup(self):
        for file in self.__files:
            if isinstance(file, list):
                file = file[0]
            os.unlink(file)
        self.__files = []
