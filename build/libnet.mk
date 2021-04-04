# Build rules for the networking library

LIBNET_SOURCES = \
	$(SRC)/net/AddressInfo.cxx \
	$(SRC)/net/HostParser.cxx \
	$(SRC)/net/Resolver.cxx \
	$(SRC)/net/SocketError.cxx \
	$(SRC)/net/State.cpp \
	$(SRC)/net/ToString.cxx \
	$(SRC)/net/IPv4Address.cxx \
	$(SRC)/net/IPv6Address.cxx \
	$(SRC)/net/StaticSocketAddress.cxx \
	$(SRC)/net/AllocatedSocketAddress.cxx \
	$(SRC)/net/SocketAddress.cxx \
	$(SRC)/net/SocketDescriptor.cxx

$(eval $(call link-library,libnet,LIBNET))
