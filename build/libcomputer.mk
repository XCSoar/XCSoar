LIBCOMPUTER_SOURCES = \
	$(SRC)/Computer/Wind/CirclingWind.cpp \
	$(SRC)/Computer/Wind/MeasurementList.cpp \
	$(SRC)/Computer/Wind/Store.cpp \
	$(SRC)/Computer/Wind/WindEKF.cpp \
	$(SRC)/Computer/Wind/WindEKFGlue.cpp \
	$(SRC)/Computer/Wind/Settings.cpp \
	$(SRC)/Computer/ClimbAverageCalculator.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitor.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorAATTime.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorFinalGlide.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorGlideTerrain.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorLandableReachable.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorSunset.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitorWind.cpp \
	$(SRC)/Computer/ConditionMonitor/ConditionMonitors.cpp \
	$(SRC)/Computer/ConditionMonitor/AirspaceEnterMonitor.cpp \
	$(SRC)/Computer/ConditionMonitor/MoreConditionMonitors.cpp \
	$(SRC)/Computer/CuComputer.cpp \
	$(SRC)/Computer/FlyingComputer.cpp \
	$(SRC)/Computer/CirclingComputer.cpp \
	$(SRC)/Computer/ThermalBandComputer.cpp \
	$(SRC)/Computer/Wind/Computer.cpp \
	$(SRC)/Computer/ContestComputer.cpp \
	$(SRC)/Computer/TraceComputer.cpp \
	$(SRC)/Computer/WarningComputer.cpp \
	$(SRC)/Computer/ThermalRecency.cpp \
	$(SRC)/Computer/ThermalLocator.cpp \
	$(SRC)/Computer/ThermalBase.cpp \
	$(SRC)/Computer/LiftDatabaseComputer.cpp \
	$(SRC)/Computer/LogComputer.cpp \
	$(SRC)/Computer/AverageVarioComputer.cpp \
	$(SRC)/Computer/GlideRatioCalculator.cpp \
	$(SRC)/Computer/GlideRatioComputer.cpp \
	$(SRC)/Computer/GlideComputer.cpp \
	$(SRC)/Computer/GlideComputerBlackboard.cpp \
	$(SRC)/Computer/GlideComputerAirData.cpp \
	$(SRC)/Computer/WaveComputer.cpp \
	$(SRC)/Computer/StatsComputer.cpp \
	$(SRC)/Computer/RouteComputer.cpp \
	$(SRC)/Computer/TaskComputer.cpp \
	$(SRC)/Computer/GlideComputerInterface.cpp \
	$(SRC)/Computer/Events.cpp \
	$(SRC)/Computer/BasicComputer.cpp \
	$(SRC)/Computer/GroundSpeedComputer.cpp \
	$(SRC)/Computer/AutoQNH.cpp \
	$(SRC)/Computer/Settings.cpp

LIBCOMPUTER_DEPENDS = AIRSPACE TASK GEO LIBNMEA FMT

$(eval $(call link-library,libcomputer,LIBCOMPUTER))
