# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Enclosure Utility"
DESCRIPTION = "A Enclosure Management Util for Display HDD status"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://enclosure-util.c;beginline=4;endline=16;md5=302e73da84735a7814365fd8ab355e2d"

SRC_URI = "file://enclosure-util \
          "

S = "${WORKDIR}/enclosure-util"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 enclosure-util ${D}${bindir}/enclosure-util
}

DEPENDS += " libbic libnvme-mi libpal libfby3-common"
RDEPENDS_${PN} += " libbic libnvme-mi libpal libfby3-common"
FILES_${PN} = "${bindir}"
