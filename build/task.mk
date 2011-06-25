
TASK_GLUE_SRC	:=\
	$(ENGINE_SRC_DIR)/Airspace/AirspacesTerrain.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceAltitudeText.cpp 

TASK_SRC	:=\
	$(ENGINE_SRC_DIR)/Airspace/Airspace.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceAltitude.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceAircraftPerformance.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AbstractAirspace.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceCircle.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspacePolygon.cpp \
	$(ENGINE_SRC_DIR)/Airspace/Airspaces.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceIntersectSort.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceNearestSort.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceSoonestSort.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspacePredicate.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceVisitor.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceIntersectionVisitor.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceWarningManager.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceWarning.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceClass.cpp \
	$(ENGINE_SRC_DIR)/Airspace/AirspaceSorter.cpp \
	$(ENGINE_SRC_DIR)/Atmosphere/Pressure.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideState.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlidePolar.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/GlideResult.cpp \
	$(ENGINE_SRC_DIR)/GlideSolvers/MacCready.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Aircraft.cpp \
	$(ENGINE_SRC_DIR)/Navigation/GeoPoint.cpp \
	$(ENGINE_SRC_DIR)/Navigation/SearchPoint.cpp \
	$(ENGINE_SRC_DIR)/Navigation/SearchPointVector.cpp \
	$(ENGINE_SRC_DIR)/Navigation/TracePoint.cpp \
	$(ENGINE_SRC_DIR)/Navigation/TaskProjection.cpp \
	$(ENGINE_SRC_DIR)/Navigation/ConvexHull/GrahamScan.cpp \
	$(ENGINE_SRC_DIR)/Navigation/ConvexHull/PolygonInterior.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Memento/DistanceMemento.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Memento/GeoVectorMemento.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Geometry/GeoVector.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Geometry/GeoEllipse.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatBoundingBox.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatGeoPoint.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatRay.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatPoint.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatEllipse.cpp \
	$(ENGINE_SRC_DIR)/Navigation/Flat/FlatLine.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskAdvance.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskAdvanceLegacy.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskAdvanceSmart.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskBehaviour.cpp \
	$(ENGINE_SRC_DIR)/Task/OrderedTaskBehaviour.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskEvents.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskManager.cpp \
	$(ENGINE_SRC_DIR)/Task/Factory/AbstractTaskFactory.cpp \
	$(ENGINE_SRC_DIR)/Task/Factory/RTTaskFactory.cpp \
	$(ENGINE_SRC_DIR)/Task/Factory/FAITaskFactory.cpp \
	$(ENGINE_SRC_DIR)/Task/Factory/FAITriangleTaskFactory.cpp \
	$(ENGINE_SRC_DIR)/Task/Factory/FAIORTaskFactory.cpp \
	$(ENGINE_SRC_DIR)/Task/Factory/FAIGoalTaskFactory.cpp \
	$(ENGINE_SRC_DIR)/Task/Factory/AATTaskFactory.cpp \
	$(ENGINE_SRC_DIR)/Task/Factory/MixedTaskFactory.cpp \
	$(ENGINE_SRC_DIR)/Task/Factory/TouringTaskFactory.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskInterface.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/AbortTask.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/AlternateTask.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/AbstractTask.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/GotoTask.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/OrderedTask.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/UnorderedTask.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/ContestManager.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/BaseTask/IntermediatePoint.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/BaseTask/ObservationZoneClient.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/BaseTask/ObservationZonePoint.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/BaseTask/OrderedTaskPoint.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/BaseTask/SampledTaskPoint.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/BaseTask/ScoredTaskPoint.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/BaseTask/TaskLeg.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/BaseTask/UnorderedTaskPoint.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskPoints/StartPoint.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskPoints/FinishPoint.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskPoints/ASTPoint.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskPoints/AATPoint.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskPoints/AATIsoline.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskPoints/AATIsolineSegment.cpp \
	$(ENGINE_SRC_DIR)/Task/ObservationZones/CylinderZone.cpp \
	$(ENGINE_SRC_DIR)/Task/ObservationZones/SectorZone.cpp \
	$(ENGINE_SRC_DIR)/Task/ObservationZones/LineSectorZone.cpp \
	$(ENGINE_SRC_DIR)/Task/ObservationZones/SymmetricSectorZone.cpp \
	$(ENGINE_SRC_DIR)/Task/ObservationZones/KeyholeZone.cpp \
	$(ENGINE_SRC_DIR)/Task/ObservationZones/BGAFixedCourseZone.cpp \
	$(ENGINE_SRC_DIR)/Task/ObservationZones/BGAEnhancedOptionZone.cpp \
	$(ENGINE_SRC_DIR)/Task/ObservationZones/AnnularSectorZone.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/Contests.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/AbstractContest.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/TaskDijkstra.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/TaskDijkstraMin.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/TaskDijkstraMax.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/ContestDijkstra.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/OLCLeague.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/OLCSprint.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/OLCClassic.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/OLCTriangle.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/OLCFAI.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/OLCPlus.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/XContestFree.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/XContestTriangle.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/OLCSISAT.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/PathSolvers/IsolineCrossingFinder.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskSolvers/TaskMacCready.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskSolvers/TaskMacCreadyTravelled.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskSolvers/TaskMacCreadyRemaining.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskSolvers/TaskMacCreadyTotal.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskSolvers/TaskBestMc.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskSolvers/TaskSolveTravelled.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskSolvers/TaskCruiseEfficiency.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskSolvers/TaskEffectiveMacCready.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskSolvers/TaskMinTarget.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskSolvers/TaskOptTarget.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskSolvers/TaskGlideRequired.cpp \
	$(ENGINE_SRC_DIR)/Task/Tasks/TaskSolvers/TaskSolution.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskStats/DistanceStat.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskStats/CommonStats.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskStats/ElementStat.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskStats/TaskStats.cpp \
	$(ENGINE_SRC_DIR)/Task/TaskStats/TaskVario.cpp \
	$(ENGINE_SRC_DIR)/Task/Visitors/ObservationZoneVisitor.cpp \
	$(ENGINE_SRC_DIR)/Task/Visitors/TaskPointVisitor.cpp \
	$(ENGINE_SRC_DIR)/Task/Visitors/TaskVisitor.cpp \
	$(ENGINE_SRC_DIR)/Route/RoutePlanner.cpp \
	$(ENGINE_SRC_DIR)/Route/AirspaceRoute.cpp \
	$(ENGINE_SRC_DIR)/Route/TerrainRoute.cpp \
	$(ENGINE_SRC_DIR)/Route/RoutePolar.cpp \
	$(ENGINE_SRC_DIR)/Route/ReachFan.cpp \
	$(ENGINE_SRC_DIR)/Route/AbstractReach.cpp \
	$(ENGINE_SRC_DIR)/Trace/Trace.cpp \
	$(ENGINE_SRC_DIR)/Waypoint/Waypoint.cpp \
	$(ENGINE_SRC_DIR)/Waypoint/Waypoints.cpp \
	$(ENGINE_SRC_DIR)/Waypoint/WaypointVisitor.cpp \
	$(ENGINE_SRC_DIR)/Math/Earth.cpp \
	$(ENGINE_SRC_DIR)/Util/AircraftStateFilter.cpp \
	$(ENGINE_SRC_DIR)/Util/DiffFilter.cpp \
	$(ENGINE_SRC_DIR)/Util/Filter.cpp \
	$(ENGINE_SRC_DIR)/Util/Gradient.cpp \
	$(ENGINE_SRC_DIR)/Util/ZeroFinder.cpp \
	$(ENGINE_SRC_DIR)/Util/Serialiser.cpp \
	$(ENGINE_SRC_DIR)/Util/Deserialiser.cpp \
	$(ENGINE_SRC_DIR)/Util/DataNodeXML.cpp \
	$(ENGINE_SRC_DIR)/Util/DataNode.cpp


ENGINE_CORE_LIBS = $(TARGET_OUTPUT_DIR)/task.a
ENGINE_GLUE_LIBS = $(TARGET_OUTPUT_DIR)/taskglue.a

ENGINE_LIBS = $(ENGINE_CORE_LIBS) $(ENGINE_GLUE_LIBS)

$(ENGINE_CORE_LIBS): $(call SRC_TO_OBJ,$(TASK_SRC))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^

$(ENGINE_GLUE_LIBS): $(call SRC_TO_OBJ,$(TASK_GLUE_SRC))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
