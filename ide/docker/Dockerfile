FROM debian:buster
RUN echo 'deb http://deb.debian.org/debian stretch main' > /etc/apt/sources.list.d/unstable.list
RUN echo 'APT::Default-Release "stable";' > /etc/apt/apt.conf.d/99defaultrelease

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
COPY ./provisioning/install-debian-packages.sh /root/install-debian-packages.sh
RUN /root/install-debian-packages.sh && apt-get clean
COPY ./provisioning/install-android-tools.sh /root/install-android-tools.sh
RUN chmod 0755 /root/install-android-tools.sh /root/install-debian-packages.sh
RUN /root/install-android-tools.sh && apt-get clean
<<<<<<< HEAD
RUN apt-get -y --install-no-recommends install python-minimal python3 vim ccache python3-pkgconfig gdb && apt-get clean
=======
RUN apt-get install --install-no-recommends python-minimal python3 vim ccache python3-pkgconfig gdb && apt-get clean
>>>>>>> de81b36c96... ide/docker: remove -y flag

COPY ./docker/bin/* /usr/local/bin/
WORKDIR /usr/local/bin
RUN chmod 755 ./*

WORKDIR /opt/xcsoar
