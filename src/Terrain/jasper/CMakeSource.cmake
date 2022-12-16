# set(JASSRC     Terrain/jasper)  # klappt nicht...
# set(JASSRC     jasper)  
set(JASSRC     .)  
# set(JASSRC     ${CMAKE_CURRENT_SOURCE_DIR}/jasper) 
list(APPEND  _SOURCES
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

      ${JASSRC}/jp2/jp2_cod.c

      ${JASSRC}/base/jas_stream.c
      ${JASSRC}/base/jas_seq.c
      ${JASSRC}/base/jas_malloc.c
)

set(SCRIPT_FILES CMakeSource.cmake)
