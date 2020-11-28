# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "CM Utility"
DESCRIPTION = "Util to communicate wiht control manager"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://cm-util.cpp;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://cm-util \
          "

S = "${WORKDIR}/cm-util"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 cm-util ${D}${bindir}/cm-util
}

DEPENDS += "libpal cli11 libobmc-i2c"
RDEPENDS_${PN} += "libpal libobmc-i2c"
LDFLAGS += "-lobmc-i2c"

FILES_${PN} = "${bindir}"
