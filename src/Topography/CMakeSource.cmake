set(_SOURCES
        Topography/CachedTopographyRenderer.cpp
        Topography/Thread.cpp
        Topography/TopographyFile.cpp
        Topography/TopographyFileRenderer.cpp
        Topography/TopographyGlue.cpp
        Topography/TopographyRenderer.cpp
        Topography/TopographyStore.cpp
        Topography/XShape.cpp
        Topography/Index.cpp
        Topography/ShapeFile.cpp
)

list(APPEND _SOURCES
        Topography/shapelib/mapalloc.c
        Topography/shapelib/mapbits.c
        Topography/shapelib/mapprimitive.c
        Topography/shapelib/mapsearch.c
        Topography/shapelib/mapshape.c
        Topography/shapelib/mapstring.cpp
        Topography/shapelib/maptree.c
        Topography/shapelib/mapxbase.c
)

set(SCRIPT_FILES
    CMakeSource.cmake
)



