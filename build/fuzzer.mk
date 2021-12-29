FUZZER_SRC_DIR = $(topdir)/fuzzer/src

FUZZ_IGC_PARSER_SOURCES = \
	$(SRC)/IGC/IGCParser.cpp \
	$(FUZZER_SRC_DIR)/FuzzIGCParser.cpp
FUZZ_IGC_PARSER_DEPENDS = IO UTIL
$(eval $(call link-program,FuzzIGCParser,FUZZ_IGC_PARSER))

FUZZ_WAYPOINT_READER_SOURCES = \
	$(SRC)/Waypoint/WaypointFileType.cpp \
	$(SRC)/Waypoint/WaypointReaderBase.cpp \
	$(SRC)/Waypoint/WaypointReader.cpp \
	$(SRC)/Waypoint/WaypointReaderWinPilot.cpp \
	$(SRC)/Waypoint/WaypointReaderFS.cpp \
	$(SRC)/Waypoint/WaypointReaderOzi.cpp \
	$(SRC)/Waypoint/WaypointReaderSeeYou.cpp \
	$(SRC)/Waypoint/WaypointReaderZander.cpp \
	$(SRC)/Waypoint/WaypointReaderCompeGPS.cpp \
	$(SRC)/Waypoint/Factory.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Compatibility/fmode.c \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/RadioFrequency.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(FUZZER_SRC_DIR)/FuzzWaypointReader.cpp
FUZZ_WAYPOINT_READER_DEPENDS = WAYPOINT GEO MATH IO UTIL ZZIP OS THREAD
$(eval $(call link-program,FuzzWaypointReader,FUZZ_WAYPOINT_READER))

FUZZ_AIRSPACE_PARSER_SOURCES = \
	$(SRC)/Airspace/AirspaceParser.cpp \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp \
	$(SRC)/Operation/Operation.cpp \
	$(SRC)/Atmosphere/Pressure.cpp \
	$(TEST_SRC_DIR)/FakeTerrain.cpp \
	$(FUZZER_SRC_DIR)/FuzzAirspaceParser.cpp
FUZZ_AIRSPACE_PARSER_DEPENDS = IO OS AIRSPACE ZZIP GEO MATH UTIL
$(eval $(call link-program,FuzzAirspaceParser,FUZZ_AIRSPACE_PARSER))

FUZZ_TOPOGRAPHY_FILE_SOURCES = \
	$(SRC)/Topography/TopographyFile.cpp \
	$(SRC)/Topography/XShape.cpp \
	$(SRC)/Projection/Projection.cpp \
	$(SRC)/Projection/WindowProjection.cpp \
	$(FUZZER_SRC_DIR)/FuzzTopographyFile.cpp
FUZZ_TOPOGRAPHY_FILE_DEPENDS = SCREEN SHAPELIB ZZIP GEO MATH IO UTIL
$(eval $(call link-program,FuzzTopographyFile,FUZZ_TOPOGRAPHY_FILE))

FUZZ_TOPOGRAPHY_INDEX_SOURCES = \
	$(SRC)/Topography/Index.cpp \
	$(FUZZER_SRC_DIR)/FuzzTopographyIndex.cpp
FUZZ_TOPOGRAPHY_INDEX_DEPENDS = RESOURCE
$(eval $(call link-program,FuzzTopographyIndex,FUZZ_TOPOGRAPHY_INDEX))

OUTPUTS += \
	$(FUZZ_IGC_PARSER_BIN) \
	$(FUZZ_WAYPOINT_READER_BIN) \
	$(FUZZ_AIRSPACE_PARSER_BIN) \
	$(FUZZ_TOPOGRAPHY_INDEX_BIN) \
	$(FUZZ_TOPOGRAPHY_FILE_BIN)
