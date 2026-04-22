# Build rules for the networking library

LIBNET_SOURCES = \
	$(SRC)/net/AddressInfo.cxx \
	$(SRC)/net/HostParser.cxx \
	$(SRC)/net/Resolver.cxx \
	$(SRC)/net/SocketError.cxx \
	$(SRC)/net/State.cpp \
	$(SRC)/net/Reachability.cxx \
	$(SRC)/net/ToString.cxx \
	$(SRC)/net/IPv4Address.cxx \
	$(SRC)/net/IPv6Address.cxx \
	$(SRC)/net/StaticSocketAddress.cxx \
	$(SRC)/net/AllocatedSocketAddress.cxx \
	$(SRC)/net/SocketAddress.cxx \
	$(SRC)/net/SocketDescriptor.cxx

LIBNET_DEPENDS = FMT

# Linux desktop: NetworkManager and ConnMan Manager (State) on D-Bus; Kobo
# and other non-(Linux+poll) builds have no D-Bus in this set (see
# build/libdbus.mk) and keep sysfs-only in #State.cpp.
ifeq ($(TARGET_IS_LINUX)$(USE_POLL_EVENT)$(TARGET_IS_KOBO),yyn)
LIBNET_SOURCES += \
	$(SRC)/net/StateNMDbus.cxx \
	$(SRC)/net/StateConnmanDbus.cxx
LIBNET_CPPFLAGS += -DHAVE_NET_STATE_NM_DBUS -DHAVE_NET_STATE_CONNMAN_DBUS
LIBNET_DEPENDS += DBUS
endif

$(eval $(call link-library,libnet,LIBNET))
