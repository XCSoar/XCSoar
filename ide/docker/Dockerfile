FROM debian:bookworm

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y python3 vim ccache python3-pkgconfig gdb && apt-get clean
COPY ./provisioning/install-debian-packages.sh /root/install-debian-packages.sh
RUN /root/install-debian-packages.sh && apt-get clean
COPY ./provisioning/install-android-tools.sh /root/install-android-tools.sh
RUN chmod 0755 /root/install-android-tools.sh /root/install-debian-packages.sh
RUN /root/install-android-tools.sh && apt-get clean

COPY ./docker/bin/* /usr/local/bin/
WORKDIR /usr/local/bin
RUN chmod 755 ./*

WORKDIR /opt/xcsoar
ENV CCACHE_DIR=/opt/xcsoar/.ccache
