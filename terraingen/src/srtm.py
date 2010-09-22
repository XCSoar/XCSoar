import os
import math

import georect
from zipfile import ZipFile
import subprocess
import shutil

cmd_gdal_merge = "gdal_merge.py"
cmd_gdal_warp = "gdalwarp" 
cmd_geojasper = "geojasper" 

'''
 1) prepare tiles
'''
def __make_tile_name(lat, lon):
    return "srtm_%02d_%02d" % (((lon + 180) / 5) + 1, (60 - lat) / 5)

def __prepare_tile(source, destination, lat, lon):
    # generate filename to search for
    filename = __make_tile_name(lat, lon)
    
    # check if the GeoTIFF file already exists in the temporary folder
    if os.path.exists(destination + filename + ".tif"):
        print "Tile " + filename + " found!"
        return os.path.relpath(destination + filename + ".tif")
    
    # check if the GeoTIFF file exists in the data folder
    if os.path.exists(source + filename + ".tif"):
        print "Tile " + filename + " found!"
        return os.path.relpath(source + filename + ".tif")
    
    # check if the ZIP file exists in the data folder
    if not os.path.exists(source + filename + ".zip"):
        print "Tile " + filename + " can not be found!"
        return None
    
    print "Tile " + filename + " found inside zip file! -> Decompressing ..."
    # decompress the ZIP file to the temporary folder
    zip = ZipFile(source + filename + ".zip", "r")
    zip.extract(filename + ".tif", destination)
    zip.close()

    # check if the GeoTIFF file now exists in the temporary folder
    if os.path.exists(destination + filename + ".tif"):
        return os.path.relpath(destination + filename + ".tif")
    
    print "Decompression failed!"
    return None

def gather_tiles(source, destination, rc):
    '''
    Makes sure the terrain tiles are available at a certain location.
    @param source: Data path
    @param destination: Temporary path
    @param rc: Bounding box (GeoRect)
    @return: The list of tile files
    '''
    if not isinstance(rc, georect.GeoRect):
        return None

    print "Gathering terrain tiles ..."
    
    # Calculate rounded bounds
    lat_start = int(math.floor(rc.bottom.value_degrees() / 5.0)) * 5
    lon_start = int(math.floor(rc.left.value_degrees() / 5.0)) * 5
    lat_end = int(math.ceil(rc.top.value_degrees() / 5.0)) * 5
    lon_end = int(math.ceil(rc.right.value_degrees() / 5.0)) * 5
    
    tiles = []
    # Iterate through latitude and longitude in 5 degree interval
    for lat in range(lat_start, lat_end, 5):
        for lon in range(lon_start, lon_end, 5):
            tile = __prepare_tile(source, destination, lat, lon)
            if tile != None:
                # If tile is available append its filename to the tiles list
                tiles.append(tile)
    
    # Return list of available tile files
    return tiles

'''
 2) merge tiles into big tif 
    cmd_gdal_merge.py 
    -n -32768 
        (Ignore pixels from files being merged in with this pixel value.)
    -init -32768 
        (Pre-initialize the output image bands with these values. However, 
         it is not marked as the nodata value in the output file. If only one 
         value is given, the same value is used in all the bands.)
    -o blabla_merged.tif 
        (The name of the output file, which will be created if it does 
         not already exist.)
    a.tif b.tif c.tif ...
        (Input files)
'''
def merge_tiles(destination, tiles):
    print "Merging terrain tiles ...",
    if os.path.exists(destination + "terrain_merged.tif"):
        os.unlink(destination + "terrain_merged.tif")
        
    args = [cmd_gdal_merge,
            "-n", "-32768",
            "-init", "-32768",
            "-o", os.path.relpath(destination + "terrain_merged.tif")]
    args.extend(tiles)
    p = subprocess.Popen(args)
    p.wait()
    print "done"

