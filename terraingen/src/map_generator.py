import os.path
import shutil
from zipfile import ZipFile, ZIP_DEFLATED, ZIP_STORED
from georect import GeoRect
from waypoint_list import WaypointList
import srtm
from ogr2ogr import ogr2ogr

class MapGenerator:
    __files = []
    
    def __init__(self, dir_data = "../data/", dir_temp = "../tmp/"):
        '''
        Constructor of the MapGenerator class
        @param dir_data: Path of the data folder
        @param dir_temp: Path of the temporary folder
        '''
        self.__dir_data = os.path.abspath(dir_data) 
        self.__dir_temp = os.path.abspath(dir_temp) 
        
        if not os.path.exists(self.__dir_temp):
            os.mkdir(self.__dir_temp)
            
        self.__bounds = None

        print "MapGenerator created"
    
    def AddWaypointFile(self, filename):
        '''
        Adds a waypoint file to the map
        @param filename: The file that should be added
        '''
        print "Adding waypoint file ...",
        if not os.path.exists(filename):
            print "failed!"
            print "(" + filename + " not found!)"
            return False
            
        dst = os.path.abspath(self.__dir_temp + "/waypoints.xcw")
        shutil.copy(filename, dst)
        if not os.path.exists(dst):
            print "failed!"
            print ("(Copying " + os.path.basename(filename) +
                   " to " + dst + " not possible!)")
            return False

        self.__files.append([dst, True])
        print "done"
        return True
            
    def AddTopology(self):
        print "Adding topology ...",
        if self.__bounds == None:
            print "failed!"
            print "(Boundaries undefined!)"
            return False
        
        print ""
        o = ogr2ogr()
        files = o.copy_clipped(self.__bounds, self.__dir_temp)
        self.__files.extend(files)
        self.__files.append([o.generate_tpl_file(self.__dir_temp), True])
        
        return True
        
    def AddTerrain(self):
        print "Adding terrain ...",
        if self.__bounds == None:
            print "failed!"
            print "(Boundaries undefined!)"
            return False
        
        print ""
        terrain_file = srtm.Create(self.__bounds, 
                                   self.__dir_data, self.__dir_temp)
        if terrain_file == None:
            return False
        
        self.__files.append(terrain_file)
        return True
        
    def SetBounds(self, bounds):
        print "Setting map boundaries ...",
        
        if not isinstance(bounds, GeoRect):
            print "failed"
            print "(GeoRect expected)"
            return False
        
        self.__bounds = bounds
        print "done"
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
        
        print "Reading waypoint file ...",
        if not isinstance(filename, str):
            print "failed!"
            print "(String expected)"
            return False
        
        if not os.path.exists(filename):
            print "failed!"
            print "(" + filename + " not found!)"
            return False
        
        f = open(filename, "r")
        wplist = WaypointList()
        wplist.parse(f)
        f.close()
        print "done"
        
        return self.SetBounds(wplist.get_bounds())        
                
    def Create(self, filename):
        '''
        Creates the map at the given location
        @param filename: Location of the map file that should be created
        '''
        print "Creating map file ...",
        # Open the zip file
        z = ZipFile(filename, "w")
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
                z.write(file[0], None, ZIP_STORED)
            else:
                z.write(file[0], None, ZIP_DEFLATED)
        z.close()
        print "done"        