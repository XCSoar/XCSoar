FUZZER_SRC_DIR = $(topdir)/fuzzer/src

FUZZ_IGC_PARSER_SOURCES = \
	$(SRC)/IGC/IGCParser.cpp \
	$(FUZZER_SRC_DIR)/FuzzIGCParser.cpp
FUZZ_IGC_PARSER_DEPENDS = IO UTIL
$(eval $(call link-program,FuzzIGCParser,FUZZ_IGC_PARSER))

FUZZ_WAYPOINT_READER_SOURCES = \
	$(SRC)/Waypoint/Factory.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(FUZZER_SRC_DIR)/FuzzWaypointReader.cpp
FUZZ_WAYPOINT_READER_DEPENDS = WAYPOINTFILE GEO MATH IO OS UTIL ZZIP THREAD UNITS
$(eval $(call link-program,FuzzWaypointReader,FUZZ_WAYPOINT_READER))

FUZZ_AIRSPACE_PARSER_SOURCES = \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(FUZZER_SRC_DIR)/FuzzAirspaceParser.cpp
FUZZ_AIRSPACE_PARSER_DEPENDS = IO OS AIRSPACE ZZIP GEO MATH UTIL UNITS
$(eval $(call link-program,FuzzAirspaceParser,FUZZ_AIRSPACE_PARSER))

FUZZ_TOPOGRAPHY_FILE_SOURCES = \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Projection/WindowProjection.cpp \
	$(FUZZER_SRC_DIR)/FuzzTopographyFile.cpp
FUZZ_TOPOGRAPHY_FILE_DEPENDS = SCREEN TOPO ZZIP GEO MATH IO UTIL
$(eval $(call link-program,FuzzTopographyFile,FUZZ_TOPOGRAPHY_FILE))

FUZZ_TOPOGRAPHY_INDEX_SOURCES = \
	$(FUZZER_SRC_DIR)/FuzzTopographyIndex.cpp
FUZZ_TOPOGRAPHY_INDEX_DEPENDS = TOPO RESOURCE
$(eval $(call link-program,FuzzTopographyIndex,FUZZ_TOPOGRAPHY_INDEX))

OUTPUTS += \
	$(FUZZ_IGC_PARSER_BIN) \
	$(FUZZ_WAYPOINT_READER_BIN) \
	$(FUZZ_AIRSPACE_PARSER_BIN) \
	$(FUZZ_TOPOGRAPHY_INDEX_BIN) \
	$(FUZZ_TOPOGRAPHY_FILE_BIN)