'''
 3) resample merged image
    gdalwarp 
    -r cubicspline
        (Resampling method to use. Cubic spline resampling.)
    -tr $degrees_per_pixel $degrees_per_pixel 
        (set output file resolution (in target georeferenced units))
    -wo "INTERLEAVE=BIL" 
        (Set a warp options. The GDALWarpOptions::papszWarpOptions docs show 
         all options. Multiple -wo options may be listed.)
    -wt Int16 
        (Working pixel data type. The data type of pixels in the source 
         image and destination image buffers.)
    -of "GTiff"
        (Select the output format. The default is GeoTIFF (GTiff). 
         Use the short format name.)
    -srcnodata -32768 
        (Set nodata masking values for input bands (different values can be 
         supplied for each band). If more than one value is supplied all values 
         should be quoted to keep them together as a single operating system 
         argument. Masked values will not be used in interpolation. Use a value 
         of None to ignore intrinsic nodata settings on the source dataset.)
    -dstnodata -1 
        (Set nodata values for output bands (different values can be supplied 
         for each band). If more than one value is supplied all values should 
         be quoted to keep them together as a single operating system argument. 
         New files will be initialized to this value and if possible the 
         nodata value will be recorded in the output file.)
    blabla_merged.tif 
        (Input file)
    blabla_resampled.tif
        (Output file)
'''    
def resample(destination, arcseconds_per_pixel):
    print "Resampling terrain ...",
    if os.path.exists(destination + "terrain_resampled.tif"):
        os.unlink(destination + "terrain_resampled.tif")
        
    degree_per_pixel = float(arcseconds_per_pixel) / 3600.0

    args = [cmd_gdal_warp,
            "-r", "cubicspline",
            "-tr", str(degree_per_pixel), str(degree_per_pixel),
            "-wo", "INTERLEAVE=BIL",
            "-wt", "Int16",
            "-of", "GTiff",
            "-srcnodata", "-32768",
            "-dstnodata", "-1",
            os.path.relpath(destination + "terrain_merged.tif"),
            os.path.relpath(destination + "terrain_resampled.tif")]
    p = subprocess.Popen(args)
    p.wait()
    print "done"

'''
 4) crop resampled image
    gdalwarp 
    -srcnodata -1 
        (Set nodata masking values for input bands (different values can be 
         supplied for each band). If more than one value is supplied all values 
         should be quoted to keep them together as a single operating system 
         argument. Masked values will not be used in interpolation. Use a value 
         of None to ignore intrinsic nodata settings on the source dataset.)
    -dstnodata -1 
        (Set nodata values for output bands (different values can be supplied 
         for each band). If more than one value is supplied all values should 
         be quoted to keep them together as a single operating system argument. 
         New files will be initialized to this value and if possible the 
         nodata value will be recorded in the output file.)
    -te $left $bottom $right $top 
        (set georeferenced extents of output file to be created (in target SRS))
    blabla_resampled.tif 
        (Input file)
    blabla_cropped.tif
        (Output file)
'''    
def crop(destination, rc):
    print "Cropping terrain ...",
    if os.path.exists(destination + "terrain.tif"):
        os.unlink(destination + "terrain.tif")
        
    args = [cmd_gdal_warp,
            "-srcnodata", "-1",
            "-dstnodata", "-1",
            "-te", str(rc.left.value_degrees()),
            str(rc.bottom.value_degrees()),
            str(rc.right.value_degrees()),
            str(rc.top.value_degrees()),
            os.path.relpath(destination + "terrain_resampled.tif"),
            os.path.relpath(destination + "terrain.tif")]
    p = subprocess.Popen(args)
    p.wait()
    print "done"

'''
 5) convert to GeoJP2 with GeoJasPer
    cmd_geojasper 
    -f blabla_cropped.tif 
        (Input file name)
    -F terrain.jp2 
        (Output file name)
    -T jp2
        (Output type)
    -O rate=0.1 
        (Compression rate, 10 times)
    -O tilewidth=256 
        (generate tiled image using tile width 256)
    -O tileheight=256 
        (generate tiled image using tile height 256)
    -O xcsoar=1
        (???)
'''    
def convert(destination, rc):
    print "Converting terrain to GeoJP2 format ...",
    if os.path.exists(destination + "terrain.jp2"):
        os.unlink(destination + "terrain.jp2")
        
    args = [cmd_geojasper,
            "-f", os.path.relpath(destination + "terrain.tif"),
            "-F" ,os.path.relpath(destination + "terrain.jp2"),
            "-T", "jp2",
            "-O", "rate=0.1",
            "-O", "tilewidth=256",
            "-O", "tileheight=256",
            "-O", "xcsoar=1",
            "-O", "lonmin=" + str(rc.left.value_degrees()),
            "-O", "lonmax=" + str(rc.right.value_degrees()),
            "-O", "latmax=" + str(rc.top.value_degrees()),
            "-O", "latmin=" + str(rc.bottom.value_degrees())]
    p = subprocess.Popen(args)
    p.wait()
    print "done"

def Create(bounds, arcseconds_per_pixel = 9.0, 
           dir_data = "../data/", dir_temp = "../tmp/"):
    dir_data = os.path.abspath(dir_data) 
    dir_temp = os.path.abspath(dir_temp) 
    
    # Make sure the tiles are available
    tiles = gather_tiles(dir_data + "/", dir_temp + "/", bounds)
    merge_tiles(dir_temp + "/", tiles)
    resample(dir_temp + "/", arcseconds_per_pixel)
    crop(dir_temp + "/", bounds)
    convert(dir_temp + "/", bounds)
    
    return [dir_temp + "/terrain.jp2", False]

    