set(TEST_LIST

# ${SRC_DIR}/AddChecksum.cpp
# ${SRC_DIR}/AirspacePrinting.cpp
# ${SRC_DIR}/AnalyseFlight.cpp
# ${SRC_DIR}/AppendGRecord.cpp
# ${SRC_DIR}/ArcApprox.cpp
# ${SRC_DIR}/BenchmarkFAITriangleSector.cpp
# ${SRC_DIR}/BenchmarkProjection.cpp
# ${SRC_DIR}/CAI302Tool.cpp
# ${SRC_DIR}/ConsoleJobRunner.cpp
# ${SRC_DIR}/ContestPrinting.cpp
# ${SRC_DIR}/DebugDisplay.cpp
# ${SRC_DIR}/DebugPort.cpp
# ${SRC_DIR}/DebugReplay.cpp
# ${SRC_DIR}/DebugReplayIGC.cpp
# ${SRC_DIR}/DebugReplayNMEA.cpp
# ${SRC_DIR}/DownloadFile.cpp
# ${SRC_DIR}/DumpFlarmNet.cpp
# ${SRC_DIR}/DumpHexColor.cpp
# ${SRC_DIR}/DumpTaskFile.cpp
# ${SRC_DIR}/DumpTextInflate.cpp
# ${SRC_DIR}/DumpVario.cpp
# ${SRC_DIR}/EmulateDevice.cpp
# ${SRC_DIR}/EnumeratePorts.cpp
# ${SRC_DIR}/FakeAsset.cpp
# ${SRC_DIR}/FakeDialogs.cpp
# ${SRC_DIR}/FakeGeoid.cpp
# ${SRC_DIR}/FakeHelpDialog.cpp
# ${SRC_DIR}/FakeLanguage.cpp
# ${SRC_DIR}/FakeListPicker.cpp
# ${SRC_DIR}/FakeLogFile.cpp
# ${SRC_DIR}/FakeMessage.cpp
# ${SRC_DIR}/FakeNMEALogger.cpp
# ${SRC_DIR}/FakeProfile.cpp
# ${SRC_DIR}/FakeTerrain.cpp
# ${SRC_DIR}/FeedFlyNetData.cpp
# ${SRC_DIR}/FeedNMEA.cpp
# ${SRC_DIR}/FeedVega.cpp
# ${SRC_DIR}/FixGRecord.cpp
# ${SRC_DIR}/FlightPath.cpp
# ${SRC_DIR}/FlightPhaseDetector.cpp
# ${SRC_DIR}/FlightPhaseJSON.cpp
# ${SRC_DIR}/FlightTable.cpp
# ${SRC_DIR}/Fonts.cpp
# ${SRC_DIR}/IGC2NMEA.cpp
# ${SRC_DIR}/KeyCodeDumper.cpp
# ${SRC_DIR}/LoadImage.cpp
# ${SRC_DIR}/LoadTerrain.cpp
# ${SRC_DIR}/LoadTopography.cpp
# ${SRC_DIR}/LogPort.cpp
# ${SRC_DIR}/NearestWaypoints.cpp
# ${SRC_DIR}/PlayTone.cpp
# ${SRC_DIR}/PlayVario.cpp
# ${SRC_DIR}/Printing.cpp
# ${SRC_DIR}/ReadGRecord.cpp
# ${SRC_DIR}/ReadMO.cpp
# ${SRC_DIR}/ReadPort.cpp
# ${SRC_DIR}/ReadProfileInt.cpp
# ${SRC_DIR}/ReadProfileString.cpp
${SRC_DIR}/RunAirspaceParser.cpp
${SRC_DIR}/RunAirspaceWarningDialog.cpp
${SRC_DIR}/RunAnalysis.cpp
${SRC_DIR}/RunAngleEntry.cpp
${SRC_DIR}/RunCanvas.cpp
${SRC_DIR}/RunChartRenderer.cpp
${SRC_DIR}/RunCirclingWind.cpp
${SRC_DIR}/RunContestAnalysis.cpp
${SRC_DIR}/RunDeclare.cpp
${SRC_DIR}/RunDeviceDriver.cpp
${SRC_DIR}/RunDownloadFlight.cpp
${SRC_DIR}/RunDownloadToFile.cpp
${SRC_DIR}/RunEnableNMEA.cpp
${SRC_DIR}/RunExternalWind.cpp
${SRC_DIR}/RunFAITriangleSectorRenderer.cpp
${SRC_DIR}/RunFinalGlideBarRenderer.cpp
${SRC_DIR}/RunFlarmUtils.cpp
${SRC_DIR}/RunFlightList.cpp
${SRC_DIR}/RunFlightListRenderer.cpp
${SRC_DIR}/RunFlightLogger.cpp
${SRC_DIR}/RunFlightParser.cpp
${SRC_DIR}/RunFlyingComputer.cpp
${SRC_DIR}/RunGeoPointEntry.cpp
${SRC_DIR}/RunHeightMatrix.cpp
${SRC_DIR}/RunHorizonRenderer.cpp
${SRC_DIR}/RunIGCWriter.cpp
${SRC_DIR}/RunInputParser.cpp
${SRC_DIR}/RunJobDialog.cpp
${SRC_DIR}/RunKalmanFilter1d.cpp
${SRC_DIR}/RunLX1600Utils.cpp
${SRC_DIR}/RunListControl.cpp
${SRC_DIR}/RunLiveTrack24.cpp
${SRC_DIR}/RunLua.cpp
${SRC_DIR}/RunMD5.cpp
${SRC_DIR}/RunMapWindow.cpp
${SRC_DIR}/RunNOAADownloader.cpp
${SRC_DIR}/RunNumberEntry.cpp
${SRC_DIR}/RunPortHandler.cpp
${SRC_DIR}/RunProfileListDialog.cpp
${SRC_DIR}/RunProgressWindow.cpp
${SRC_DIR}/RunRenderOZ.cpp
${SRC_DIR}/RunRepositoryParser.cpp
${SRC_DIR}/RunSHA256.cpp
${SRC_DIR}/RunSkyLinesTracking.cpp
${SRC_DIR}/RunTask.cpp
${SRC_DIR}/RunTaskEditorDialog.cpp
${SRC_DIR}/RunTerminal.cpp
${SRC_DIR}/RunTextEntry.cpp
${SRC_DIR}/RunTextWriter.cpp
${SRC_DIR}/RunTimeEntry.cpp
${SRC_DIR}/RunTrace.cpp
${SRC_DIR}/RunVegaSettings.cpp
${SRC_DIR}/RunWPASupplicant.cpp
${SRC_DIR}/RunWaveComputer.cpp
${SRC_DIR}/RunWaypointParser.cpp
${SRC_DIR}/RunWindArrowRenderer.cpp
${SRC_DIR}/RunWindComputer.cpp
${SRC_DIR}/RunWindEKF.cpp
${SRC_DIR}/RunWindZigZag.cpp
${SRC_DIR}/RunXMLParser.cpp

${SRC_DIR}/RunWeGlideClient.cpp
${SRC_DIR}/TestAATPoint.cpp
${SRC_DIR}/TestARange.cpp
${SRC_DIR}/TestAirspaceParser.cpp
${SRC_DIR}/TestAllocatedGrid.cpp
${SRC_DIR}/TestAngle.cpp
${SRC_DIR}/TestByteSizeFormatter.cpp
${SRC_DIR}/TestCRC8.cpp
${SRC_DIR}/TestCRC16.cpp
${SRC_DIR}/TestCSVLine.cpp
${SRC_DIR}/TestClimbAvCalc.cpp
${SRC_DIR}/TestColorRamp.cpp
${SRC_DIR}/TestDateTime.cpp
${SRC_DIR}/TestDiffFilter.cpp
${SRC_DIR}/TestDriver.cpp
${SRC_DIR}/TestEarth.cpp
${SRC_DIR}/TestFileUtil.cpp
${SRC_DIR}/TestFlarmNet.cpp
${SRC_DIR}/TestFlatGeoPoint.cpp
${SRC_DIR}/TestFlatLine.cpp
${SRC_DIR}/TestFlatPoint.cpp
${SRC_DIR}/TestGRecord.cpp
${SRC_DIR}/TestGeoBounds.cpp
${SRC_DIR}/TestGeoClip.cpp
${SRC_DIR}/TestGeoPoint.cpp
${SRC_DIR}/TestGeoPointFormatter.cpp
${SRC_DIR}/TestGlidePolar.cpp
${SRC_DIR}/TestHexColorFormatter.cpp
${SRC_DIR}/TestHexString.cpp
${SRC_DIR}/TestIGCFilenameFormatter.cpp
${SRC_DIR}/TestIGCParser.cpp
${SRC_DIR}/TestLXNToIGC.cpp
${SRC_DIR}/TestLeastSquares.cpp
${SRC_DIR}/TestLine2D.cpp
${SRC_DIR}/TestLogger.cpp
${SRC_DIR}/TestMETARParser.cpp
${SRC_DIR}/TestMacCready.cpp
${SRC_DIR}/TestMath.cpp
${SRC_DIR}/TestMathTables.cpp
${SRC_DIR}/TestNotify.cpp
${SRC_DIR}/TestOrderedTask.cpp
${SRC_DIR}/TestOverwritingRingBuffer.cpp
${SRC_DIR}/TestPlanes.cpp
${SRC_DIR}/TestPolars.cpp
${SRC_DIR}/TestProfile.cpp
${SRC_DIR}/TestProjection.cpp
${SRC_DIR}/TestQuadrilateral.cpp
${SRC_DIR}/TestRadixTree.cpp
${SRC_DIR}/TestRoughTime.cpp
${SRC_DIR}/TestStrings.cpp
${SRC_DIR}/TestSunEphemeris.cpp
${SRC_DIR}/TestTaskPoint.cpp
${SRC_DIR}/TestTaskWaypoint.cpp
${SRC_DIR}/TestTeamCode.cpp
${SRC_DIR}/TestThermalBand.cpp
${SRC_DIR}/TestThermalBase.cpp
${SRC_DIR}/TestTimeFormatter.cpp
${SRC_DIR}/TestTrace.cpp
${SRC_DIR}/TestUTF8.cpp
${SRC_DIR}/TestUTM.cpp
${SRC_DIR}/TestUnits.cpp
${SRC_DIR}/TestUnitsFormatter.cpp
${SRC_DIR}/TestValidity.cpp
${SRC_DIR}/TestWaypointReader.cpp
${SRC_DIR}/TestWaypoints.cpp
${SRC_DIR}/TestWrapClock.cpp
${SRC_DIR}/TestZeroFinder.cpp
${SRC_DIR}/TestPolylineDecoder.cpp    # add 7.38
# ${SRC_DIR}/VerifyGRecord.cpp
# ${SRC_DIR}/ViewImage.cpp
# ${SRC_DIR}/WriteTextFile.cpp
# ${SRC_DIR}/harness_aircraft.cpp
# ${SRC_DIR}/harness_airspace.cpp
# ${SRC_DIR}/harness_flight.cpp
# ${SRC_DIR}/harness_task.cpp
# ${SRC_DIR}/harness_task2.cpp
# ${SRC_DIR}/harness_waypoints.cpp
# ${SRC_DIR}/lxn2igc.cpp
# ${SRC_DIR}/test_aat.cpp
# ${SRC_DIR}/test_acfilter.cpp
# ${SRC_DIR}/test_airspace.cpp
# ${SRC_DIR}/test_automc.cpp
# ${SRC_DIR}/test_bestcruisetrack.cpp
# ${SRC_DIR}/test_cruiseefficiency.cpp
# ${SRC_DIR}/test_debug.cpp
# ${SRC_DIR}/test_effectivemc.cpp
# ${SRC_DIR}/test_fixed.cpp
# ${SRC_DIR}/test_flight.cpp
# ${SRC_DIR}/test_highterrain.cpp
# ${SRC_DIR}/test_mc.cpp
# ${SRC_DIR}/test_modes.cpp
# ${SRC_DIR}/test_pressure.cpp
# ${SRC_DIR}/test_randomtask.cpp
# ${SRC_DIR}/test_reach.cpp
# ${SRC_DIR}/test_replay_olc.cpp
# ${SRC_DIR}/test_replay_retrospective.cpp
# ${SRC_DIR}/test_replay_task.cpp
# ${SRC_DIR}/test_route.cpp
# ${SRC_DIR}/test_task.cpp
# ${SRC_DIR}/test_troute.cpp
# ${SRC_DIR}/test_vopt.cpp
)

