# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Setup GPIO when BMC boot up"
DESCRIPTION = "Set and export GPIO"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://setup-gpio.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://setup-gpio.c \
           file://Makefile \
           file://setup-gpio.sh \
          "
S = "${WORKDIR}"

binfiles = "setup-gpio \
           "
DEPENDS += " libgpio-ctrl update-rc.d-native "
RDEPENDS_${PN} += " libgpio-ctrl "
LDFLAGS += " -lgpio-ctrl "

pkgdir = "setup-gpio"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -d ${D}${sysconfdir}/init.d
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
  install -m 755 setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
  update-rc.d -r ${D} setup-gpio.sh start 59 5 .
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/setup-gpio/ ${prefix}/local/bin ${sysconfdir}"
