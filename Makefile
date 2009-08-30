#
# This is the XCSoar build script.  To compile XCSoar, you must
# specify the target platform, e.g. for Pocket PC 2003, type:
#
#   make TARGET=PPC2003
#
# The following parameters may be specified on the "make" command
# line:
#
#   TARGET      The name of the target platform.  See the TARGETS variable
#               below for a list of valid target platforms.
#
#   DEBUG       If set to "y", the debugging version of XCSoar is built.
#
#   V           Verbosity; 1 is the default, and prints terse information.
#               0 means quiet, and 2 prints the full compiler commands.
#

TARGETS = PC PPC2002 PPC2003 PPC2003X PNA WM5 WINE ALTAIR ALTAIRPORTRAIT

ifeq ($(TARGET),)
$(error No target specified.  Set the variable TARGET to one of: $(TARGETS))
endif

ifeq ($(filter $(TARGET),$(TARGETS)),)
$(error Invalid target: $(TARGET))
endif

#
SRC=Common/Source
HDR=Common/Header
#
PROFILE		:=
OPTIMIZE	:=-O2
# -Wdisabled-optimization
# -Wunused -Wshadow -Wunreachable-code
CONFIG_PPC2002	:=n
CONFIG_PPC2003	:=n
CONFIG_ALTAIR	:=n
CONFIG_PC	:=n
CONFIG_WINE	:=n
ALTAIR_PORTRAIT :=n
CONFIG_PNA	:=n
MINIMAL		:=n
XSCALE		:=n
GTARGET		:=$(TARGET)

ifeq ($(TARGET),PPC2002)
  CONFIG_PPC2002	:=y
else
  ifeq ($(TARGET),PPC2003)
    CONFIG_PPC2003	:=y
  else
    ifeq ($(TARGET),PPC2003X)
      CONFIG_PPC2003	:=y
      XSCALE :=y
      GTARGET := PPC2003
    else
      ifeq ($(TARGET),PC)
        CONFIG_PC	:=y
      else
        ifeq ($(TARGET),WINE)
          CONFIG_WINE :=y
        else
          ifeq ($(TARGET),ALTAIR)
            CONFIG_ALTAIR :=y
	    MINIMAL :=y
	    XSCALE :=y
          endif
          ifeq ($(TARGET),ALTAIRPORTRAIT)
            CONFIG_ALTAIR :=y
	    ALTAIR_PORTRAIT :=y
	    MINIMAL       :=y
	    XSCALE	:=y
          endif
	  ifeq ($(TARGET),PNA)
	    CONFIG_PNA := y
	    CONFIG_PPC2003 := y
	    MINIMAL       :=n
	  endif
	  ifeq ($(TARGET),WM5)
	    CONFIG_WM5 := y
	    MINIMAL := n
	  endif
	  ifeq ($(TARGET),WM5X)
	    CONFIG_WM5 := y
	    MINIMAL := n
	    XSCALE := y
	  endif
	endif
      endif
    endif
  endif
endif

############# build and CPU info

ifeq ($(CONFIG_PC),y)
TCPATH		:=i586-mingw32msvc-
CPU		:=i586
MCPU		:= -mcpu=$(CPU)
else

ifeq ($(CONFIG_WINE),y)
TCPATH		:=wine
CPU		:=i586
MCPU		:= -mcpu=$(CPU)
else

TCPATH		:=arm-mingw32ce-

ifeq ($(XSCALE),y)
CPU		:=xscale
MCPU		:= -mcpu=$(CPU)
else
CPU		:=
MCPU		:=
endif

ifeq ($(TARGET),PNA)
CPU		:=arm1136j-s
MCPU		:=
endif
ifeq ($(CONFIG_PPC2002),y)
CPU		:=strongarm1110
MCPU		:= -mcpu=$(CPU)
endif

endif

endif

############# platform info

ifeq ($(CONFIG_PPC2002),y)
CE_MAJOR	:=3
CE_MINOR	:=00
CE_PLATFORM	:=310
TARGET		:=PPC2002
PCPU		:=ARM
endif
ifeq ($(CONFIG_PPC2003),y)
CE_MAJOR	:=4
CE_MINOR	:=00
CE_PLATFORM	:=400
PCPU		:=ARMV4
endif

