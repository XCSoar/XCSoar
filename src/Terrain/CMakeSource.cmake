set(TERRAIN_DIR     ${CMAKE_CURRENT_SOURCE_DIR}) 
set(_SOURCES
        ${TERRAIN_DIR}/HeightMatrix.cpp
        ${TERRAIN_DIR}/Intersection.cpp
        ${TERRAIN_DIR}/Loader.cpp
        ${TERRAIN_DIR}/RasterBuffer.cpp
        ${TERRAIN_DIR}/RasterMap.cpp
        ${TERRAIN_DIR}/RasterProjection.cpp
        ${TERRAIN_DIR}/RasterRenderer.cpp
        ${TERRAIN_DIR}/RasterTerrain.cpp
        ${TERRAIN_DIR}/RasterTile.cpp
        ${TERRAIN_DIR}/RasterTileCache.cpp
        ${TERRAIN_DIR}/ScanLine.cpp
        ${TERRAIN_DIR}/TerrainRenderer.cpp
        ${TERRAIN_DIR}/TerrainSettings.cpp
        ${TERRAIN_DIR}/Thread.cpp
        ${TERRAIN_DIR}/WorldFile.cpp
        ${TERRAIN_DIR}/ZzipStream.cpp

        ${TERRAIN_DIR}/AsyncLoader.cpp
)
if(0)  # # jasper is an own project..

# set(JASSRC     Terrain/jasper)  # klappt nicht...
set(JASSRC     jasper)  
# set(JASSRC     ${CMAKE_CURRENT_SOURCE_DIR}/jasper) 
list(APPEND  _SOURCES
# list(APPEND  jasper_SOURCES
      ${JASSRC}/jpc/jpc_bs.c
      ${JASSRC}/jpc/jpc_cs.c
      ${JASSRC}/jpc/jpc_dec.c
      ${JASSRC}/jpc/jpc_math.c
      ${JASSRC}/jpc/jpc_mqdec.c
      ${JASSRC}/jpc/jpc_mqcod.c
      ${JASSRC}/jpc/jpc_qmfb.c
      ${JASSRC}/jpc/jpc_rtc.cpp
      ${JASSRC}/jpc/jpc_t1dec.c
      ${JASSRC}/jpc/jpc_t1cod.c
      ${JASSRC}/jpc/jpc_t2dec.c
      ${JASSRC}/jpc/jpc_t2cod.c
      ${JASSRC}/jpc/jpc_tagtree.c
      ${JASSRC}/jpc/jpc_tsfb.c

      ${JASSRC}/base/jas_stream.c
      ${JASSRC}/base/jas_seq.c
      ${JASSRC}/base/jas_malloc.c
)
endif(0)  # # jasper is an own project..


set(SCRIPT_FILES CMakeSource.cmake)

add_compile_definitions(JAS_INCLUDE_JPG_CODEC=1)
