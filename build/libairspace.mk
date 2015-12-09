AIRSPACE_SRC_DIR = $(SRC)/Engine/Airspace

AIRSPACE_SOURCES = \
	$(ENGINE_SRC_DIR)/Util/AircraftStateFilter.cpp \
	$(AIRSPACE_SRC_DIR)/AirspacesTerrain.cpp \
	$(AIRSPACE_SRC_DIR)/Airspace.cpp \
	$(AIRSPACE_SRC_DIR)/AirspaceAltitude.cpp \
	$(AIRSPACE_SRC_DIR)/AirspaceAircraftPerformance.cpp \
	$(AIRSPACE_SRC_DIR)/AbstractAirspace.cpp \
	$(AIRSPACE_SRC_DIR)/AirspaceCircle.cpp \
	$(AIRSPACE_SRC_DIR)/AirspacePolygon.cpp \
	$(AIRSPACE_SRC_DIR)/Airspaces.cpp \
	$(AIRSPACE_SRC_DIR)/AirspaceIntersectSort.cpp \
	$(AIRSPACE_SRC_DIR)/SoonestAirspace.cpp \
	$(AIRSPACE_SRC_DIR)/Predicate/AirspacePredicate.cpp \
	$(AIRSPACE_SRC_DIR)/Predicate/AirspacePredicateHeightRange.cpp \
	$(AIRSPACE_SRC_DIR)/Predicate/OutsideAirspacePredicate.cpp \
	$(AIRSPACE_SRC_DIR)/AirspaceIntersectionVisitor.cpp \
	$(AIRSPACE_SRC_DIR)/AirspaceWarningConfig.cpp \
	$(AIRSPACE_SRC_DIR)/AirspaceWarningManager.cpp \
	$(AIRSPACE_SRC_DIR)/AirspaceWarning.cpp \
	$(AIRSPACE_SRC_DIR)/AirspaceSorter.cpp

$(eval $(call link-library,libairspace,AIRSPACE))