set(TEST_LIST
    ${SRC_DIR}/UploadFile.cpp
    ${SRC_DIR}/RunWeGlideClient.cpp
    ${SRC_DIR}/DownloadFile.cpp
    ${SRC_DIR}/TestDriver.cpp
    ${SRC_DIR}/test_fixed.cpp
    ${SRC_DIR}/TestWaypoints.cpp
    ${SRC_DIR}/test_pressure.cpp
    ${SRC_DIR}/test_task.cpp
    ${SRC_DIR}/TestOverwritingRingBuffer.cpp
    ${SRC_DIR}/TestDateTime.cpp
    ${SRC_DIR}/TestRoughTime.cpp
    ${SRC_DIR}/TestWrapClock.cpp
    ${SRC_DIR}/TestPolylineDecoder.cpp
    ${SRC_DIR}/TestTransponderCode.cpp
    ${SRC_DIR}/TestMath.cpp
    ${SRC_DIR}/TestMathTables.cpp
    ${SRC_DIR}/TestAngle.cpp
    ${SRC_DIR}/TestARange.cpp
    ${SRC_DIR}/TestGrahamScan.cpp
    ${SRC_DIR}/TestUnits.cpp
    ${SRC_DIR}/TestEarth.cpp
    ${SRC_DIR}/TestSunEphemeris.cpp
    ${SRC_DIR}/TestValidity.cpp
    ${SRC_DIR}/TestUTM.cpp
    ${SRC_DIR}/TestAllocatedGrid.cpp
    ${SRC_DIR}/TestRadixTree.cpp
    ${SRC_DIR}/TestGeoBounds.cpp
    ${SRC_DIR}/TestGeoClip.cpp
    ${SRC_DIR}/TestLogger.cpp
    ${SRC_DIR}/TestGRecord.cpp
    ${SRC_DIR}/TestClimbAvCalc.cpp
    ${SRC_DIR}/TestWaypointReader.cpp
    ${SRC_DIR}/TestThermalBase.cpp
    ${SRC_DIR}/TestFlarmNet.cpp
    ${SRC_DIR}/TestColorRamp.cpp
    ${SRC_DIR}/TestGeoPoint.cpp
    ${SRC_DIR}/TestDiffFilter.cpp
    ${SRC_DIR}/TestFileUtil.cpp
    ${SRC_DIR}/TestPolars.cpp
    ${SRC_DIR}/TestCSVLine.cpp
    ${SRC_DIR}/TestGlidePolar.cpp
    ${SRC_DIR}/test_replay_task.cpp
    ${SRC_DIR}/TestProjection.cpp
    ${SRC_DIR}/TestFlatPoint.cpp
    ${SRC_DIR}/TestFlatLine.cpp
    ${SRC_DIR}/TestFlatGeoPoint.cpp
    ${SRC_DIR}/TestMacCready.cpp
    ${SRC_DIR}/TestOrderedTask.cpp
    ${SRC_DIR}/TestAATPoint.cpp
    ${SRC_DIR}/TestPlanes.cpp
    ${SRC_DIR}/TestTaskPoint.cpp
    ${SRC_DIR}/TestTaskWaypoint.cpp
    ${SRC_DIR}/TestTeamCode.cpp
    ${SRC_DIR}/TestZeroFinder.cpp
    ${SRC_DIR}/TestAirspaceParser.cpp
    ${SRC_DIR}/TestMETARParser.cpp
    ${SRC_DIR}/TestIGCParser.cpp
    ${SRC_DIR}/TestStrings.cpp
    ${SRC_DIR}/TestUTF8.cpp
    ${SRC_DIR}/TestCRC16.cpp
    ${SRC_DIR}/TestCRC8.cpp
    ${SRC_DIR}/TestUnitsFormatter.cpp
    ${SRC_DIR}/TestGeoPointFormatter.cpp
    ${SRC_DIR}/TestHexColorFormatter.cpp
    ${SRC_DIR}/TestByteSizeFormatter.cpp
    ${SRC_DIR}/TestTimeFormatter.cpp
    ${SRC_DIR}/TestIGCFilenameFormatter.cpp
    ${SRC_DIR}/TestNMEAFormatter.cpp
    ${SRC_DIR}/TestLXNToIGC.cpp
    ${SRC_DIR}/TestLeastSquares.cpp
    ${SRC_DIR}/TestHexString.cpp
    ${SRC_DIR}/TestThermalBand.cpp
)

set(GUI_TEST_LIST
    ${SRC_DIR}/RunWindArrowRenderer.cpp
    ${SRC_DIR}/DebugDisplay.cpp
)

set(SCRIPT_FILES
   CMakeSource.cmake
)