ifeq ($(CONFIG_WM5),y)
CE_MAJOR := 5
CE_MINOR := 00
CE_PLATFORM := 500
PCPU := ARMV4
endif

# armv4i
ifeq ($(CONFIG_ALTAIR),y)
CE_MAJOR	:=5
CE_MINOR	:=00
CE_PLATFORM	:=500
TARGET		:=ALTAIR
ifeq ($(ALTAIR_PORTRAIT),y)
TARGET          :=ALTAIRPORTRAIT
endif
endif

ifeq ($(CONFIG_PC),y)
# armv4i
CE_MAJOR	:=5
CE_MINOR	:=00
CE_PLATFORM	:=500
TARGET		:=PC
endif
ifeq ($(CONFIG_WINE),y)
# armv4i
CE_MAJOR	:=5
CE_MINOR	:=00
CE_PLATFORM	:=500
TARGET		:=WINE
CONFIG_PC	:=y
endif

######## output files

OUTPUTS 	:= XCSoar-$(TARGET).exe XCSoarSimulator-$(TARGET).exe
ifeq ($(CONFIG_ALTAIR),y)
OUTPUTS 	:= XCSoar-$(TARGET).exe
endif
ifeq ($(ALTAIR_PORTRAIT),y)
OUTPUTS 	:= XCSoar-$(TARGET).exe
endif
ifeq ($(CONFIG_PNA),y)
OUTPUTS 	:= XCSoar-$(TARGET).exe
endif

######## tools

EXE		:=$(findstring .exe,$(MAKE))
AR		:=$(TCPATH)ar$(EXE)
CXX		:=$(TCPATH)g++$(EXE)
CC		:=$(TCPATH)gcc$(EXE)
SIZE		:=$(TCPATH)size$(EXE)
STRIP		:=$(TCPATH)strip$(EXE)
WINDRES		:=$(TCPATH)windres$(EXE)
SYNCE_PCP	:=synce-pcp
SYNCE_PRM	:=synce-prm
CE_VERSION	:=0x0$(CE_MAJOR)$(CE_MINOR)
ARFLAGS		:=r

ifeq ($(CONFIG_WINE),y)
AR = ar$(EXE)
STRIP = strip$(EXE)
WINDRES = wrc$(EXE)
endif

######## windows definitions

ifeq ($(CONFIG_PC),y)
CE_DEFS		:=-D_WIN32_WINDOWS=$(CE_VERSION) -DWINVER=$(CE_VERSION)
CE_DEFS		+=-D_WIN32_IE=$(CE_VERSION) -DWINDOWSPC=1
else
CE_DEFS		:=-D_WIN32_WCE=$(CE_VERSION)
CE_DEFS		+=-DWIN32_PLATFORM_PSPC=$(CE_PLATFORM)
endif

UNICODE		:= -DUNICODE -D_UNICODE

######## paths

ifeq ($(CONFIG_WINE),y)
INCLUDES	:= -I$(HDR) -I$(SRC)
else
INCLUDES	:= -I$(HDR)/mingw32compat -I$(HDR) -I$(SRC)
endif

######## compiler flags

CPPFLAGS	:= $(INCLUDES) $(CE_DEFS)
CPPFLAGS	+= -DFLARM_AVERAGE
ifeq ($(CONFIG_PNA),y)
CPPFLAGS	+= -DBIGDISPLAY -DCECORE -DPNA
endif

ifeq ($(CONFIG_PC),y)
CPPFLAGS	+= -D_WINDOWS -DWIN32 -DCECORE -DUNDER_CE=300
  ifeq ($(CONFIG_WINE),y)
CPPFLAGS	+= -D__MINGW32__ -D__WINE__
# -mno-cygwin
  else
CPPFLAGS	+= $(UNICODE) -D_MBCS
  endif
else
CPPFLAGS	+= -D_ARM_ $(UNICODE)
  ifeq ($(CONFIG_ALTAIR),y)
CPPFLAGS 	+=-IPPC2005 -DGNAV
    ifeq ($(ALTAIR_PORTRAIT),y)
CPPFLAGS	+= -DFORCEPORTRAIT
    endif
  endif
endif

ifneq ($(CONFIG_WINE),y)
CPPFLAGS += -DHAVE_MSVCRT
endif

