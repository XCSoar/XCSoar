DRIVER_SRC_DIR = $(SRC)/Device/Driver

VOLKSLOGGER_SOURCES = \
	$(DRIVER_SRC_DIR)/Volkslogger/Register.cpp \
	$(DRIVER_SRC_DIR)/Volkslogger/Parser.cpp \
	$(DRIVER_SRC_DIR)/Volkslogger/Protocol.cpp \
	$(DRIVER_SRC_DIR)/Volkslogger/Declare.cpp \
	$(DRIVER_SRC_DIR)/Volkslogger/Database.cpp \
	$(DRIVER_SRC_DIR)/Volkslogger/Util.cpp \
	$(DRIVER_SRC_DIR)/Volkslogger/dbbconv.cpp \
	$(DRIVER_SRC_DIR)/Volkslogger/grecord.cpp \
	$(DRIVER_SRC_DIR)/Volkslogger/vlapi2.cpp \
	$(DRIVER_SRC_DIR)/Volkslogger/vlapihlp.cpp \
	$(DRIVER_SRC_DIR)/Volkslogger/vlconv.cpp \
	$(DRIVER_SRC_DIR)/Volkslogger/Logger.cpp

CAI302_SOURCES = \
	$(DRIVER_SRC_DIR)/CAI302/Protocol.cpp \
	$(DRIVER_SRC_DIR)/CAI302/PocketNav.cpp \
	$(DRIVER_SRC_DIR)/CAI302/Mode.cpp \
	$(DRIVER_SRC_DIR)/CAI302/Parser.cpp \
	$(DRIVER_SRC_DIR)/CAI302/Settings.cpp \
	$(DRIVER_SRC_DIR)/CAI302/Declare.cpp \
	$(DRIVER_SRC_DIR)/CAI302/Logger.cpp \
	$(DRIVER_SRC_DIR)/CAI302/Manage.cpp \
	$(DRIVER_SRC_DIR)/CAI302/Register.cpp

IMI_SOURCES = \
	$(DRIVER_SRC_DIR)/IMI/Protocol/MessageParser.cpp \
	$(DRIVER_SRC_DIR)/IMI/Protocol/Communication.cpp \
	$(DRIVER_SRC_DIR)/IMI/Protocol/Checksum.cpp \
	$(DRIVER_SRC_DIR)/IMI/Protocol/Conversion.cpp \
	$(DRIVER_SRC_DIR)/IMI/Protocol/IGC.cpp \
	$(DRIVER_SRC_DIR)/IMI/Protocol/Protocol.cpp \
	$(DRIVER_SRC_DIR)/IMI/Declare.cpp \
	$(DRIVER_SRC_DIR)/IMI/Internal.cpp \
	$(DRIVER_SRC_DIR)/IMI/Logger.cpp \
	$(DRIVER_SRC_DIR)/IMI/Register.cpp

LX_SOURCES = \
	$(DRIVER_SRC_DIR)/LX/NanoLogger.cpp \
	$(DRIVER_SRC_DIR)/LX/NanoDeclare.cpp \
	$(DRIVER_SRC_DIR)/LX/Protocol.cpp \
	$(DRIVER_SRC_DIR)/LX/Mode.cpp \
	$(DRIVER_SRC_DIR)/LX/Parser.cpp \
	$(DRIVER_SRC_DIR)/LX/Settings.cpp \
	$(DRIVER_SRC_DIR)/LX/Declare.cpp \
	$(DRIVER_SRC_DIR)/LX/Logger.cpp \
	$(DRIVER_SRC_DIR)/LX/Convert.cpp \
	$(DRIVER_SRC_DIR)/LX/LXN.cpp \
	$(DRIVER_SRC_DIR)/LX/Register.cpp

FLARM_SOURCES = \
	$(DRIVER_SRC_DIR)/FLARM/Device.cpp \
	$(DRIVER_SRC_DIR)/FLARM/Register.cpp \
	$(DRIVER_SRC_DIR)/FLARM/Mode.cpp \
	$(DRIVER_SRC_DIR)/FLARM/Parser.cpp \
	$(DRIVER_SRC_DIR)/FLARM/StaticParser.cpp \
	$(DRIVER_SRC_DIR)/FLARM/Settings.cpp \
	$(DRIVER_SRC_DIR)/FLARM/Declare.cpp \
	$(DRIVER_SRC_DIR)/FLARM/Logger.cpp \
	$(DRIVER_SRC_DIR)/FLARM/CRC16.cpp \
	$(DRIVER_SRC_DIR)/FLARM/BinaryProtocol.cpp \
	$(DRIVER_SRC_DIR)/FLARM/TextProtocol.cpp

