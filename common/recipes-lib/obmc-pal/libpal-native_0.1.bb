# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "OpenBMC PAL"
DESCRIPTION = "Platform Abstraction Layer for OpenBMC"

SECTION = "libs"
PR = "r1"

LICENSE = "LGPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/LGPL-2.1;md5=1a6d268fd218675ffea8be556788b780"

inherit native

SRC_URI = "file://pal.c \
           file://pal.h \
           file://pal_sensors.h \
           file://Makefile \
          "
DEPENDS += " libkv-native libipmi-native obmc-pal-native"
LDFLAGS += ""
SOURCES = "pal.c"
HEADERS = "pal.h pal_sensors.h"

S = "${WORKDIR}"

do_compile() {
  make SOURCES="${SOURCES}" HEADERS="${HEADERS}"
}

do_install_append() {
  install -d ${D}${libdir}
  install -m 0644 libpal.so ${D}${libdir}/libpal.so

  install -d ${D}${includedir}/openbmc
  for f in ${HEADERS}; do
    install -m 0644 ${S}/$f ${D}${includedir}/openbmc/$f
  done
}