ifeq ($(DEBUG),y)
OPTIMIZE := -O0 -ggdb
else
CPPFLAGS += -DNDEBUG -Wuninitialized
endif

CXXFLAGS	:=$(OPTIMIZE) -fno-exceptions $(PROFILE)
CFLAGS		:=$(OPTIMIZE) $(PROFILE)

####### linker configuration

ifneq ($(CONFIG_WINE),y)
LDFLAGS		:=-Wl,--major-subsystem-version=$(CE_MAJOR)
LDFLAGS		+=-Wl,--minor-subsystem-version=$(CE_MINOR)
ifeq ($(CONFIG_PC),y)
LDFLAGS		+=-Wl,-subsystem,windows
endif
endif

LDFLAGS		+=$(PROFILE)

ifeq ($(CONFIG_PC),y)
LDLIBS		:= -lcomctl32 -lkernel32 -luser32 -lgdi32 -ladvapi32 -lwinmm -lmsimg32 -lstdc++
else
  LDLIBS		:= -lcommctrl -lstdc++
  ifeq ($(MINIMAL),n)
    LDLIBS		+= -laygshell
    ifneq ($(TARGET),PNA)
      LDLIBS		+= -limgdecmp
    endif
  endif
endif

####### compiler target

ifeq ($(CONFIG_PC),y)
TARGET_ARCH	:=-mwindows -march=i586 -mms-bitfields
else

TARGET_ARCH	:=-mwin32 $(MCPU)
ifeq ($(TARGET),PNA)
TARGET_ARCH	:=-mwin32
endif

endif
WINDRESFLAGS	:=-I$(HDR) -I$(SRC) $(CE_DEFS) -D_MINGW32_
ifeq ($(CONFIG_ALTAIR),y)
WINDRESFLAGS	+=-DGNAV
endif
MAKEFLAGS	+=-r

####### build verbosity

# Internal - Control verbosity
#  make V=0 - quiet
#  make V=1 - terse (default)
#  make V=2 - show commands
ifeq ($(V),2)
Q		:=
NQ		:=\#
else
Q		:=@
ifeq ($(V),0)
NQ		:=\#
else
NQ		:=
endif
endif

####### sources

ifeq ($(CONFIG_PC),n)
#CPPFLAGS_Common_Source_ :=-Werror
endif

DEVS	:=\
	$(SRC)/Device/devAltairPro.o \
	$(SRC)/Device/devBorgeltB50.o \
	$(SRC)/Device/devCAI302.o \
	$(SRC)/Device/devCaiGpsNav.o \
	$(SRC)/Device/devCondor.o \
	$(SRC)/Device/devEW.o \
	$(SRC)/Device/devEWMicroRecorder.o \
	$(SRC)/Device/devFlymasterF1.o \
	$(SRC)/Device/devGeneric.o \
	$(SRC)/Device/devLX.o \
	$(SRC)/Device/devNmeaOut.o \
	$(SRC)/Device/devPosiGraph.o \
	$(SRC)/Device/devVega.o \
	$(SRC)/Device/devVolkslogger.o \
	$(SRC)/Device/devXCOM760.o \
	$(SRC)/Device/devZander.o

