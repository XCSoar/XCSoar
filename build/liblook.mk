# Build rules for the Look library

LOOK_SRC_DIR = $(SRC)/

LOOK_SOURCES := \
	$(SRC)/Look/AutoFont.cpp \
	$(SRC)/Look/Look.cpp \
	$(SRC)/Look/DialogLook.cpp \
	$(SRC)/Look/ButtonLook.cpp \
	$(SRC)/Look/CheckBoxLook.cpp \
	$(SRC)/Look/TerminalLook.cpp \
	$(SRC)/Look/VarioLook.cpp \
	$(SRC)/Look/ChartLook.cpp \
	$(SRC)/Look/MapLook.cpp \
	$(SRC)/Look/OverlayLook.cpp \
	$(SRC)/Look/TopographyLook.cpp \
	$(SRC)/Look/WindArrowLook.cpp \
	$(SRC)/Look/ThermalBandLook.cpp \
	$(SRC)/Look/TraceHistoryLook.cpp \
	$(SRC)/Look/AirspaceLook.cpp \
	$(SRC)/Look/TrailLook.cpp \
	$(SRC)/Look/CrossSectionLook.cpp \
	$(SRC)/Look/GestureLook.cpp \
	$(SRC)/Look/HorizonLook.cpp \
	$(SRC)/Look/TaskLook.cpp \
	$(SRC)/Look/TrafficLook.cpp \
	$(SRC)/Look/InfoBoxLook.cpp \
	$(SRC)/Look/WaypointLook.cpp \
	$(SRC)/Look/AircraftLook.cpp \
	$(SRC)/Look/NOAALook.cpp \
	$(SRC)/Look/FinalGlideBarLook.cpp \
	$(SRC)/Look/FlarmTrafficLook.cpp \
	$(SRC)/Look/VarioBarLook.cpp \
	$(SRC)/Look/IconLook.cpp \
	$(SRC)/Look/ThermalAssistantLook.cpp \
	$(SRC)/Look/WaveLook.cpp \
	$(SRC)/Look/ClimbPercentLook.cpp

LOOK_CPPFLAGS_INTERNAL = $(SCREEN_CPPFLAGS)

$(eval $(call link-library,liblook,LOOK))
