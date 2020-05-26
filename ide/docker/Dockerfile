FROM debian:buster
RUN echo 'deb http://deb.debian.org/debian stretch main' > /etc/apt/sources.list.d/unstable.list
RUN echo 'APT::Default-Release "stable";' > /etc/apt/apt.conf.d/99defaultrelease

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
COPY ./provisioning/install-debian-packages.sh /root/install-debian-packages.sh
RUN /root/install-debian-packages.sh && apt-get clean
COPY ./provisioning/install-android-tools.sh /root/install-android-tools.sh
RUN chmod 755 /root/install-android-tools.sh /root/install-debian-packages.sh
RUN /root/install-android-tools.sh && apt-get clean
RUN apt-get -y install python-minimal python3 vim ccache python3-pkgconfig && apt-get clean

WORKDIR /opt/xcsoar

COPY ./docker/bin/* /usr/local/bin/
RUN cd /usr/local/bin && chmod 755 ./*

# Environment to support running XCSoar
ENV DISPLAY=host.docker.internal:0
ENV SDL_RENDER_DRIVER=software
# We set the home directory to the project root such that XCSoar generated files
# are retained.
ENV HOME=/opt/xcsoar