DLGS	:=\
	$(SRC)/Dialogs/dlgAirspace.o \
	$(SRC)/Dialogs/dlgAirspaceColours.o \
	$(SRC)/Dialogs/dlgAirspaceDetails.o \
	$(SRC)/Dialogs/dlgAirspacePatterns.o \
	$(SRC)/Dialogs/dlgAirspaceSelect.o \
	$(SRC)/Dialogs/dlgAirspaceWarning.o \
	$(SRC)/Dialogs/dlgBasicSettings.o \
	$(SRC)/Dialogs/dlgBrightness.o \
	$(SRC)/Dialogs/dlgChecklist.o \
	$(SRC)/Dialogs/dlgComboPicker.o \
	$(SRC)/Dialogs/dlgConfiguration.o \
	$(SRC)/Dialogs/dlgConfiguration2.o \
	$(SRC)/Dialogs/dlgConfigurationVario.o \
	$(SRC)/Dialogs/dlgFlarmTraffic.o \
	$(SRC)/Dialogs/dlgHelp.o \
	$(SRC)/Dialogs/dlgLoggerReplay.o \
	$(SRC)/Dialogs/dlgStartPoint.o \
	$(SRC)/Dialogs/dlgStartup.o \
	$(SRC)/Dialogs/dlgStatistics.o \
	$(SRC)/Dialogs/dlgStatus.o \
	$(SRC)/Dialogs/dlgStatusSystem.o \
	$(SRC)/Dialogs/dlgStatusTask.o \
	$(SRC)/Dialogs/dlgSwitches.o \
	$(SRC)/Dialogs/dlgTarget.o \
	$(SRC)/Dialogs/dlgTaskCalculator.o \
	$(SRC)/Dialogs/dlgTaskOverview.o \
	$(SRC)/Dialogs/dlgTaskRules.o \
	$(SRC)/Dialogs/dlgTaskWaypoint.o \
	$(SRC)/Dialogs/dlgTeamCode.o \
	$(SRC)/Dialogs/dlgTextEntry.o \
	$(SRC)/Dialogs/dlgTextEntry_Keyboard.o \
	$(SRC)/Dialogs/dlgTools.o \
	$(SRC)/Dialogs/dlgVegaDemo.o \
	$(SRC)/Dialogs/dlgVoice.o \
	$(SRC)/Dialogs/dlgWayPointDetails.o \
	$(SRC)/Dialogs/dlgWaypointEdit.o \
	$(SRC)/Dialogs/dlgWayPointSelect.o \
	$(SRC)/Dialogs/dlgWaypointOutOfTerrain.o \
	$(SRC)/Dialogs/dlgWeather.o \
	$(SRC)/Dialogs/dlgWindSettings.o \
	$(SRC)/Dialogs/dlgStartTask.o \
	$(SRC)/Dialogs/dlgFontEdit.o \

VOLKS	:=\
	$(SRC)/Device/Volkslogger/dbbconv.cpp \
	$(SRC)/Device/Volkslogger/grecord.cpp \
	$(SRC)/Device/Volkslogger/vlapi2.cpp \
	$(SRC)/Device/Volkslogger/vlapihlp.cpp \
	$(SRC)/Device/Volkslogger/vlapisys_win.cpp \
	$(SRC)/Device/Volkslogger/vlconv.cpp \
	$(SRC)/Device/Volkslogger/vlutils.cpp

