# Location ./XCSoarAug/src/CMakeSource.cmake

# file(GLOB_RECURSE SOURCE_FILES  ${PROJECTGROUP_SOURCE_DIR}/src/*.cpp)
# file(GLOB_RECURSE HEADER_FILES  ${PROJECTGROUP_SOURCE_DIR}/src/*.h*)
file(GLOB HEADER_FILES  *.h*)

set(BASIC_SOURCES
    ${SRC}/ActionInterface.cpp
    ${SRC}/ApplyExternalSettings.cpp
    ${SRC}/ApplyVegaSwitches.cpp
    ${SRC}/Asset.cpp
    ${SRC}/BallastDumpManager.cpp
    ${SRC}/BatteryTimer.cpp
    ${SRC}/Components.cpp
    ${SRC}/CommandLine.cpp
    ${SRC}/DataGlobals.cpp
    ${SRC}/FlightStatistics.cpp
    ${SRC}/HorizonWidget.cpp
    ${SRC}/Interface.cpp
    ${SRC}/LocalPath.cpp
    ${SRC}/MainWindow.cpp
    ${SRC}/ProcessTimer.cpp
    ${SRC}/ProgressWindow.cpp
    ${SRC}/ProgressGlue.cpp
    ${SRC}/Protection.cpp
    ${SRC}/RadioFrequency.cpp
    ${SRC}/ResourceLoader.cpp
    ${SRC}/Simulator.cpp
    ${SRC}/Startup.cpp
    ${SRC}/UIActions.cpp

    ${SRC}/Pan.cpp

    ${SRC}/PageSettings.cpp
    ${SRC}/PageState.cpp
    ${SRC}/PageActions.cpp
    ${SRC}/StatusMessage.cpp
    ${SRC}/PopupMessage.cpp
    ${SRC}/Message.cpp

    ${SRC}/LogFile.cpp
    ${SRC}/DrawThread.cpp
   
    ${SRC}/UIReceiveBlackboard.cpp
    ${SRC}/UIGlobals.cpp
    ${SRC}/UIState.cpp
    ${SRC}/UISettings.cpp
    ${SRC}/DisplaySettings.cpp
    ${SRC}/MapSettings.cpp
    ${SRC}/SystemSettings.cpp

    ${SRC}/MergeThread.cpp
    ${SRC}/CalculationThread.cpp
    ${SRC}/DisplayMode.cpp
   
    ${SRC}/UtilsSettings.cpp
    ${SRC}/UtilsSystem.cpp
    ${SRC}/Version.cpp
   
    ${SRC}/RateLimiter.cpp
    ${SRC}/TeamActions.cpp
)

set(SOURCE_FILES ${BASIC_SOURCES} )
set(SCRIPT_FILES CMakeSource.cmake
  ../ide/xcsoar.natvis
)

file(GLOB ICON_FILES  ${PROJECTGROUP_SOURCE_DIR}/Data/icons/*.svg)
