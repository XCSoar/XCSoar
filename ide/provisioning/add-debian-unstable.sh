#!/bin/bash

# Adds stretch sources list to debian for jdk8 dependency
echo 'deb http://deb.debian.org/debian stretch main' > /etc/apt/sources.list.d/unstable.list
echo 'APT::Default-Release "stable";' > /etc/apt/apt.conf.d/99defaultrelease