OBJS	:=\
	\
	$(SRC)/AATDistance.o 		\
	$(SRC)/Airspace.o 		\
	$(SRC)/AirspaceParser.o 	\
	$(SRC)/AirspaceWarning.o 	\
	$(SRC)/Atmosphere.o 		\
	$(SRC)/BestAlternate.o 		\
	$(SRC)/ClimbAverageCalculator.o \
	$(SRC)/ConditionMonitor.o 	\
	$(SRC)/Calibration.o 		\
	$(SRC)/Calculations.o 		\
	$(SRC)/Calculations2.o 		\
	$(SRC)/CalculationsAbort.o 	\
	$(SRC)/CalculationsAirspace.o 	\
	$(SRC)/CalculationsAutoMc.o 	\
	$(SRC)/CalculationsBallast.o 	\
	$(SRC)/CalculationsClimb.o 	\
	$(SRC)/CalculationsSector.o 	\
	$(SRC)/CalculationsTask.o 	\
	$(SRC)/CalculationsTerrain.o 	\
	$(SRC)/CalculationsVario.o 	\
	$(SRC)/CalculationsVertical.o 	\
	$(SRC)/CalculationsWind.o 	\
	$(SRC)/FlarmIdFile.o 		\
	$(SRC)/FlarmCalculations.o 	\
	$(SRC)/FlightStatistics.o 	\
	$(SRC)/GlideComputer.o 		\
	$(SRC)/GlideRatio.o 		\
	$(SRC)/GlideSolvers.o 		\
	$(SRC)/Logger.o 		\
	$(SRC)/LoggerSign.o 		\
	$(SRC)/ReplayLogger.o 		\
	$(SRC)/McReady.o 		\
	$(SRC)/NavFunctions.o		\
	$(SRC)/OnLineContest.o 		\
	$(SRC)/Persist.o 		\
	$(SRC)/SnailTrail.o 		\
	$(SRC)/Task.o			\
	$(SRC)/TeamCodeCalculation.o 	\
	$(SRC)/ThermalLocator.o 	\
	$(SRC)/Waypointparser.o 	\
	$(SRC)/WayPoint.o 		\
	$(SRC)/windanalyser.o 		\
	$(SRC)/windmeasurementlist.o 	\
	$(SRC)/windstore.o 		\
	$(SRC)/WindZigZag.o 		\
	\
	$(SRC)/Gauge/GaugeCDI.o \
	$(SRC)/Gauge/GaugeFLARM.o \
	$(SRC)/Gauge/GaugeVarioAltA.o \
	\
	$(SRC)/AirfieldDetails.o 	\
	$(SRC)/ButtonLabel.o 		\
	$(SRC)/DataField/Base.o 	\
	$(SRC)/DataField/Boolean.o 	\
	$(SRC)/DataField/ComboList.o 	\
	$(SRC)/DataField/Enum.o 	\
	$(SRC)/DataField/FileReader.o 	\
	$(SRC)/DataField/Float.o 	\
	$(SRC)/DataField/Integer.o 	\
	$(SRC)/DataField/String.o 	\
	$(SRC)/Dialogs.o 		\
	$(SRC)/ExpandMacros.o 		\
	$(SRC)/Formatter/Base.o 	\
	$(SRC)/Formatter/TeamCode.o 	\
	$(SRC)/Formatter/WayPoint.o 	\
	$(SRC)/Formatter/LowWarning.o 	\
	$(SRC)/Formatter/Time.o 	\
	$(SRC)/InfoBox.o 		\
	$(SRC)/InfoBoxLayout.o 		\
	$(SRC)/InfoBoxEvents.o 		\
	$(SRC)/InfoBoxManager.o 	\
	$(SRC)/InputEvents.o 		\
	$(SRC)/InputEventsActions.o 	\
	$(SRC)/Message.o 		\
	$(SRC)/WindowControls.o 	\
	$(SRC)/LogFile.o 		\
	\
	$(SRC)/MapWindow.o 		\
	$(SRC)/MapWindowAirspace.o 	\
	$(SRC)/MapWindowBase.o 		\
	$(SRC)/MapWindowEvents.o	\
	$(SRC)/MapWindowGlideRange.o 	\
	$(SRC)/MapWindowGraphics.o 	\
	$(SRC)/MapWindowLabels.o 	\
	$(SRC)/MapWindowProjection.o 	\
	$(SRC)/MapWindowRender.o 	\
	$(SRC)/MapWindowScale.o 	\
	$(SRC)/MapWindowSymbols.o 	\
	$(SRC)/MapWindowTask.o 		\
	$(SRC)/MapWindowTarget.o	\
	$(SRC)/MapWindowThermal.o 	\
	$(SRC)/MapWindowTraffic.o 	\
	$(SRC)/MapWindowTrail.o 	\
	$(SRC)/MapWindowWaypoints.o 	\
	$(SRC)/MapWindow2.o 		\
	\
	$(SRC)/Topology.o		\
	$(SRC)/TopologyStore.o		\
	$(SRC)/RasterMap.o 		\
	$(SRC)/RasterTerrain.o 		\
	$(SRC)/RasterWeather.o 		\
	$(SRC)/TerrainRenderer.o	\
	$(SRC)/Marks.o 			\
	\
	$(SRC)/Asset.o 			\
	$(SRC)/Appearance.o 		\
	$(SRC)/Battery.o 		\
	$(SRC)/Language.o 		\
	$(SRC)/LocalPath.o 		\
	$(SRC)/Interface.o		\
	$(SRC)/LocalTime.o		\
	$(SRC)/Units.o 			\
	$(SRC)/UtilsAirspace.o		\
	$(SRC)/UtilsFLARM.o		\
	$(SRC)/UtilsFont.o		\
	$(SRC)/UtilsProfile.o		\
	$(SRC)/UtilsSettings.o		\
	$(SRC)/UtilsSystem.o		\
	$(SRC)/UtilsText.o		\
	$(SRC)/Version.o 		\
	$(SRC)/Audio/VegaVoice.o	\
	$(SRC)/Compatibility/string.o 	\
	$(SRC)/Registry.o 		\
	$(SRC)/xmlParser.o 		\
	\
	$(SRC)/Math/Earth.o 		\
	$(SRC)/Math/FastMath.o 		\
	$(SRC)/Math/Geometry.o 		\
	$(SRC)/Math/leastsqs.o 		\
	$(SRC)/Math/LowPassFilter.o 	\
	$(SRC)/Math/Pressure.o 		\
	$(SRC)/Math/Screen.o 		\
	$(SRC)/Math/SunEphemeris.o 	\
	\
	$(SRC)/Screen/Animation.o 	\
	$(SRC)/Screen/Blank.o 		\
	$(SRC)/Screen/Chart.o 		\
	$(SRC)/Screen/Fonts.o 		\
	$(SRC)/Screen/Graphics.o 	\
	$(SRC)/Screen/Ramp.o 		\
	$(SRC)/Screen/STScreenBuffer.o 	\
	$(SRC)/Screen/Util.o 		\
	$(SRC)/Screen/VOIMAGE.o 	\
	$(SRC)/Screen/Bitmap.o \
	$(SRC)/Screen/Brush.o \
	$(SRC)/Screen/Canvas.o \
	$(SRC)/Screen/VirtualCanvas.o \
	$(SRC)/Screen/BufferCanvas.o \
	$(SRC)/Screen/BitmapCanvas.o \
	$(SRC)/Screen/PaintCanvas.o \
	$(SRC)/Screen/Font.o \
	$(SRC)/Screen/Pen.o \
	$(SRC)/Screen/Window.o \
	$(SRC)/Screen/BufferWindow.o \
	$(SRC)/Screen/PaintWindow.o \
	$(SRC)/Screen/TextWindow.o \
	$(SRC)/Screen/EditWindow.o \
	$(SRC)/Screen/MainWindow.o \
	\
	$(SRC)/Polar/WinPilot.o 	\
	$(SRC)/Polar/BuiltIn.o 		\
	$(SRC)/Polar/Historical.o 	\
	\
	$(SRC)/Screen/shapelib/mapbits.o 	\
	$(SRC)/Screen/shapelib/maperror.o 	\
	$(SRC)/Screen/shapelib/mapprimitive.o 	\
	$(SRC)/Screen/shapelib/mapsearch.o 	\
	$(SRC)/Screen/shapelib/mapshape.o 	\
	$(SRC)/Screen/shapelib/maptree.o 	\
	$(SRC)/Screen/shapelib/mapxbase.o 	\
	\
	$(SRC)/StdAfx.o			\
	$(SRC)/Protection.o 		\
	$(SRC)/ProcessTimer.o 		\
	$(SRC)/Components.o 		\
	$(SRC)/XCSoar.o 		\
	\
	$(SRC)/Device/device.o 		\
	$(SRC)/Device/Geoid.o 		\
	$(SRC)/Device/Parser.o		\
	$(SRC)/Device/Port.o 		\
	$(DEVS) 			\
	\
	$(DLGS:.cpp=.o) 		\
	$(VOLKS:.cpp=.o) 		\
	$(SRC)/XCSoar.rsc 		\
	$(SRC)/jasper.a 		\
	$(SRC)/zzip.a 			\
	$(SRC)/compat.a

