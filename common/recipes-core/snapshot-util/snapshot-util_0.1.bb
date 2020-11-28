# Copyright 2019-present Facebook. All Rights Reserved.
SUMMARY = "RMA Snapshot Utility"
DESCRIPTION = "Util for creating RMA snapshot"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://snapshot-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://snapshot-util.c \
           file://Makefile \
          "

S = "${WORKDIR}"

binfiles = "snapshot-util \
           "

pkgdir = "snapshot-util"

DEPENDS += "libbic libpal"
RDEPENDS_${PN} += "libbic libpal"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${binfiles}; do
    install -m 755 $f ${dst}/$f
    ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/snapshot-util ${prefix}/local/bin"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
