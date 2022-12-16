set(_SOURCES
        Dialogs/Airspace/AirspaceCRendererSettingsDialog.cpp
        Dialogs/Airspace/AirspaceCRendererSettingsPanel.cpp
        Dialogs/Airspace/AirspaceList.cpp
        Dialogs/Airspace/dlgAirspace.cpp
        Dialogs/Airspace/dlgAirspaceDetails.cpp
        Dialogs/Airspace/dlgAirspacePatterns.cpp
        Dialogs/Airspace/dlgAirspaceWarnings.cpp
        Dialogs/ColorListDialog.cpp
        Dialogs/ComboPicker.cpp
        Dialogs/DataField.cpp
        Dialogs/Device/BlueFly/BlueFlyConfigurationDialog.cpp
        Dialogs/Device/CAI302/UnitsEditor.cpp
        Dialogs/Device/CAI302/WaypointUploader.cpp
        Dialogs/Device/DeviceEditWidget.cpp
        Dialogs/Device/DeviceListDialog.cpp
        Dialogs/Device/FLARM/ConfigWidget.cpp
        Dialogs/Device/LX/ManageLX16xxDialog.cpp
        Dialogs/Device/LX/ManageNanoDialog.cpp
        Dialogs/Device/LX/ManageLXNAVVarioDialog.cpp
        Dialogs/Device/LX/NanoConfigWidget.cpp
        Dialogs/Device/LX/LXNAVVarioConfigWidget.cpp
        Dialogs/Device/ManageCAI302Dialog.cpp
        Dialogs/Device/ManageFlarmDialog.cpp
        Dialogs/Device/PortMonitor.cpp
        Dialogs/Device/PortPicker.cpp
        Dialogs/Device/PortDataField.cpp
        Dialogs/Device/Vega/SwitchesDialog.cpp
        Dialogs/Device/Vega/VegaConfigurationDialog.cpp
        Dialogs/Device/Vega/VegaDemoDialog.cpp
        Dialogs/Device/Vega/VegaParametersWidget.cpp
        Dialogs/DialogSettings.cpp
        Dialogs/dlgAnalysis.cpp
        Dialogs/dlgChecklist.cpp
        Dialogs/dlgCredits.cpp
        Dialogs/dlgInfoBoxAccess.cpp
        Dialogs/dlgQuickMenu.cpp
        Dialogs/dlgSimulatorPrompt.cpp
        Dialogs/dlgStatus.cpp
        Dialogs/DownloadFilePicker.cpp
        Dialogs/Error.cpp
        Dialogs/FileManager.cpp
        Dialogs/FilePicker.cpp
        Dialogs/GeoPointEntry.cpp
        Dialogs/HelpDialog.cpp
        Dialogs/Inflate.cpp
        Dialogs/JobDialog.cpp
        Dialogs/KnobTextEntry.cpp
        Dialogs/ListPicker.cpp
        Dialogs/LockScreen.cpp
        Dialogs/MapItemListDialog.cpp
        Dialogs/MapItemListSettingsDialog.cpp
        Dialogs/MapItemListSettingsPanel.cpp
        Dialogs/Message.cpp
        Dialogs/NumberEntry.cpp

        Dialogs/Plane/PlaneDetailsDialog.cpp
        Dialogs/Plane/PlaneListDialog.cpp
        Dialogs/Plane/PlanePolarDialog.cpp
        Dialogs/Plane/PolarShapeEditWidget.cpp

        Dialogs/ProfileListDialog.cpp
        Dialogs/ProfilePasswordDialog.cpp

        Dialogs/ProgressDialog.cpp
        Dialogs/ReplayDialog.cpp

        Dialogs/Settings/dlgBasicSettings.cpp
        Dialogs/Settings/dlgConfigInfoboxes.cpp
        Dialogs/Settings/dlgConfiguration.cpp
        Dialogs/Settings/Panels/AirspaceConfigPanel.cpp
        Dialogs/Settings/Panels/CloudConfigPanel.cpp
        Dialogs/Settings/Panels/WeGlideConfigPanel.cpp
        Dialogs/Settings/Panels/GaugesConfigPanel.cpp
        Dialogs/Settings/Panels/GlideComputerConfigPanel.cpp
        Dialogs/Settings/Panels/InfoBoxesConfigPanel.cpp
        Dialogs/Settings/Panels/InterfaceConfigPanel.cpp
        Dialogs/Settings/Panels/LayoutConfigPanel.cpp
        Dialogs/Settings/Panels/LoggerConfigPanel.cpp
        Dialogs/Settings/Panels/MapDisplayConfigPanel.cpp
        Dialogs/Settings/Panels/PagesConfigPanel.cpp
        Dialogs/Settings/Panels/RouteConfigPanel.cpp
        Dialogs/Settings/Panels/SafetyFactorsConfigPanel.cpp
        Dialogs/Settings/Panels/ScoringConfigPanel.cpp
        Dialogs/Settings/Panels/SiteConfigPanel.cpp
        Dialogs/Settings/Panels/SymbolsConfigPanel.cpp
        Dialogs/Settings/Panels/TaskDefaultsConfigPanel.cpp
        Dialogs/Settings/Panels/TaskRulesConfigPanel.cpp
        Dialogs/Settings/Panels/TerrainDisplayConfigPanel.cpp
        Dialogs/Settings/Panels/TimeConfigPanel.cpp
        Dialogs/Settings/Panels/TrackingConfigPanel.cpp
        Dialogs/Settings/Panels/UnitsConfigPanel.cpp
        Dialogs/Settings/Panels/VarioConfigPanel.cpp
        Dialogs/Settings/Panels/WaypointDisplayConfigPanel.cpp
        Dialogs/Settings/Panels/WeatherConfigPanel.cpp
        Dialogs/Settings/Panels/WindConfigPanel.cpp
        Dialogs/Settings/WindSettingsDialog.cpp
        Dialogs/Settings/WindSettingsPanel.cpp

       Dialogs/SimulatorPromptWindow.cpp
        Dialogs/StartupDialog.cpp

        Dialogs/StatusPanels/FlightStatusPanel.cpp
        Dialogs/StatusPanels/RulesStatusPanel.cpp
        Dialogs/StatusPanels/StatusPanel.cpp
        Dialogs/StatusPanels/SystemStatusPanel.cpp
        Dialogs/StatusPanels/TaskStatusPanel.cpp
        Dialogs/StatusPanels/TimesStatusPanel.cpp

        Dialogs/Task/AlternatesListDialog.cpp
        Dialogs/Task/dlgTaskHelpers.cpp
        Dialogs/Task/Manager/TaskActionsPanel.cpp
        Dialogs/Task/Manager/TaskClosePanel.cpp
        Dialogs/Task/Manager/TaskEditPanel.cpp
        Dialogs/Task/Manager/TaskListPanel.cpp
        Dialogs/Task/Manager/TaskManagerDialog.cpp
        Dialogs/Task/Manager/TaskMapButtonRenderer.cpp
        Dialogs/Task/Manager/TaskMiscPanel.cpp
        Dialogs/Task/Manager/TaskPropertiesPanel.cpp
        Dialogs/Task/MutateTaskPointDialog.cpp
        Dialogs/Task/OptionalStartsDialog.cpp
        Dialogs/Task/TargetDialog.cpp
        Dialogs/Task/TaskPointDialog.cpp
        Dialogs/Task/Widgets/CylinderZoneEditWidget.cpp
        Dialogs/Task/Widgets/KeyholeZoneEditWidget.cpp
        Dialogs/Task/Widgets/LineSectorZoneEditWidget.cpp
        Dialogs/Task/Widgets/ObservationZoneEditWidget.cpp
        Dialogs/Task/Widgets/SectorZoneEditWidget.cpp

        Dialogs/TextEntry.cpp
        Dialogs/TimeEntry.cpp
        Dialogs/DateEntry.cpp
        Dialogs/TouchTextEntry.cpp

        Dialogs/Tracking/CloudEnableDialog.cpp
        Dialogs/Traffic/FlarmTrafficDetails.cpp
        Dialogs/Traffic/TeamCodeDialog.cpp
        Dialogs/Traffic/TrafficList.cpp

        Dialogs/Waypoint/dlgWaypointDetails.cpp
        Dialogs/Waypoint/dlgWaypointEdit.cpp
        Dialogs/Waypoint/Manager.cpp
        Dialogs/Waypoint/NearestWaypoint.cpp
        Dialogs/Waypoint/WaypointCommandsWidget.cpp
        Dialogs/Waypoint/WaypointInfoWidget.cpp
        Dialogs/Waypoint/WaypointList.cpp
        Dialogs/Weather/NOAADetails.cpp
        Dialogs/Weather/NOAAList.cpp
        Dialogs/Weather/PCMetDialog.cpp
        Dialogs/Weather/RASPDialog.cpp
        Dialogs/Weather/WeatherDialog.cpp

        Dialogs/WidgetDialog.cpp

        Dialogs/CoDialog.cpp
)
if(UNIX)
  list(APPEND _SOURCES
        Dialogs/Settings/Panels/AudioVarioConfigPanel.cpp
        Dialogs/Settings/Panels/AudioConfigPanel.cpp

        Dialogs/Weather/MapOverlayWidget.cpp
  )
endif()

set(SCRIPT_FILES
    CMakeSource.cmake

    ../../build/main.mk
)