#	$(SRC)/VarioSound.o \
#	$(SRC)/WaveThread.o \

ifeq ($(CONFIG_ALTAIR),y)
OBJS += PPC2005/aygShellWrp.o
endif

XCSOARSETUP_OBJS=\
	$(SRC)/XcSoarSetup.o

XCSOARLAUNCH_OBJS=\
	$(SRC)/XCSoarLaunch.o

ZZIPSRC	:=$(SRC)/zzip
ZZIP	:=\
	$(ZZIPSRC)/adler32.c	 	$(ZZIPSRC)/compress.c \
	$(ZZIPSRC)/crc32.c 		$(ZZIPSRC)/deflate.c \
	$(ZZIPSRC)/err.c 		$(ZZIPSRC)/fetch.c \
	$(ZZIPSRC)/file.c 		\
	$(ZZIPSRC)/infback.c 		$(ZZIPSRC)/inffast.c \
	$(ZZIPSRC)/inflate.c 		$(ZZIPSRC)/info.c \
	$(ZZIPSRC)/inftrees.c 		$(ZZIPSRC)/plugin.c \
	$(ZZIPSRC)/trees.c 		$(ZZIPSRC)/uncompr.c \
	$(ZZIPSRC)/zip.c 		$(ZZIPSRC)/zstat.c \
	$(ZZIPSRC)/zutil.c

