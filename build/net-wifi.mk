# NetworkManager / ConnMan settings via D-Bus (Linux only; not Kobo, not Android)
HAVE_LINUX_NET_WIFI := n
ifeq ($(TARGET_IS_LINUX),y)
ifeq ($(TARGET_IS_KOBO),n)
ifeq ($(TARGET_IS_ANDROID),n)
HAVE_LINUX_NET_WIFI := y
endif
endif
endif

ifeq ($(HAVE_LINUX_NET_WIFI),y)
XCSOAR_SOURCES += \
	$(SRC)/net/wifi/LinuxNetWifiDbus.cpp \
	$(SRC)/net/wifi/NetworkManagerWifi.cpp \
	$(SRC)/net/wifi/ConnmanWifi.cpp \
	$(SRC)/net/wifi/WifiError.cpp \
	$(SRC)/net/wifi/WifiService.cpp \
	$(SRC)/net/wifi/LinuxWifiDialog.cpp

XCSOAR_CPPFLAGS += -DHAVE_LINUX_NET_WIFI
endif