FLYTEC_SOURCES = \
	$(DRIVER_SRC_DIR)/Flytec/Register.cpp \
	$(DRIVER_SRC_DIR)/Flytec/Parser.cpp \
	$(DRIVER_SRC_DIR)/Flytec/Logger.cpp

VEGA_SOURCES = \
	$(DRIVER_SRC_DIR)/Vega/Misc.cpp \
	$(DRIVER_SRC_DIR)/Vega/Parser.cpp \
	$(DRIVER_SRC_DIR)/Vega/Settings.cpp \
	$(DRIVER_SRC_DIR)/Vega/Volatile.cpp \
	$(DRIVER_SRC_DIR)/Vega/Register.cpp

BLUEFLY_SOURCES = \
	$(DRIVER_SRC_DIR)/BlueFly/Misc.cpp \
	$(DRIVER_SRC_DIR)/BlueFly/Parser.cpp \
	$(DRIVER_SRC_DIR)/BlueFly/Settings.cpp \
	$(DRIVER_SRC_DIR)/BlueFly/Register.cpp

XCTRACER_SOURCES = \
	$(DRIVER_SRC_DIR)/XCTracer/Parser.cpp \
	$(DRIVER_SRC_DIR)/XCTracer/Register.cpp

THERMALEXPRESS_SOURCES = \
	$(DRIVER_SRC_DIR)/ThermalExpress/Driver.cpp
 
DRIVER_SOURCES = \
	$(SRC)/Device/Driver.cpp \
	$(SRC)/Device/Register.cpp \
	$(VOLKSLOGGER_SOURCES) \
	$(CAI302_SOURCES) \
	$(IMI_SOURCES) \
	$(LX_SOURCES) \
	$(FLARM_SOURCES) \
	$(FLYTEC_SOURCES) \
	$(VEGA_SOURCES) \
	$(BLUEFLY_SOURCES) \
	$(XCTRACER_SOURCES) \
	$(THERMALEXPRESS_SOURCES) \
	$(DRIVER_SRC_DIR)/AltairPro.cpp \
	$(DRIVER_SRC_DIR)/BorgeltB50.cpp \
	$(DRIVER_SRC_DIR)/CaiGpsNav.cpp \
	$(DRIVER_SRC_DIR)/CaiLNav.cpp \
	$(DRIVER_SRC_DIR)/Condor.cpp \
	$(DRIVER_SRC_DIR)/CProbe.cpp \
	$(DRIVER_SRC_DIR)/EW.cpp \
	$(DRIVER_SRC_DIR)/EWMicroRecorder.cpp \
	$(DRIVER_SRC_DIR)/Eye.cpp \
	$(DRIVER_SRC_DIR)/FlymasterF1.cpp \
	$(DRIVER_SRC_DIR)/FlyNet.cpp \
	$(DRIVER_SRC_DIR)/Generic.cpp \
	$(DRIVER_SRC_DIR)/LevilAHRS_G.cpp \
	$(DRIVER_SRC_DIR)/Leonardo.cpp \
	$(DRIVER_SRC_DIR)/NmeaOut.cpp \
	$(DRIVER_SRC_DIR)/OpenVario.cpp \
	$(DRIVER_SRC_DIR)/PosiGraph.cpp \
	$(DRIVER_SRC_DIR)/XCOM760.cpp \
	$(DRIVER_SRC_DIR)/ILEC.cpp \
	$(DRIVER_SRC_DIR)/Westerboer.cpp \
	$(DRIVER_SRC_DIR)/Zander.cpp \
	$(DRIVER_SRC_DIR)/Vaulter.cpp \
	$(DRIVER_SRC_DIR)/KRT2.cpp \
	$(DRIVER_SRC_DIR)/AirControlDisplay.cpp \
	$(DRIVER_SRC_DIR)/ATR833.cpp

$(eval $(call link-library,driver,DRIVER))