JASSRC	:=$(SRC)/jasper
JASPER	:=\
	$(JASSRC)/base/jas_cm.c 	$(JASSRC)/base/jas_debug.c \
	$(JASSRC)/base/jas_getopt.c	$(JASSRC)/base/jas_icc.c \
	$(JASSRC)/base/jas_iccdata.c 	$(JASSRC)/base/jas_image.c \
	$(JASSRC)/base/jas_init.c 	$(JASSRC)/base/jas_malloc.c \
	$(JASSRC)/base/jas_seq.c 	$(JASSRC)/base/jas_stream.c \
	$(JASSRC)/base/jas_string.c 	$(JASSRC)/base/jas_tvp.c \
	$(JASSRC)/base/jas_version.c	$(JASSRC)/jp2/jp2_cod.c \
	$(JASSRC)/jp2/jp2_dec.c 	$(JASSRC)/jpc/jpc_bs.c \
	$(JASSRC)/jpc/jpc_cs.c 		$(JASSRC)/jpc/jpc_dec.c \
	$(JASSRC)/jpc/jpc_math.c 	$(JASSRC)/jpc/jpc_mct.c \
	$(JASSRC)/jpc/jpc_mqdec.c       $(JASSRC)/jpc/jpc_mqcod.c \
	$(JASSRC)/jpc/jpc_qmfb.c 	$(JASSRC)/jpc/jpc_rtc.cpp \
	$(JASSRC)/jpc/jpc_t1dec.c 	$(JASSRC)/jpc/jpc_t1enc.c \
	$(JASSRC)/jpc/jpc_t1cod.c \
	$(JASSRC)/jpc/jpc_t2dec.c 	$(JASSRC)/jpc/jpc_t2cod.c \
	$(JASSRC)/jpc/jpc_tagtree.c	$(JASSRC)/jpc/jpc_tsfb.c \
	$(JASSRC)/jpc/jpc_util.c 	$(JASSRC)/jpc/RasterTile.cpp

COMPATSRC:=$(SRC)/wcecompat
COMPAT	:=\
	$(COMPATSRC)/string_extras.c \
	$(COMPATSRC)/ts_string.cpp

ifneq ($(CONFIG_WINE),y)
COMPAT += $(COMPATSRC)/errno.cpp
endif

all:	$(OUTPUTS)

####### products

install: XCSoar-$(TARGET).exe XCSoarSimulator-$(TARGET).exe
	@echo Copying to device...
	-$(SYNCE_PRM) ':/Program Files/XCSoar/XCSoar.exe'
	-$(SYNCE_PRM) ':/Program Files/XCSoar/XCSoarSimulator.exe'
	$(SYNCE_PCP) XCSoar-$(TARGET).exe ':/Program Files/XCSoar/XCSoar.exe'
	$(SYNCE_PCP) XCSoarSimulator-$(TARGET).exe ':/Program Files/XCSoar/XCSoarSimulator.exe'

cab:	XCSoar-$(TARGET).exe XCSoarSimulator-$(TARGET).exe
	@echo Making cabs
	cp XCSoar-$(TARGET).exe $(GTARGET)/XCSoar/gcc/XCSoar.exe
	cp XCSoarSimulator-$(TARGET).exe $(GTARGET)/XCSoarSimulator/gcc/XCSoarSimulator.exe
	wine $(GTARGET)/Cabwiz.exe XCSoar$(TARGET)-gcc.inf /cpu $(PCPU)
	mv XCSoar$(TARGET)-gcc.$(PCPU).CAB XCSoar$(TARGET).$(PCPU).CAB

#	wine ezsetup.exe -l english -i XCSoar$(TARGET).ini -r installmsg.txt -e gpl.txt -o InstallXCSoar-$(TARGET).exe

XCSoar-$(TARGET).exe: XCSoar-$(TARGET)-ns.exe
	@$(NQ)echo "  STRIP   $@"
	$(Q)$(STRIP) $< -o $@
	$(Q)$(SIZE) $@

