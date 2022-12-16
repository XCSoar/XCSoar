
set(_SOURCES
XCSOAR_SOURCES += \
	java/Global.cxx \
	java/Object.cxx \
	java/String.cxx \
	java/Exception.cxx \
	java/File.cxx \
	java/Path.cxx \
	java/InputStream.cxx \
	java/URL.cxx \
	java/Closeable.cxx \
	Device/AndroidSensors.cpp \
	Device/Port/AndroidPort.cpp \
	Device/Port/AndroidBluetoothPort.cpp \
	Device/Port/AndroidIOIOUartPort.cpp \
	Device/Port/AndroidUsbSerialPort.cpp \
	Android/NativeView.cpp \
	Android/Environment.cpp \
	Android/Bitmap.cpp \
	Android/Product.cpp \
	Android/Nook.cpp \
	Android/InternalSensors.cpp \
	Android/SoundUtil.cpp \
	Android/TextUtil.cpp \
	Android/EventBridge.cpp \
	Android/NativePortListener.cpp \
	Android/NativeInputListener.cpp \
	Android/PortBridge.cpp \
	Android/Sensor.cpp \
	Android/BluetoothHelper.cpp \
	Android/NativeDetectDeviceListener.cpp \
	Android/NativeSensorListener.cpp \
	Android/Battery.cpp \
	Android/GliderLink.cpp \
	Android/DownloadManager.cpp \
	Android/Vibrator.cpp \
	Android/Context.cpp \
	Android/BMP085Device.cpp \
	Android/I2CbaroDevice.cpp \
	Android/NunchuckDevice.cpp \
	Android/VoltageDevice.cpp \
	Android/IOIOHelper.cpp \
	Android/UsbSerialHelper.cpp \
	Android/TextEntryDialog.cpp \
	Android/FileProvider.cpp \
	Android/Main.cpp

)

set(SCRIPT_FILES
    CMakeSource.cmake
)

# # # jetzt nicht mehr so ;-)  
# # # jetzt nicht mehr so ;-)  
# # # jetzt nicht mehr so ;-)  add_library(${LIB_TARGET_NAME} ${XCSOAR_LIB_TYPE}
# # # jetzt nicht mehr so ;-)      ${SOURCE_FILES}
# # # jetzt nicht mehr so ;-)      ${HEADER_FILES}
# # # jetzt nicht mehr so ;-)      ${CMAKE_CURRENT_LIST_DIR}/CMakeSource.cmake
# # # jetzt nicht mehr so ;-)      ${SCRIPT_FILES}
# # # jetzt nicht mehr so ;-)  )
# # # jetzt nicht mehr so ;-)  # message(FATAL_ERROR "Stop!")
# # # jetzt nicht mehr so ;-)  
# # # jetzt nicht mehr so ;-)  set_target_properties(${LIB_TARGET_NAME} PROPERTIES FOLDER Libs)
# # # jetzt nicht mehr so ;-)  
# # # jetzt nicht mehr so ;-)  target_link_libraries(${LIB_TARGET_NAME} PUBLIC IO)
# # # jetzt nicht mehr so ;-)  
# # # jetzt nicht mehr so ;-)  endif()