XCSoarSimulator-$(TARGET).exe: XCSoarSimulator-$(TARGET)-ns.exe
	@$(NQ)echo "  STRIP   $@"
	$(Q)$(STRIP) $< -o $@
	$(Q)$(SIZE) $@

XCSoar-$(TARGET)-ns.exe: $(OBJS:.o=-$(TARGET).o)
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

XCSoarSimulator-$(TARGET)-ns.exe: $(OBJS:.o=-$(TARGET)-Simulator.o)
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

XCSoarSetup.dll: $(XCSOARSETUP_OBJS)
	$(CC) -shared $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@
# JMW not tested yet, probably need to use dlltool?

XCSoarLaunch.dll: $(XCSOARLAUNCH_OBJS)
	$(CC) -shared $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

#
# Create libraries for zzip, jasper and compatibility stuff
#
$(SRC)/zzip.a: $(patsubst %.cpp,%.o,$(ZZIP:.c=.o))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^

$(SRC)/jasper.a: $(patsubst %.cpp,%.o,$(JASPER:.c=.o))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^

$(SRC)/compat.a: $(patsubst %.cpp,%.o,$(COMPAT:.c=.o))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^

####### shared objects
#
# Tell make how to create a compiled resource object (rsc)
#
%.rsc: %.rc
	@sed -e 's,[Bb]itmaps\\\\,Bitmaps/,g' \
	    -e 's,XCSoar.ICO,xcsoar.ico,g' \
	    -e 's,\.\.\\\\Data\\\\Dialogs\\\\,../Data/Dialogs/,g' \
	    -e 's,small\.bmp,Small.bmp,g' \
		< $< > $<.tmp
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) $<.tmp $@
	@$(RM) $<.tmp

####### dependency handling

DEPFILE		=$(dir $@).$(notdir $@).d
DEPFLAGS	=-Wp,-MD,$(DEPFILE)
dirtarget	=$(subst \\,_,$(subst /,_,$(dir $@)))
cc-flags	=$(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(CPPFLAGS_$(dirtarget)) $(TARGET_ARCH)
cxx-flags	=$(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(CPPFLAGS_$(dirtarget)) $(TARGET_ARCH)

#
# Useful debugging targets - make preprocessed versions of the source
#
%.i: %.cpp FORCE
	$(CXX) $(cxx-flags) -E $(OUTPUT_OPTION) $<

%.s: %.cpp FORCE
	$(CXX) $(cxx-flags) -S $(OUTPUT_OPTION) $<

%.i: %.c FORCE
	$(CC) $(cc-flags) -E $(OUTPUT_OPTION) $<

####### build rules

#
# Provide our own rules for building...
#
%-$(TARGET).o: %.c
	@$(NQ)echo "  CC      $@"
	$(Q)$(CC) $(cc-flags) -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)

%-$(TARGET).o: %.cpp
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) $(cxx-flags) -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)

%-$(TARGET).o: %.cxx
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) $(cxx-flags) -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)

%-$(TARGET)-Simulator.o: %.c
	@$(NQ)echo "  CC      $@"
	$(Q)$(CC) $(cc-flags) -D_SIM_ -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)

%-$(TARGET)-Simulator.o: %.cpp
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) $(cxx-flags) -D_SIM_ -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)

%-$(TARGET)-Simulator.o: %.cxx
	@$(NQ)echo "  CXX     $@"
	$(Q)$(CXX) $(cxx-flags) -D_SIM_ -c $(OUTPUT_OPTION) $<
	@sed -i '1s,^[^ :]*,$@,' $(DEPFILE)

IGNORE	:= \( -name .svn -o -name CVS -o -name .git \) -prune -o

clean: cleani FORCE
	find . $(IGNORE) \( -name '*.[oa]' -o -name '*.rsc' -o -name '.*.d' \) \
	-type f -print | xargs -r $(RM)
	$(RM) XCSoar-$(TARGET)-ns.exe XCSoarSimulator-$(TARGET)-ns.exe

cleani: FORCE
	find . $(IGNORE) \( -name '*.i' \) \
		-type f -print | xargs -r $(RM)

.PHONY: FORCE

ifneq ($(wildcard $(SRC)/.*.d),)
include $(wildcard $(SRC)/.*.d)
endif
ifneq ($(wildcard $(SRC)/*/.*.d),)
include $(wildcard $(SRC)/*/.*.d)
endif

